#!/bin/bash
# Controls for check_tracefile.py, and a mutation check of the controls.
#
#   check_tracefile_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the script and against each mutant of the script.
# The script must fail no control, and each mutant at least one. Only then
# does the last line say that every mutant was killed, and the status is 0.
#
# Every fixture starts from one clean capture, filtered tracefile and summary,
# which must pass. Each control then changes one thing. The claims:
#
#   - a count changed by the read fails, in either direction: a lambda's FNA
#     raised from 0, which is what lcov 2.5's consistency repair does, and a
#     function's FNA lowered to 0. So do a changed line count, a record added,
#     a record lost, and a carriage return added, since records are compared
#     byte for byte;
#   - a file dropped although it has records besides function records fails:
#     one with line records none of which was hit, and one with line records
#     beside function records;
#   - a file kept although a pattern removes it fails, and so does a file
#     absent from the capture;
#   - a pattern matches as lcov's does. `*` is any run of characters, and `?`
#     exactly one. Every other character is literal, and letters keep their
#     case. A pattern may match anywhere in the path: the capture holds a file
#     whose path contains a pattern's text past its start, which a check
#     anchoring the pattern at the start of the path would call dropped. A
#     file which any one pattern matches is removed;
#   - a summary figure which disagrees with the records fails, and so does a
#     summary with no figure for lines or for functions. The capture holds a
#     function with two aliases, one hit, because lcov counts aliases: a check
#     that counted functions by their FNL index instead would disagree with
#     lcov;
#   - malformed input fails, and the error gives the file and line: a second
#     record for one file, a record with no end, whether at the end of the
#     file or before the next file's record, no records, and a line outside
#     any file's record other than a test name or a blank line;
#   - a file which cannot be read fails with an error rather than a traceback;
#   - each option is required, and --removed needs at least one pattern;
#   - bytes which are not UTF-8 pass through as themselves. A clean pair with
#     such a symbol passes, the byte is quoted as itself when a count changes,
#     and a changed byte fails. A dropped file whose path holds such a byte is
#     listed as itself, and a summary holding such a byte is read.
#
# Two kinds of dropped file pass. The script lists each kind, sorted, under a
# heading with a count, and prints the heading when the count is 0:
#
#   - a file with no coverage points. The clean pair has one. A fixture adds
#     two more such files, out of order, between them holding every per-file
#     total, and a system header with no coverage points, which belongs under
#     the removal patterns instead;
#   - a file with function records but no line records, which lcov deletes on
#     reading any tracefile and llvm-cov writes. A fixture adds two, out of
#     order.
#
# Each control on a listing compares the whole of it, so that a file listed
# under the wrong heading fails.

