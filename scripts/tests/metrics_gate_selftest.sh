#!/bin/bash
# Controls for metrics_gate.py, against records built here, and a mutation
# check of the controls.
#
#   metrics_gate_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the script and against each mutant of the script.
# The script must fail no control, and each mutant at least one.
#
# The baseline's counts are chosen so that each series' allowance differs
# from the others', and from what any other denominator would give. Each
# control is a claim the script exists to keep:
#   - against-baseline passes records which agree, and a rise of each series
#     up to its allowance: 1 in 200 of the baseline's count for the symbols
#     and for their names' bytes, 1 in 20 for prune's instructions; one more
#     fails, naming the series, and so does that rise in each series alone;
#   - a fall never fails;
#   - a count absent from either record fails, naming the series, as does a
#     count which is zero, a boolean, a fraction or text;
#   - records differing in their toolchain, cache or target, or in the
#     functions which both count in prune's analysis, are a break: the break
#     fails, naming the field, and no series is compared; the acceptance of a
#     break passes it; a section which one record lacks is no break;
#   - a record made from a tree which was not clean fails, whichever it is;
#   - an acceptance raises its own series' allowance by its count and no
#     other series'; the line a failure prints names the whole rise, and,
#     put in the description, passes the same records; it is found among
#     other lines, indented, and with a carriage return before its line
#     feed;
#   - a line starting `metrics-gate:` which is no acceptance fails, as do a
#     series accepted twice and a break accepted twice; a line mentioning it
#     after other text is prose;
#   - a failure of either symbol series lists the objects which moved, at
#     most 20, largest move first, and counts those which moved, were added
#     and were removed; a failure of prune's series alone, and a pass, list
#     none;
#   - leftover-materials passes up to 400 instructions per leftover entry,
#     and fails one more, a record lacking the field or a count, and a record
#     whose run with leftovers found no entries;
#   - a record which cannot be read as JSON, or holds no object, and an
#     absent description are refused with status 2, as are arguments of any
#     other form.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script=$here/../metrics_gate.py
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

case "$*" in
  ''|--mutations) ;;
  *) echo "Usage: $0 [--mutations]" >&2; exit 2;;
esac

# The baseline. Its allowances: symbols +1,000, bytes +2,000, prune +30,000.
jq -n '{
  commit: "base", clean: true, target: "TestAll",
  toolchain: {compiler: {id: "GNU", package: {version: "15.2.0-16ubuntu1"}}, standard_library: "libstdc++"},
  cache: {CMAKE_BUILD_TYPE: "Debug"},
  executable: {symbols: 200000, symbol_name_bytes: 400000},
  defined_symbols_by_object: {"a.cpp": 100, "b.cpp": 200, "c.cpp": 300, "gone.cpp": 7},
  prune_dependency_analysis: {instructions: 600000, function: "prune*", cpu_model: "EPYC"},
  leftover_materials_setup: {leftover_entries: 10,
                             without_leftovers: {instructions: 50000}, with_leftovers: {instructions: 54000}}
}' > "$tmp/base.json"

# measured <name> <jq filter>: the baseline, as measured at "head", changed by
# the filter, in $tmp/<name>.json
measured() { jq '.commit = "head" | '"$2" "$tmp/base.json" > "$tmp/$1.json"; }

measured same           '.'
measured symbols_at     '.executable.symbols += 1000'
measured symbols_over   '.executable.symbols += 1001'
measured symbols_far    '.executable.symbols += 3000'
measured bytes_at       '.executable.symbol_name_bytes += 2000'
measured bytes_over     '.executable.symbol_name_bytes += 2001'
measured prune_at       '.prune_dependency_analysis.instructions += 30000'
measured prune_over     '.prune_dependency_analysis.instructions += 30001'
measured falls          '.executable.symbols -= 50000 | .executable.symbol_name_bytes -= 90000
                         | .prune_dependency_analysis.instructions -= 300000'
measured all_over       '.executable.symbols += 1001 | .executable.symbol_name_bytes += 2001
                         | .prune_dependency_analysis.instructions += 30001'
