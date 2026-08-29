#!/bin/bash
# Controls for compile_time_report.py.
#
# The traces are generated here rather than checked in, so that a control can
# state the shape it is testing and nothing else. Each one is a claim the
# script got wrong at least once, or would be worthless without:
#
#   - self time must subtract children, or every enclosing phase reads as the
#     whole compile and explains nothing;
#   - a span with no children keeps its whole duration, which is the case that
#     matters: uninstrumented compiler work has a duration and no count;
#   - the by-time and by-count rankings must be able to disagree, because the
#     first version of this script ranked only by count and was blind to a
#     translation unit that had grown 8x slower without doing more of anything;
#   - an empty build directory must be an error, not a clean-looking report.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script="$here/../compile_time_report.py"
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
fails=0

check() { # check <name> <yes|no> <pattern>
  if grep -qE "$3" "$tmp/out"; then got=yes; else got=no; fi
  if [[ "$got" != "$2" ]]; then
    echo "FAIL: $1 (expected $2, got $got, pattern: $3)"; fails=$((fails+1))
  fi
}

trace() { # trace <file> <json-array-of-events>
  mkdir -p "$(dirname "$1")"
  printf '{"traceEvents": %s}\n' "$2" > "$1"
}

b="$tmp/build"

# Slow unit: one 10s span containing a 1s child, so self time is 9s to the
# parent and 1s to the child. Few events, so the count ranking ranks it last.
trace "$b/slow.cpp.json" '[
 {"ph":"X","name":"Total ExecuteCompiler","ts":0,"dur":10000000,"args":{"count":1}},
 {"ph":"X","name":"Total InstantiateFunction","ts":0,"dur":10000000,"args":{"count":2}},
 {"ph":"X","name":"InstantiateFunction","ts":0,"dur":10000000,"args":{"detail":"slow_outer"}},
 {"ph":"X","name":"InstantiateFunction","ts":100,"dur":1000000,"args":{"detail":"inner_child"}}]'

# Fast unit: a tenth the time, a hundred times the events. Ranked first by
# count and last by time - the disagreement the script must be able to show.
trace "$b/busy.cpp.json" '[
 {"ph":"X","name":"Total ExecuteCompiler","ts":0,"dur":1000000,"args":{"count":1}},
 {"ph":"X","name":"Total CheckConstraintSatisfaction","ts":0,"dur":900000,"args":{"count":500000}}]'

python3 "$script" "$b" --top 5 > "$tmp/out" 2>&1
check "both units found"            yes "2 translation units"
check "totals summed across units"  yes "500000"
check "time ranking leads with slow" yes "10\.00s  slow\.cpp"
check "count ranking leads with busy" yes "50000[0-9]  busy\.cpp"
# The orderings must actually differ, or the control proves nothing: assert the
# slow unit appears above the busy one under time and below it under count.
awk '/slowest translation units/,/heaviest translation units/' "$tmp/out" > "$tmp/bytime"
awk '/heaviest translation units/,/Note the two/'              "$tmp/out" > "$tmp/bycount"
grep -n 'slow\.cpp' "$tmp/bytime"  | cut -d: -f1 > "$tmp/a"
grep -n 'busy\.cpp' "$tmp/bytime"  | cut -d: -f1 > "$tmp/b"
if [[ $(cat "$tmp/a") -ge $(cat "$tmp/b") ]]; then
  echo "FAIL: time ranking does not put the slow unit first"; fails=$((fails+1)); fi
grep -n 'slow\.cpp' "$tmp/bycount" | cut -d: -f1 > "$tmp/a"
grep -n 'busy\.cpp' "$tmp/bycount" | cut -d: -f1 > "$tmp/b"
if [[ $(cat "$tmp/a") -le $(cat "$tmp/b") ]]; then
  echo "FAIL: count ranking does not put the busy unit first"; fails=$((fails+1)); fi

python3 "$script" "$b" --self slow --top 5 > "$tmp/out" 2>&1
check "self time subtracts the child" yes "9\.00s  InstantiateFunction: slow_outer"
check "child keeps its own time"      yes "1\.00s  InstantiateFunction: inner_child"
check "parent is not credited in full" no "10\.00s  InstantiateFunction: slow_outer"

# A childless span keeps everything: the shape of uninstrumented work, and the
# reason the count-only report could not see the regression this script found.
trace "$tmp/lone/lone.cpp.json" '[
 {"ph":"X","name":"Total ExecuteCompiler","ts":0,"dur":5000000,"args":{"count":1}},
 {"ph":"X","name":"InstantiateFunction","ts":0,"dur":5000000,"args":{"detail":"childless"}}]'
python3 "$script" "$tmp/lone" --self lone --top 5 > "$tmp/out" 2>&1
check "childless span keeps its time" yes "5\.00s  InstantiateFunction: childless"

# Negative control: nothing to read must say so and exit non-zero, or a build
# configured without -ftime-trace reads as a build with nothing slow in it.
mkdir -p "$tmp/empty"
python3 "$script" "$tmp/empty" > "$tmp/out" 2>&1
rc=$?
check "empty build dir explains itself" yes "no -ftime-trace output"
[[ $rc -eq 0 ]] && { echo "FAIL: empty build dir exited 0"; fails=$((fails+1)); }

# A JSON file that is not a trace must be skipped rather than crash the run.
echo '{"not":"a trace"}' > "$b/compile_commands.json"
echo 'not json at all'   > "$b/stray.json"
python3 "$script" "$b" --top 3 > "$tmp/out" 2>&1
check "non-trace JSON ignored" yes "2 translation units"

if [[ $fails -eq 0 ]]; then echo "compile_time_report selftest: all controls pass"; else
  echo "compile_time_report selftest: $fails control(s) failed"; fi
exit $((fails > 0))