set -u
if [[ $# -gt 1 || ( $# -eq 1 && $1 != --mutations ) ]]; then
  echo "usage: $(basename "$0") [--mutations]" >&2; exit 2
fi
here=$(cd "$(dirname "$0")" && pwd -P)
script="$here/../check_tracefile.py"
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
verbose=yes

fail() {
  fails=$((fails+1))
  if [[ $verbose == yes ]]; then echo "FAIL: $*"; sed 's/^/    /' "$out"; fi
}

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
Reading tracefile filtered.
Summary coverage rate:
  source files: 1
  lines.......: 100.0% (4 of 4 lines)
  functions...: 50.0% (2 of 4 functions)
Message summary:
  no messages were reported
EOF
}

# invoke <name> <status> <pattern> <argument>...: the subject, given the
# arguments, exits with the status, and its output matches the pattern
invoke() {
  local name=$1 status=$2 pattern=$3 rc
  shift 3
  total=$((total+1))
  python3 "$subject" "$@" > "$out" 2>&1; rc=$?
  if [[ $rc -ne $status ]]; then
    fail "$name (expected exit $status, got $rc)"
  elif ! LC_ALL=C grep -qE -- "$pattern" "$out"; then
    fail "$name (output lacks: $pattern)"
  fi
}

# run <name> <status> <pattern>: invoke, on $tmp's capture, filtered tracefile
# and summary, and the patterns in $removed
run() {
  invoke "$1" "$2" "$3" --capture "$tmp/capture" --filtered "$tmp/filtered" --summary "$tmp/summary" \
                        --removed "${removed[@]}"
}

reset() {
  capture > "$tmp/capture"; filtered > "$tmp/filtered"; summary > "$tmp/summary"; removed=('/usr/*')
}

edit() { # edit <file> <sed expression>
  sed -e "$2" "$tmp/$1" > "$tmp/edited" && mv "$tmp/edited" "$tmp/$1"
}

listed() { # listed <name> <line>...: the lines after the first of the output
  local name=$1; shift
  total=$((total+1))
  printf '%s\n' "$@" > "$tmp/listing"
  if ! tail -n +2 "$out" | cmp -s - "$tmp/listing"; then
    fail "$name (expected the listing after the output)"
    if [[ $verbose == yes ]]; then sed 's/^/    expected: /' "$tmp/listing"; fi
  fi
}

noPoints='Dropped by lcov for having no coverage points'
functionOnly='Dropped by lcov for having function records but no line records'

clean_controls() {
  reset
  run "a clean pair passes" 0 "1 of 4 captured files kept unchanged; 2 removed by pattern, 1 with no coverage points"
  run "a clean pair reports the figures" 0 "4 of 4 lines, 2 of 4 functions"
  run "a clean pair lists no function-only files" 0 \
      "Dropped by lcov for having function records but no line records: 0 files"

  reset; capture | awk '/^SF:\/src\/empty\.hpp$/{skip=1} !skip{print} skip && /^end_of_record$/{skip=0}' \
                   > "$tmp/capture"
  run "a capture of files which all have coverage points passes" 0 "0 with no coverage points"
  listed "an empty listing keeps its heading" "$noPoints: 0 files" "$functionOnly: 0 files"
}

dropped_controls() {
  reset
  printf '%s\n' SF:/src/y.hpp LF:0 LH:0 end_of_record                           >> "$tmp/capture"
  printf '%s\n' SF:/usr/include/n.h LF:0 LH:0 end_of_record                     >> "$tmp/capture"
  printf '%s\n' SF:/src/d.hpp BRF:0 BRH:0 MCF:0 MCH:0 FNF:0 FNH:0 end_of_record >> "$tmp/capture"
  run "dropped files with no coverage points pass" 0 \
      "1 of 7 captured files kept unchanged; 3 removed by pattern, 3 with no coverage points"
  listed "dropped files with no coverage points are listed, sorted, under a count" \
         "$noPoints: 3 files" '  /src/d.hpp' '  /src/empty.hpp' '  /src/y.hpp' "$functionOnly: 0 files"

  reset
  printf '%s\n' SF:/src/z.hpp FNL:0,30 FNA:0,15,_ZN1zC2ERKS_ FNF:1 FNH:1 LF:0 LH:0 end_of_record >> "$tmp/capture"
  printf '%s\n' SF:/src/b.hpp FNL:0,12 FNA:0,0,_ZN1b1fEv     FNF:1 FNH:0 LF:0 LH:0 end_of_record >> "$tmp/capture"
  run "dropped function-only files pass" 0 "1 of 6 captured files kept unchanged"
  listed "dropped function-only files are listed, sorted, under a count" \
         "$noPoints: 1 files" '  /src/empty.hpp' "$functionOnly: 2 files" '  /src/b.hpp' '  /src/z.hpp'

  reset; printf '%s\n' SF:/src/cold.cpp DA:1,0 LF:1 LH:0 end_of_record >> "$tmp/capture"
  run "a dropped file with only unhit lines is named" 1 \
      '/src/cold.cpp has records besides function records, matches no removal pattern, and was dropped'

  reset
  printf '%s\n' SF:/src/c.hpp FNL:0,3,4 FNA:0,5,_ZN1c1gEv DA:3,5 DA:4,5 LF:2 LH:2 end_of_record >> "$tmp/capture"
  run "a dropped file with function and line records is named" 1 \
      '/src/c.hpp has records besides function records, matches no removal pattern, and was dropped'
}

record_controls() {
  reset; edit filtered 's/^FNA:1,0,/FNA:1,1,/'; edit summary 's/(2 of 4 functions)/(3 of 4 functions)/'
  run "a lambda marked called is named" 1 \
      'the capture has "FNA:1,0,_ZZ1gvENKUlvE_clEv" where the filtered tracefile has "FNA:1,1,'

  reset; edit filtered 's/^FNA:0,2,/FNA:0,0,/'; edit summary 's/(2 of 4 functions)/(1 of 4 functions)/'
  run "a called function marked uncalled is named" 1 'the capture has "FNA:0,2,_Z1fv"'

  reset; edit filtered 's/^DA:2,2$/DA:2,3/'
  run "a changed line count is named" 1 'the capture has "DA:2,2" where the filtered tracefile has "DA:2,3"'

  reset; edit filtered 's/^LH:4$/LH:4\
DA:9,0/'; edit summary 's/(4 of 4 lines)/(4 of 5 lines)/'
  run "a record the capture lacks is named" 1 \
      'the capture has "\(nothing\)" where the filtered tracefile has "DA:9,0"'

  reset; edit filtered '/^LH:4$/d'
  run "a lost record is named" 1 'the capture has "LH:4" where the filtered tracefile has "\(nothing\)"'

  reset; edit filtered $'s/^DA:2,2$/DA:2,2\r/'
  run "a carriage return the capture lacks is named" 1 \
      $'the capture has "DA:2,2" where the filtered tracefile has "DA:2,2\r"'

  reset
  capture | awk '/^SF:\/usr\/include\/x\.h$/{p=1} p{print} p && /^end_of_record$/{exit}' >> "$tmp/filtered"
  edit summary 's/(4 of 4 lines)/(5 of 5 lines)/'
  run "a kept file a pattern removes is named" 1 '/usr/include/x.h matches a removal pattern but was kept'

  reset; edit filtered 's|^SF:/src/a.cpp$|SF:/src/c.cpp|'
  run "a file absent from the capture is named" 1 '/src/c.cpp is in the filtered tracefile but not the capture'
}

pattern_controls() {
  reset; removed=('/usr/*' '/lib/v?/*')
  printf '%s\n' SF:/lib/v1/z.h DA:1,1 LF:1 LH:1 end_of_record >> "$tmp/capture"
  run "a file only the second pattern matches is removed" 0 \
      "1 of 5 captured files kept unchanged; 3 removed by pattern"

  reset; removed=('/usr/*' '/lib/v?/*')
  printf '%s\n' SF:/lib/v10/z.h DA:1,1 LF:1 LH:1 end_of_record >> "$tmp/capture"
  run "a ? matches exactly one character" 1 \
      '/lib/v10/z.h has records besides function records, matches no removal pattern'

  reset; removed=('/usr/*' '/opt/x.y/*')
  printf '%s\n' SF:/opt/xzy/w.h DA:1,1 LF:1 LH:1 end_of_record >> "$tmp/capture"
  run "a . matches only itself" 1 '/opt/xzy/w.h has records besides function records, matches no removal pattern'

  reset; printf '%s\n' SF:/USR/include/q.h DA:1,1 LF:1 LH:1 end_of_record >> "$tmp/capture"
  run "a pattern matches letters of its own case only" 1 \
      '/USR/include/q.h has records besides function records, matches no removal pattern'
}

summary_controls() {
  reset; edit summary 's/(2 of 4 functions)/(2 of 3 functions)/'
  run "a function figure the records disagree with is named" 1 \
      'reports 2 of 3 functions hit, but the filtered tracefile has 2 of 4 FNA records'

  reset; edit summary 's/(4 of 4 lines)/(3 of 4 lines)/'
  run "a line figure the records disagree with is named" 1 \
      'reports 3 of 4 lines hit, but the filtered tracefile has 4 of 4 DA records'

  reset; edit summary '/functions\./d'
  run "a summary with no function figure is named" 1 'the summary has no functions figure'

  reset; edit summary '/lines\./d'
  run "a summary with no line figure is named" 1 'the summary has no lines figure'
}

malformed_controls() {
  reset; filtered >> "$tmp/capture"
  run "a second record for one file is named" 1 'capture:35: /src/a.cpp has a second record'

  reset; edit filtered '/^end_of_record$/d'
  run "a record with no end is named" 1 'filtered: the record for /src/a.cpp has no end_of_record'

  reset; edit capture '18d'
  run "a record which the next file's record interrupts is named" 1 \
      'capture:18: the record for /src/a.cpp has no end_of_record'

  reset; printf 'end_of_record\n' >> "$tmp/filtered"
  run "an end of record outside any file is named" 1 'filtered:18: "end_of_record" is outside any file'

  reset; printf 'TN:\n' > "$tmp/filtered"
  run "a tracefile with no records is named" 1 'filtered has no file records'

  reset; edit filtered '1i\
DA:1,1'
  run "a record outside any file is named" 1 'filtered:1: "DA:1,1" is outside any file'

  reset; printf '\n' >> "$tmp/capture"
  run "a blank line outside any file passes" 0 "1 of 4 captured files kept unchanged"

  reset
  invoke "a capture which cannot be read is named" 1 "^error: .*$tmp/absent" \
         --capture "$tmp/absent" --filtered "$tmp/filtered" --summary "$tmp/summary" --removed '/usr/*'
  invoke "a summary which cannot be read is named" 1 "^error: .*$tmp/absent" \
         --capture "$tmp/capture" --filtered "$tmp/filtered" --summary "$tmp/absent" --removed '/usr/*'
}

option_controls() {
  local capture=(--capture "$tmp/capture") filtered=(--filtered "$tmp/filtered")
  local summary=(--summary "$tmp/summary")  patterns=(--removed '/usr/*')
  reset
  invoke "--capture is required" 2 "the following arguments are required: --capture" \
         "${filtered[@]}" "${summary[@]}" "${patterns[@]}"
  invoke "--filtered is required" 2 "the following arguments are required: --filtered" \
         "${capture[@]}" "${summary[@]}" "${patterns[@]}"
  invoke "--summary is required" 2 "the following arguments are required: --summary" \
         "${capture[@]}" "${filtered[@]}" "${patterns[@]}"
  invoke "--removed is required" 2 "the following arguments are required: --removed" \
         "${capture[@]}" "${filtered[@]}" "${summary[@]}"
  invoke "--removed needs a pattern" 2 "--removed: expected at least one argument" \
         "${capture[@]}" "${filtered[@]}" "${summary[@]}" --removed
}

encoding_controls() {
  # lcov writes a function name's letters from U+0080 to U+00FF as single
  # Latin-1 bytes, so a tracefile need not be UTF-8. Here is sequoia's
  # café_free_test, as lcov 2.5 writes the name gcc 16 mangles.
  local nonUtf8Symbol=$'_ZN15caf\xe9_free_test3runEv'
  reset
  LC_ALL=C edit capture  "s/_Z1fv/$nonUtf8Symbol/"
  LC_ALL=C edit filtered "s/_Z1fv/$nonUtf8Symbol/"
  run "a symbol which is not UTF-8 passes" 0 "1 of 4 captured files kept unchanged"
  LC_ALL=C edit filtered 's/^FNA:0,2,/FNA:0,0,/'; edit summary 's/(2 of 4 functions)/(1 of 4 functions)/'
  run "a symbol which is not UTF-8 is quoted as itself when its count changes" 1 \
      "the capture has \"FNA:0,2,$nonUtf8Symbol\""

  reset
  LC_ALL=C edit capture  "s/_Z1fv/$nonUtf8Symbol/"
  LC_ALL=C edit filtered $'s/_Z1fv/_ZN15caf\xe8_free_test3runEv/'
  run "a byte which is not UTF-8, changed, fails" 1 "the capture has \"FNA:0,2,$nonUtf8Symbol\""

  reset
  printf '%s\n' $'SF:/src/caf\xe9.hpp' FNL:0,12 FNA:0,0,_ZN1b1fEv FNF:1 FNH:0 LF:0 LH:0 end_of_record \
                >> "$tmp/capture"
  run "a dropped file whose path is not UTF-8 passes" 0 "1 of 5 captured files kept unchanged"
  listed "a dropped file whose path is not UTF-8 is listed as itself" \
         "$noPoints: 1 files" '  /src/empty.hpp' "$functionOnly: 1 files" $'  /src/caf\xe9.hpp'

  reset; LC_ALL=C edit summary $'s/^Reading tracefile filtered\\.$/Reading tracefile caf\xe9.info./'
  run "a summary which is not UTF-8 is read" 0 "1 of 4 captured files kept unchanged"
}

run_controls() { # run_controls <script>: sets fails, and total, the controls run
  subject=$1 fails=0 total=0
  out=$(mktemp "$tmp/out.XXXXXX")
  clean_controls
  dropped_controls
  record_controls
  pattern_controls
  summary_controls
  malformed_controls
  option_controls
  encoding_controls
}

# Each mutant breaks one behaviour, and is (description, old text, new text).
mutants() { # mutants <dir>: writes <dir>/<n>/check_tracefile.py, and
            # prints a line "<n> <occurrences of the old text> <description>"
  python3 - "$script" "$1" <<'EOF'
import os, sys
TOTALS    = "TOTAL_RECORDS    = {'FNF', 'FNH', 'LF', 'LH', 'BRF', 'BRH', 'MCF', 'MCH'}"
FUNCTIONS = "FUNCTION_RECORDS = {'FNL', 'FNA'}"
MUTATIONS = [
    *((f'{total} not a total', TOTALS, TOTALS.replace(f"'{total}'", "'XX'"))
      for total in ('FNF', 'FNH', 'LF', 'LH', 'BRF', 'BRH', 'MCF', 'MCH')),
    ('DA a total',                       TOTALS,    TOTALS.replace("'FNF'", "'DA', 'FNF'")),
    ('FNL not a function record',        FUNCTIONS, FUNCTIONS.replace("'FNL', ", "")),
    ('FNA not a function record',        FUNCTIONS, FUNCTIONS.replace(", 'FNA'", "")),
    ('DA a function record',             FUNCTIONS, FUNCTIONS.replace("'FNA'", "'FNA', 'DA'")),
    ('a tag ends at a comma',            "line.split(':', 1)[0]", "line.split(',', 1)[0]"),
    ('* literal',                        "'.*' if c == '*'", "re.escape(c) if c == '*'"),
    ('? literal',                        "'.' if c == '?'", "re.escape(c) if c == '?'"),
    ('? any run of characters',          "'.' if c == '?'", "'.*' if c == '?'"),
    ('other characters as regex',        "else re.escape(c) for c in glob", "else c for c in glob"),
    ('patterns ignore case',             "for c in glob))", "for c in glob), re.IGNORECASE)"),
    ('patterns anchored at the start',   "pattern.search(source)", "pattern.match(source)"),
    ('every pattern must match',         "any(pattern.search", "all(pattern.search"),
    ('only the first pattern read',      "for glob in arguments.removed]", "for glob in arguments.removed[:1]]"),
    ('a second record accepted',         "if source in records:", "if False:"),
    ('end_of_record ignored',            "elif line == 'end_of_record':\n                source = None",
                                         "elif line == 'end_of_record':\n                pass"),
    ('a stray line accepted',            "elif line and not line.startswith('TN:'):", "elif False:"),
    ('a stray end of record accepted',   "elif line and not line.startswith('TN:'):",
                                         "elif line and not line.startswith('TN:') and line != 'end_of_record':"),
    ('a test name refused',              "elif line and not line.startswith('TN:'):", "elif line:"),
    ('a blank line refused',             "elif line and not line.startswith('TN:'):",
                                         "elif not line.startswith('TN:'):"),
    ('an open record accepted',          "    if source is not None:\n        raise", "    if False:\n        raise"),
    ('an interrupted record accepted',   "elif line.startswith('SF:'):", "elif False:"),
    ('no records accepted',              "if not records:", "if False:"),
    ('line numbers from 0',              "enumerate(tracefile, 1)", "enumerate(tracefile)"),
    ('a tracefile read as UTF-8',        "encoding='utf-8', errors='surrogateescape', newline=''",
                                         "encoding='utf-8', newline=''"),
    ('line endings translated',          "errors='surrogateescape', newline='') as tracefile",
                                         "errors='surrogateescape') as tracefile"),
    ('trailing whitespace stripped',     "line = line.rstrip('\\n')", "line = line.rstrip()"),
    ('the summary read as UTF-8',        "open(arguments.summary, encoding='utf-8', errors='surrogateescape')",
                                         "open(arguments.summary, encoding='utf-8')"),
    ('output written strictly',          "stream.reconfigure(encoding='utf-8', errors='surrogateescape')",
                                         "pass"),
    ('a kept file need not be captured', "if source not in captured:", "if False:"),
    ('a kept file may match a pattern',  "        if removed_by(source, patterns):\n            raise",
                                         "        if False:\n            raise"),
    ('records compared to the shorter',  "zip_longest(captured[source], lines, fillvalue='(nothing)')",
                                         "zip(captured[source], lines)"),
    ('a changed record accepted',        "if in_capture != in_filtered:", "if False:"),
    ('removals by pattern uncounted',    "by_pattern += 1", "pass"),
    ('a pattern second to no points',    "        if removed_by(source, patterns):\n            by_pattern += 1",
                                         "        if removed_by(source, patterns) and tags:"
                                         "\n            by_pattern += 1"),
    ('a file without points refused',    "elif not tags:", "elif False:"),
    ('a function-only file refused',     "elif tags <= FUNCTION_RECORDS:", "elif False:"),
    ('any dropped file accepted',        "elif tags <= FUNCTION_RECORDS:", "elif True:"),
    ('without points unsorted',          "sorted(without_points)", "without_points"),
    ('function-only unsorted',           "sorted(function_only)", "function_only"),
    ('a figure only on the first line',  "re.MULTILINE)", "0)"),
    ('hit and found swapped',            "int(match.group(1)), int(match.group(2))",
                                         "int(match.group(2)), int(match.group(1))"),
    ('every record hit',                 "int(line.split(',')[1]) != 0", "1"),
    ('the first field the count',        "int(line.split(',')[1]) != 0", "int(line.split(',')[0]) != 0"),
    ('functions counted by FNL',         "('functions', 'FNA')", "('functions', 'FNL')"),
    ('lines unchecked',                  "(('lines', 'DA'), ('functions', 'FNA'))", "(('functions', 'FNA'),)"),
    ('a figure mismatch accepted',       "if reported != counted:", "if False:"),
    ('the capture summarised',           "check_summary(summary.read(), filtered)",
                                         "check_summary(summary.read(), captured)"),
    ('an unreadable file a traceback',   "except (Failure, OSError) as error:", "except Failure as error:"),
    ('a failure exits 0',                "        return 1\n", "        return 0\n"),
    ('--capture optional',               "'--capture',  required=True", "'--capture',  required=False"),
    ('--filtered optional',              "'--filtered', required=True", "'--filtered', required=False"),
    ('--summary optional',               "'--summary',  required=True", "'--summary',  required=False"),
    ('--removed optional',               "'--removed',  required=True", "'--removed',  required=False"),
    ('--removed may be empty',           "nargs='+'", "nargs='*'"),
    ('an empty heading omitted',         "        print(f'Dropped by lcov",
                                         "        if sources: print(f'Dropped by lcov"),
    ('the headings swapped',             "(('no coverage points',", "(('function records but no line records',"),
]
script, directory = sys.argv[1:]
with open(script, encoding='utf-8') as f:
    source = f.read()
for n, (description, old, new) in enumerate(MUTATIONS):
    os.makedirs(os.path.join(directory, str(n)))
    with open(os.path.join(directory, str(n), 'check_tracefile.py'), 'w', encoding='utf-8') as f:
        f.write(source.replace(old, new))
    print(n, source.count(old), description)
EOF
}

if [[ $# -eq 1 ]]; then
  verbose=no survivors=0 count=0
  run_controls "$script"
  echo "unmutated: $fails of $total controls fail$( ((fails)) && echo '  <-- must be none')"
  if ((fails)); then survivors=1; fi
  mutants "$tmp/mutants" > "$tmp/mutants.txt" || exit 1
  while read -r n occurrences description; do
    if [[ $occurrences -ne 1 ]]; then
      echo "$description: the text to mutate occurs $occurrences times"; survivors=1; continue
    fi
    run_controls "$tmp/mutants/$n/check_tracefile.py"; count=$((count+1))
    echo "$description: $fails of $total controls fail$( ((fails)) || echo '  <-- SURVIVED')"
    if ((fails == 0)); then survivors=1; fi
  done < "$tmp/mutants.txt"
  if ((survivors || count == 0)); then exit 1; fi
  echo "check_tracefile.py: $count mutants, every one killed"
  exit 0
fi

run_controls "$script"
if [[ $fails -eq 0 ]]; then echo "check_tracefile.py: all $total controls pass"; else
  echo "check_tracefile.py: $fails of $total controls failed"; fi
exit $((fails > 0))