measured movers         '.executable.symbols += 1500
                         | .defined_symbols_by_object = {"a.cpp": 90, "b.cpp": 260, "c.cpp": 300, "new.cpp": 40}'
measured compiler       '.toolchain.compiler.package.version = "15.2.0-17ubuntu1" | .executable.symbols += 9999'
measured cache          '.cache.CMAKE_BUILD_TYPE = "Release"'
measured target         '.target = "TestChamber"'
measured toolchain_gone 'del(.toolchain)'
measured function       '.prune_dependency_analysis.function = "analyse*"'
measured unclean        '.clean = false'
measured clean_gone     'del(.clean)'
measured bytes_movers   '.executable.symbol_name_bytes += 2001 | .defined_symbols_by_object["a.cpp"] += 1'
measured many_movers    '.executable.symbols += 1001
                         | .defined_symbols_by_object = ([range(25)] | map({key: "m\(. + 10).cpp", value: (. + 1)})
                                                         | from_entries)'
jq '.clean = false' "$tmp/base.json" > "$tmp/base_unclean.json"
jq 'del(.prune_dependency_analysis)' "$tmp/base.json" > "$tmp/base_without_analysis.json"
for series in executable.symbols executable.symbol_name_bytes prune_dependency_analysis.instructions; do
  name=${series#*.}
  measured "no_$name"       "del(.$series)"
  measured "boolean_$name"  ".$series = true"
  measured "fraction_$name" ".$series += 0.5"
  measured "text_$name"     ".$series = \"7\""
  measured "zero_$name"     ".$series = 0"
  jq "del(.$series)" "$tmp/base.json" > "$tmp/base_without_$name.json"
done
measured no_analysis 'del(.prune_dependency_analysis)'

# The setup cost of leftover materials: 4,000 instructions for 10 entries in
# the baseline, 400 per entry at the limit.
measured leftover_at      '.leftover_materials_setup.with_leftovers.instructions = 54000'
measured leftover_over    '.leftover_materials_setup.with_leftovers.instructions = 54001'
measured leftover_cheaper '.leftover_materials_setup.with_leftovers.instructions = 49000'
measured leftover_none    'del(.leftover_materials_setup)'
measured leftover_empty   '.leftover_materials_setup.leftover_entries = 0'
measured leftover_no_with 'del(.leftover_materials_setup.with_leftovers.instructions)'
measured leftover_no_without 'del(.leftover_materials_setup.without_leftovers.instructions)'
measured leftover_no_entries 'del(.leftover_materials_setup.leftover_entries)'

: > "$tmp/nothing.txt"
echo 'not json' > "$tmp/garbage.json"
echo '[1, 2]' > "$tmp/array.json"

# description <name> <line>...: the lines, as a description, in $tmp/<name>.txt
description() { local name=$1; shift; printf '%s\n' "$@" > "$tmp/$name.txt"; }

description accept_symbols_2000  'metrics-gate: accept symbols +2000'
description accept_symbols_1999  'metrics-gate: accept symbols +1999'
description accept_bytes         'metrics-gate: accept symbol_name_bytes +3000'
description accept_break         'Some prose first.' 'metrics-gate: accept a break in the series'
description in_prose             'The gate takes a line such as metrics-gate: accept symbols +5000.'
description twice                'metrics-gate: accept symbols +2000' 'metrics-gate: accept symbols +2000'
description break_twice          'metrics-gate: accept a break in the series' 'metrics-gate: accept a break in the series'
printf 'Prose.\r\n   metrics-gate: accept symbols +1001  \r\nMore prose.\r\n' > "$tmp/crlf.txt"
malformed=(
  'metrics-gate: accept symbols 2345'
  'metrics-gate: accept symbols +01001'
  'metrics-gate: accept symbols +-1'
  'metrics-gate: accept symbols +'
  'metrics-gate: accept symbols +1.5'
  'metrics-gate: accept symbols +١٠٠١'
  'metrics-gate: accept symbols +1001 please'
  'metrics-gate: accept  symbols +1001'
  'metrics-gate:accept symbols +1001'
  'metrics-gate: accepts symbols +1001'
  'metrics-gate: accept runner_setup +1001'
  'metrics-gate: accept a break'
  'metrics-gate: accept a break in the series please'
)

fail() { echo "FAIL: $1"; fails=$((fails+1)); }

# run <argument>...: sets $status, $out and $err
run() {
  python3 -B "$under_test" "$@" > "$tmp/out" 2> "$tmp/err"
  status=$?
  out=$(cat "$tmp/out")
  err=$(cat "$tmp/err")
}

# compare <measured> [<description>]: against-baseline, the measured record
# against the baseline, with the description, or none
compare() { run against-baseline "$tmp/$1.json" "$tmp/base.json" "$tmp/${2:-nothing}.txt"; }

# passes <description> <measured> [<description file>]: status 0 and no error
passes() {
  local description=$1; shift
  compare "$@"
  [ "$status" -eq 0 ] && [[ $out != *'error: '* ]] && [[ $out == *'notice: '* ]] \
    || fail "$description: status $status, printed '$out' '$err'"
}

# fails_with <description> <expected text> <measured> [<description file>]:
# status 1 and an error holding the text
fails_with() {
  local description=$1 expected=$2; shift 2
  compare "$@"
  [ "$status" -eq 1 ] && grep -qF -- "error: $expected" <<< "$out" \
    || fail "$description: status $status, printed '$out' '$err'"
}

# leftover <description> <expected status> <expected text> <record>
leftover() {
  run leftover-materials "$tmp/$4.json"
  [ "$status" -eq "$2" ] && grep -qF -- "$3" <<< "$out" \
    || fail "$1: status $status, printed '$out' '$err'"
}

# refused <description> <argument>...: status 2, nothing on standard output
refused() {
  local description=$1; shift
  run "$@"
  [ "$status" -eq 2 ] && [ -z "$out" ] && [ -n "$err" ] \
    || fail "$description: status $status, printed '$out' '$err'"
}

# controls <script>
# Runs the controls against <script>, and counts in $fails those that fail.
controls() {
  fails=0
  under_test=$1

  passes "records which agree" same
  compare same
  local allowance
  for allowance in \
      'notice: symbols: 200000 -> 200000, +0 (+0.00%), allowed +1000 (1 in 200 of the baseline)' \
      'notice: symbol_name_bytes: 400000 -> 400000, +0 (+0.00%), allowed +2000 (1 in 200 of the baseline)' \
      'notice: prune_dependency_analysis: 600000 -> 600000, +0 (+0.00%), allowed +30000 (1 in 20 of the baseline)'; do
    grep -qxF "$allowance" <<< "$out" || fail "records which agree: no line '$allowance' in '$out'"
  done
  passes "a rise of the symbols at their allowance" symbols_at
  fails_with "a rise of the symbols beyond their allowance" "symbols: 200000 -> 201001" symbols_over
  grep -qxF 'error:   metrics-gate: accept symbols +1001' <<< "$out" \
    || fail "a rise of the symbols beyond their allowance: the line accepting the whole rise is not printed: '$out'"
  passes "a rise of the bytes at their allowance" bytes_at
  fails_with "a rise of the bytes beyond their allowance" "symbol_name_bytes: 400000 -> 402001" bytes_over
  passes "a rise of prune's count at its allowance" prune_at
  fails_with "a rise of prune's count beyond its allowance" "prune_dependency_analysis: 600000 -> 630001" prune_over
  passes "falls in every series" falls
  compare all_over
  [ "$status" -eq 1 ] && [ "$(grep -c '^error: [a-z_]*: [0-9]* -> ' <<< "$out")" -eq 3 ] \
    || fail "every series beyond its allowance: status $status, printed '$out'"

  for name in symbols symbol_name_bytes instructions; do
    series=$name; [ "$name" = instructions ] && series=prune_dependency_analysis
    fails_with "the measured record without $series" "$series: the measured record" "no_$name"
    fails_with "a boolean for $series" "$series: the measured record" "boolean_$name"
    fails_with "a fraction for $series" "$series: the measured record" "fraction_$name"
    fails_with "text for $series" "$series: the measured record" "text_$name"
    fails_with "zero for $series" "$series: the measured record" "zero_$name"
    run against-baseline "$tmp/same.json" "$tmp/base_without_$name.json" "$tmp/nothing.txt"
    [ "$status" -eq 1 ] && grep -qF "error: $series: the baseline's record, of base, has no count" <<< "$out" \
      || fail "the baseline without $series: status $status, printed '$out'"
  done
  fails_with "the measured record without prune's section" "prune_dependency_analysis: the measured record" no_analysis

  fails_with "a compiler which differs" "a break in the series: the records differ in toolchain.compiler.package.version," compiler
  [[ $out != *'symbols:'* ]] || fail "a compiler which differs: a series was compared: '$out'"
  grep -qxF "error:   metrics-gate: accept a break in the series" <<< "$out" \
    || fail "a compiler which differs: the line accepting a break is not printed: '$out'"
  fails_with "a cache which differs" "a break in the series: the records differ in cache.CMAKE_BUILD_TYPE," cache
  fails_with "a target which differs" "a break in the series: the records differ in target," target
  fails_with "a toolchain absent" "a break in the series: the records differ in toolchain," toolchain_gone
  fails_with "prune's functions which differ" "a break in the series: the records differ in prune_dependency_analysis.function," function
  run against-baseline "$tmp/same.json" "$tmp/base_without_analysis.json" "$tmp/nothing.txt"
  [ "$status" -eq 1 ] && [[ $out != *'a break'* ]] && grep -qF "error: prune_dependency_analysis: the baseline's record, of base, has no count" <<< "$out" \
    || fail "a baseline without prune's section: a break, or no failure of the series: status $status, printed '$out'"
  fails_with "a measured record made from a tree not clean" "the measured record, of head, was made from a tree which was not clean" unclean
  fails_with "a measured record which does not say it is clean" "the measured record, of head, was made from a tree which was not clean" clean_gone
  run against-baseline "$tmp/same.json" "$tmp/base_unclean.json" "$tmp/nothing.txt"
  [ "$status" -eq 1 ] && grep -qF "error: the baseline's record, of base, was made from a tree which was not clean" <<< "$out" \
    || fail "a baseline made from a tree not clean: status $status, printed '$out'"
  passes "an accepted break" compiler accept_break
  [[ $out == *'notice: a break in the series'* && $out != *'symbols:'* ]] \
    || fail "an accepted break: the break is not noticed, or a series was compared: '$out'"

  passes "a rise of the symbols at their allowance and its acceptance" symbols_far accept_symbols_2000
  [[ $out == *'and +2000 accepted'* ]] || fail "an accepted rise: the acceptance is not reported: '$out'"
  fails_with "a rise of the symbols beyond their allowance and its acceptance" "symbols: 200000 -> 203000" \
             symbols_far accept_symbols_1999
  fails_with "an acceptance of the bytes, for the symbols" "symbols: 200000 -> 203000" symbols_far accept_bytes
  passes "an acceptance indented, with carriage returns" symbols_over crlf
  fails_with "an acceptance in prose, not applied" "symbols: 200000 -> 201001" symbols_over in_prose
  passes "an acceptance in prose, not refused" symbols_at in_prose
  fails_with "a series accepted twice" "symbols is accepted twice" symbols_over twice
  fails_with "a break accepted twice" "a break in the series is accepted twice" compiler break_twice
  local line
  for line in "${malformed[@]}"; do
    description malformed "$line"
    fails_with "the line '$line'" "the line " same malformed
  done
  compare all_over
  sed -n 's/^error:   //p' <<< "$out" > "$tmp/printed.txt"
  [ "$(wc -l < "$tmp/printed.txt")" -eq 3 ] || fail "every series beyond its allowance: not three lines to accept them: '$out'"
  passes "the lines a failure prints, as a description" all_over printed
  compare compiler
  sed -n 's/^error:   metrics-gate/metrics-gate/p' <<< "$out" > "$tmp/printed.txt"
  passes "the line a break prints, as a description" compiler printed

  compare movers
  [ "$status" -eq 1 ] && [ "$(sed -n '/objects moved/,$p' <<< "$out")" = "notice: 4 of 5 objects moved (1 added, 1 removed); the 4 largest moves:
notice:   +60 b.cpp
notice:   +40 new.cpp
notice:   -10 a.cpp
notice:   -7 gone.cpp" ] || fail "a failing symbol series: the movers are not listed as predicted: '$out'"
  compare bytes_movers
  [ "$status" -eq 1 ] && [ "$(sed -n '/objects moved/,$p' <<< "$out")" = "notice: 1 of 4 objects moved (0 added, 0 removed); the 1 largest moves:
notice:   +1 a.cpp" ] || fail "a failure of the bytes alone: the movers are not listed as predicted: '$out'"
  compare many_movers
  [ "$status" -eq 1 ] && grep -qxF 'notice: 29 of 29 objects moved (25 added, 4 removed); the 20 largest moves:' <<< "$out" \
    && [ "$(grep -c '^notice:   [+-]' <<< "$out")" -eq 20 ] && grep -qxF 'notice:   -300 c.cpp' <<< "$out" \
    && grep -qxF 'notice:   +9 m18.cpp' <<< "$out" && ! grep -qF 'm17.cpp' <<< "$out" \
    || fail "more movers than are listed: not the 20 largest: '$out'"
  compare prune_over
  [[ $out != *'objects moved'* ]] || fail "a failure of prune alone lists movers: '$out'"
  compare symbols_at
  [[ $out != *'objects moved'* ]] || fail "a pass lists movers: '$out'"

  leftover "leftover materials at the limit" 0 'notice: leftover materials cost the runner'"'"'s setup 4000 instructions for 10 entries, 400.0 per entry, on' leftover_at
  leftover "leftover materials beyond the limit" 1 'error: leftover materials cost the runner'"'"'s setup 4001 instructions' leftover_over
  leftover "leftover materials cheaper than none" 0 'notice: leftover materials cost the runner'"'"'s setup -1000 instructions' leftover_cheaper
  leftover "no leftover materials section" 1 'error: the record of head has no count' leftover_none
  leftover "no instructions with leftovers" 1 'error: the record of head has no count' leftover_no_with
  leftover "no instructions without leftovers" 1 'error: the record of head has no count' leftover_no_without
  leftover "no count of leftover entries" 1 'error: the record of head has no count' leftover_no_entries
  leftover "no leftover entries" 1 'error: the run with leftover materials found no leftover entries' leftover_empty

  refused "a measured record which is not JSON" against-baseline "$tmp/garbage.json" "$tmp/base.json" "$tmp/nothing.txt"
  refused "a baseline which is not JSON" against-baseline "$tmp/same.json" "$tmp/garbage.json" "$tmp/nothing.txt"
  refused "a baseline which holds no object" against-baseline "$tmp/same.json" "$tmp/array.json" "$tmp/nothing.txt"
  refused "an absent measured record" against-baseline "$tmp/absent.json" "$tmp/base.json" "$tmp/nothing.txt"
  refused "an absent description" against-baseline "$tmp/same.json" "$tmp/base.json" "$tmp/absent.txt"
  refused "a record for leftover materials which is not JSON" leftover-materials "$tmp/garbage.json"
  refused "no arguments"
  refused "an unknown mode" compare "$tmp/same.json" "$tmp/base.json" "$tmp/nothing.txt"
  refused "against-baseline without a description" against-baseline "$tmp/same.json" "$tmp/base.json"
  refused "leftover-materials with two records" leftover-materials "$tmp/same.json" "$tmp/base.json"
}

# Each mutant breaks one behaviour that the controls claim. Its entry holds a
# description, the text it replaces, and the replacement.
mutations=(
  'symbols at 1 in 100'              '("executable", "symbols"),                    200)' '("executable", "symbols"),                    100)'
  'bytes at 1 in 100'                '("executable", "symbol_name_bytes"),          200)' '("executable", "symbol_name_bytes"),          100)'
  'prune at 1 in 10'                 '("prune_dependency_analysis", "instructions"), 20)' '("prune_dependency_analysis", "instructions"), 10)'
  'prune not compared'               '    ("prune_dependency_analysis", ("prune_dependency_analysis", "instructions"), 20),
' ''
  'the allowance itself fails'       'if rise <= held // denominator + accepted:' 'if rise < held // denominator + accepted:'
  'a fall fails'                     'if rise <= held // denominator + accepted:' 'if abs(rise) <= held // denominator + accepted:'
  'an acceptance not applied'        'if rise <= held // denominator + accepted:' 'if rise <= held // denominator:'
  'an acceptance applied to every series' 'acceptances.accepted_rises.get(name, 0))' 'max(acceptances.accepted_rises.values(), default=0))'
  'an absent baseline count is zero' 'if held is None:' 'if held is None and False:'
  'an absent measured count passes'  '        return [f"error: {name}: the measured record, of {measured.get('"'"'commit'"'"')}, has no count: "
                "its measurement failed"]' '        return []'
  'a boolean is a count'             'and not isinstance(value, bool) and value > 0' 'and value > 0'
  'a fraction is a count'            'isinstance(value, int) and not isinstance(value, bool)' 'isinstance(value, (int, float)) and not isinstance(value, bool)'
  'zero is a count'                  'and not isinstance(value, bool) and value > 0' 'and not isinstance(value, bool) and value >= 0'
  'the toolchain not compared'       'MEASUREMENT_CONDITIONS = (("toolchain",), ("cache",), ("target",))' 'MEASUREMENT_CONDITIONS = (("cache",), ("target",))'
  'the cache not compared'           'MEASUREMENT_CONDITIONS = (("toolchain",), ("cache",), ("target",))' 'MEASUREMENT_CONDITIONS = (("toolchain",), ("target",))'
  'the target not compared'          'MEASUREMENT_CONDITIONS = (("toolchain",), ("cache",), ("target",))' 'MEASUREMENT_CONDITIONS = (("toolchain",), ("cache",))'
  "prune's functions not compared"   'SECTION_CONDITIONS = (("prune_dependency_analysis", "function"),)' 'SECTION_CONDITIONS = ()'
  'an absent section a break'        'if None not in (value_at(measured, path), value_at(baseline, path))
                 and value_at' 'if value_at'
  'only the top of a field compared' 'return [difference for key in sorted(measured.keys() | baseline.keys())' 'return [] if measured.keys() == baseline.keys() else [difference for key in sorted(measured.keys() | baseline.keys())'
  'a break still compares series'    '                + (findings_of_break(broken, measured, baseline, acceptances.accepts_break) if broken
                   else findings_of_all_series(measured, baseline, acceptances)))' '                + (findings_of_break(broken, measured, baseline, acceptances.accepts_break) if broken
                   else []) + findings_of_all_series(measured, baseline, acceptances))'
  'a break passes'                   'kind = "notice" if accepts_break else "error"' 'kind = "notice"'
  'an accepted break still fails'    'kind = "notice" if accepts_break else "error"' 'kind = "error"'
  'an unclean tree passes'           '               if record.get("clean") is not True]' '               if record.get("clean") is False]'
  'only the measured tree checked'   'for role, record in (("measured", measured), ("baseline'"'"'s", baseline))' 'for role, record in (("measured", measured),)'
  'acceptances read unstripped'      'for line in (line.strip() for line in lines):' 'for line in lines:'
  'an acceptance found anywhere'     'if not line.startswith(ACCEPTANCE_PREFIX):' 'if ACCEPTANCE_PREFIX not in line:'
  'the break line as a prefix'       'if line == BREAK_ACCEPTANCE_LINE:' 'if line.startswith(BREAK_ACCEPTANCE_LINE):'
  'a break accepted twice'           '            if accepts_break:
                errors.append' '            if False:
                errors.append'
  'a malformed line passed over'     '            errors.append(f"the line {line!r}' '            continue; errors.append(f"the line {line!r}'
  'a malformed line passes'          'findings = ([f"error: {e}" for e in acceptances.errors]' 'findings = ([f"notice: {e}" for e in acceptances.errors]'
  'no plus sign needed'              'or not words[3].startswith("+")' 'or False'
  'a leading zero accepted'          ' or words[3][1] == "0"):' '):'
  'any decimal digit accepted'       'or not words[3][1:].isascii()' 'or False'
  'extra words accepted'             'len(words) != 4 or' 'len(words) < 4 or'
  'any verb accepted'                'words[:2] != ["", "accept"]' 'words[:1] != [""]'
  'any series accepted'              'or words[2] not in names' 'or False'
  'an acceptance twice accepted'     'elif words[2] in accepted_rises:' 'elif False:'
  'the excess printed, not the rise' 'acceptance_line(name, rise)}' 'acceptance_line(name, rise - held // denominator)}'
  'the break line not printed'       '"then re-run the failed jobs:", f"error:   {BREAK_ACCEPTANCE_LINE}"]' '"then re-run the failed jobs:"]'
  'no movers'                        '+ (findings_of_movers(measured, baseline) if symbols_failed else []))' '+ [])'
  'movers on every failure'          'for name, path, _ in SERIES if path[0] == "executable"' 'for name, path, _ in SERIES'
  'movers for the symbols alone'     'for name, path, _ in SERIES if path[0] == "executable"' 'for name, path, _ in SERIES if name == "symbols"'
  'movers by name'                   'key=lambda move: (-abs(move[0]), move[1]))' 'key=lambda move: move[1])'
  'movers by signed size'            'key=lambda move: (-abs(move[0]), move[1]))' 'key=lambda move: (-move[0], move[1]))'
  'added objects not counted'        'added   = sum(source not in held for source in found)' 'added   = 0'
  'removed objects not counted'      'removed = sum(source not in found for source in held)' 'removed = 0'
  'every mover listed'               'MOVERS_LISTED_AT_MOST = 20' 'MOVERS_LISTED_AT_MOST = 100000'
  'one mover listed'                 'MOVERS_LISTED_AT_MOST = 20' 'MOVERS_LISTED_AT_MOST = 1'
  'leftover limit doubled'           'LEFTOVER_COST_LIMIT_PER_ENTRY = 400' 'LEFTOVER_COST_LIMIT_PER_ENTRY = 800'
  'the leftover limit fails'         'exceeded = cost > LEFTOVER_COST_LIMIT_PER_ENTRY * entries' 'exceeded = cost >= LEFTOVER_COST_LIMIT_PER_ENTRY * entries'
  'no leftover entries named'        '    if value_at(record, field + ("leftover_entries",)) == 0:' '    if False:'
  'an absent leftover count passes'  'if None in (entries, without, with_):' 'if entries is None:'
  'leftover per entry by the without run' 'cost = with_ - without' 'cost = with_ - without - without // 100'
  'an error exits 0'                 'return 1 if any(map(is_error, findings)) else 0' 'return 0'
  'leftover errors exit 0'           'return 1 if exceeded else 0' 'return 0'
  'a refusal exits 1'                '        print(e, file=sys.stderr)
        return 2' '        print(e, file=sys.stderr)
        return 1'
  'a non-object record read'         'if not isinstance(record, dict):' 'if False:'
  'any number of arguments'          'len(arguments) != modes[arguments[0]][1] + 1' 'len(arguments) < 2'
)

