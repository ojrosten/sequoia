#!/bin/bash
# Controls for check_tracefile.py.
#
# Every fixture starts from one clean capture, filtered tracefile and summary, which must
# pass; each control then breaks one thing, and must fail with a message naming it:
#
#   - a count changed by the read, in either direction: a lambda's FNA raised from 0, which
#     is what lcov 2.5's consistency repair does, and a function's FNA lowered to 0;
#   - a line count changed, a record added and a record lost;
#   - a file dropped although it has coverage points, whether they are function records
#     alone (lcov deletes, without a message, any file with no line records on reading it,
#     and llvm-cov writes such files) or line records none of which was hit;
#   - a file kept although a pattern removes it, and a file absent from the capture;
#   - a summary figure that disagrees with the records, and a summary with no figures. The
#     capture holds a function with two aliases, one hit, because lcov counts aliases: a
#     check that counted functions by their FNL index instead would disagree with lcov;
#   - malformed input: a second record for one file, a record with no end, no records.
#
# The capture also holds a file whose path contains a removal pattern's text past its
# start. lcov removes it, since lcov searches the whole path; a check that anchored the
# pattern at the start of the path, as shell globbing does, would call it dropped.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script="$here/../check_tracefile.py"
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fails=0

capture() {
  cat <<'EOF'
TN:
SF:/src/a.cpp
FNL:0,1,3
FNA:0,2,_Z1fv
FNL:1,5,5
FNA:1,0,_ZZ1gvENKUlvE_clEv
FNL:2,7,7
FNA:2,0,_Z1hIiEvv
FNA:2,3,_Z1hIlEvv
FNF:3
FNH:2
DA:1,2
DA:2,2
DA:5,1
DA:7,3
LF:4
LH:4
end_of_record
SF:/usr/include/x.h
DA:1,4
LF:1
LH:1
end_of_record
SF:/sdk/usr/include/y.h
DA:1,1
LF:1
LH:1
end_of_record
SF:/src/empty.hpp
FNF:0
FNH:0
LF:0
LH:0
end_of_record
EOF
}

filtered() {
  capture | awk '/^SF:\/src\/a\.cpp$/{p=1} p{print} p && /^end_of_record$/{exit}'
}

summary() {
  cat <<'EOF'
Summary coverage rate:
  source files: 1
  lines.......: 100.0% (4 of 4 lines)
  functions...: 50.0% (2 of 4 functions)
EOF
}

run() { # run <name> <0|1> <pattern>; reads $tmp/capture, $tmp/filtered, $tmp/summary
  python3 "$script" --capture "$tmp/capture" --filtered "$tmp/filtered" --summary "$tmp/summary" \
                    --removed '/usr/*' > "$tmp/out" 2>&1
  local status=$?
  if [[ $status -ne $2 ]]; then
    echo "FAIL: $1 (expected exit $2, got $status)"; sed 's/^/    /' "$tmp/out"; fails=$((fails+1))
  elif ! grep -qE -- "$3" "$tmp/out"; then
    echo "FAIL: $1 (output lacks: $3)"; sed 's/^/    /' "$tmp/out"; fails=$((fails+1))
  fi
}

reset() { capture > "$tmp/capture"; filtered > "$tmp/filtered"; summary > "$tmp/summary"; }

edit() { # edit <file> <sed expression>
  sed -e "$2" "$tmp/$1" > "$tmp/edited" && mv "$tmp/edited" "$tmp/$1"
}

reset
run "a clean pair passes" 0 "1 of 4 captured files kept unchanged; 2 removed by pattern, 1 with no coverage points"
run "a clean pair reports the figures" 0 "4 of 4 lines, 2 of 4 functions"

reset; edit filtered 's/^FNA:1,0,/FNA:1,1,/'; edit summary 's/(2 of 4 functions)/(3 of 4 functions)/'
run "a lambda marked called is named" 1 \
    'the capture has "FNA:1,0,_ZZ1gvENKUlvE_clEv" where the filtered tracefile has "FNA:1,1,'

reset; edit filtered 's/^FNA:0,2,/FNA:0,0,/'; edit summary 's/(2 of 4 functions)/(1 of 4 functions)/'
run "a called function marked uncalled is named" 1 'the capture has "FNA:0,2,_Z1fv"'

reset; edit filtered 's/^DA:2,2$/DA:2,3/'
run "a changed line count is named" 1 'the capture has "DA:2,2" where the filtered tracefile has "DA:2,3"'

reset; edit filtered 's/^LH:4$/LH:4\
DA:9,0/'; edit summary 's/(4 of 4 lines)/(4 of 5 lines)/'
run "a record the capture lacks is named" 1 'the capture has "\(nothing\)" where the filtered tracefile has "DA:9,0"'

reset; edit filtered '/^LH:4$/d'
run "a lost record is named" 1 'the capture has "LH:4" where the filtered tracefile has "\(nothing\)"'

reset; printf '%s\n' SF:/src/b.hpp FNL:0,30 FNA:0,15,_ZN1bC2ERKS_ FNF:1 FNH:1 LF:0 LH:0 end_of_record >> "$tmp/capture"
run "a dropped file with only function records is named" 1 \
    '/src/b.hpp has coverage points and matches no removal pattern, but was dropped'

reset; printf '%s\n' SF:/src/cold.cpp DA:1,0 LF:1 LH:0 end_of_record >> "$tmp/capture"
run "a dropped file with only unhit lines is named" 1 \
    '/src/cold.cpp has coverage points and matches no removal pattern, but was dropped'

reset; capture | awk '/^SF:\/usr\/include\/x\.h$/{p=1} p{print} p && /^end_of_record$/{exit}' >> "$tmp/filtered"
edit summary 's/(4 of 4 lines)/(5 of 5 lines)/'
run "a kept file a pattern removes is named" 1 '/usr/include/x.h matches a removal pattern but was kept'

reset; edit filtered 's|^SF:/src/a.cpp$|SF:/src/c.cpp|'
run "a file absent from the capture is named" 1 '/src/c.cpp is in the filtered tracefile but not the capture'

reset; edit summary 's/(2 of 4 functions)/(2 of 3 functions)/'
run "a function figure the records disagree with" 1 \
    'reports 2 of 3 functions hit, but the filtered tracefile has 2 of 4 FNA records'

reset; edit summary 's/(4 of 4 lines)/(3 of 4 lines)/'
run "a line figure the records disagree with" 1 \
    'reports 3 of 4 lines hit, but the filtered tracefile has 4 of 4 DA records'

reset; edit summary '/functions/d'
run "a summary with no function figure" 1 'the summary has no functions figure'

reset; filtered >> "$tmp/capture"
run "a second record for one file" 1 '/src/a.cpp has a second record'

reset; edit filtered '/^end_of_record$/d'
run "a record with no end" 1 'the record for /src/a.cpp has no end_of_record'

reset; printf 'TN:\n' > "$tmp/filtered"
run "a tracefile with no records" 1 'has no file records'

reset; edit filtered '1i\
DA:1,1'
run "a record outside any file" 1 '"DA:1,1" is outside any file'

if [[ $fails -ne 0 ]]; then
  echo "$fails control(s) failed"
  exit 1
fi
echo "check_tracefile.py: all controls pass"