if [ "$*" != --mutations ]; then
  controls "$script"
  if [ "$fails" -eq 0 ]; then echo "metrics_gate: all controls pass"; else exit 1; fi
  exit 0
fi

source_text=$(cat "$script")
controls "$script" > "$tmp/controls.txt" 2>&1
echo "unmutated: $fails controls fail$([ "$fails" -eq 0 ] || echo '  <-- must be none')"
survivors=$fails
i=0
while [ "$i" -lt ${#mutations[@]} ]; do
  description=${mutations[i]} old=${mutations[i+1]} new=${mutations[i+2]}
  i=$((i+3))
  after=${source_text#*"$old"}
  if [ "$after" = "$source_text" ] || [ "${after#*"$old"}" != "$after" ]; then
    echo "$description: the text to mutate does not occur exactly once"
    survivors=$((survivors+1))
    continue
  fi
  printf '%s\n' "${source_text%%"$old"*}$new$after" > "$tmp/mutant.py"
  controls "$tmp/mutant.py" > "$tmp/controls.txt" 2>&1
  echo "$description: $fails controls fail$([ "$fails" -ne 0 ] || echo '  <-- SURVIVED')"
  [ "$fails" -ne 0 ] || survivors=$((survivors+1))
done
[ "$survivors" -eq 0 ]
