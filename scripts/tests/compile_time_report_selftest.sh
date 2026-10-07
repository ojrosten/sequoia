#!/bin/bash
# Controls for compile_time_report.py, and a mutation check of the controls.
#
#   compile_time_report_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the script and against each mutant of the script.
# The script must fail no control, and each mutant at least one.
#
# The controls write their own traces, so that each fixture holds the shape
# under test and nothing else. Stand-ins for ninja and clang answer --detail,
# so the selftest needs neither. The claims:
#
#   - a summary sums each phase's counts and times over every unit, and ranks
#     the units both by time and by count. The fixture's rankings disagree,
#     since a unit can grow slower without doing more of anything clang counts;
#   - a trace is a unit only if its object lies beside it. CMake's compiler
#     checks leave well-formed traces with no object. A file beside an object
#     which is not a trace is not a unit either;
#   - a build directory with no traces is an error, not an empty report;
#   - a comparison with a baseline lists each phase whose count moved, the
#     largest move first, including a phase new or gone. A baseline written
#     holds the counts of the run that writes it, after any comparison;
#   - self time subtracts each span's children from that span alone. A span
#     with no children keeps its whole duration, which is how work clang does
#     not instrument shows. Summary events, and the spans enclosing a whole
#     phase of compilation, are left out;
#   - --detail recompiles the one object matching --source at full
#     granularity, leaves the build's object alone, and groups the event's
#     sites by source line. Each way it can fail exits non-zero.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script="$here/../compile_time_report.py"
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
verbose=yes

fail() {
  fails=$((fails+1))
  if [[ $verbose == yes ]]; then echo "FAIL: $*"; fi
}

# run <argument>...: the subject's output in $out, and its exit status in $rc
run() { python3 "$subject" "$@" > "$out" 2>&1; rc=$?; }

check() { # check <name> <yes|no> <pattern> [file, by default $out]
  local got
  total=$((total+1))
  if grep -qE -- "$3" "${4:-$out}"; then got=yes; else got=no; fi
  if [[ $got != "$2" ]]; then fail "$1 (expected $2, got $got, pattern: $3)"; fi
}

exits() { # exits <name> <status|nonzero>
  total=$((total+1))
  if [[ $2 == nonzero ]]; then
    if [[ $rc -eq 0 ]]; then fail "$1 (expected a non-zero exit, got 0)"; fi
  elif [[ $rc -ne $2 ]]; then fail "$1 (expected exit $2, got $rc)"; fi
}

in_order() { # in_order <name> <file> <pattern>...: each matches a later line
  local name=$1 file=$2 previous=0 line pattern
  shift 2
  total=$((total+1))
  for pattern; do
    line=$(grep -nE -- "$pattern" "$file" | head -1 | cut -d: -f1)
    if [[ -z $line || $line -le $previous ]]; then
      fail "$name (absent or out of order: $pattern)"; return
    fi
    previous=$line
  done
}

exists() { # exists <name> <yes|no> <path>
  local got
  total=$((total+1))
  if [[ -e $3 ]]; then got=yes; else got=no; fi
  if [[ $got != "$2" ]]; then fail "$1 (expected $2, got $got: $3)"; fi
}

json_is() { # json_is <name> <file> <json>: the file holds the same JSON value
  total=$((total+1))
  if ! python3 -c 'import json, sys
sys.exit(json.load(open(sys.argv[1])) != json.loads(sys.argv[2]))' "$2" "$3" 2>/dev/null
  then fail "$1 ($2 does not hold $3)"; fi
}

section() { # section <heading>: the lines of $out from <heading> to a blank one
  awk -v heading="$1" '$0 ~ heading {on=1} on && /^$/ {exit} on' "$out"
}

trace() { # trace <file> <json-array-of-events>: a trace, and its object
  mkdir -p "$(dirname "$1")"
  printf '{"traceEvents": %s}\n' "$2" > "$1"
  : > "${1%.json}.o"
}

objectless() { # objectless <file> <json-array-of-events>: a trace alone
  trace "$1" "$2"
  rm "${1%.json}.o"
}

summary_controls() {
  local b="$work/build" now
  # Ten seconds, and three counted events.
  trace "$b/slow.cpp.json" '[
   {"ph":"X","name":"Total ExecuteCompiler","ts":0,"dur":10000000,"args":{"count":1}},
   {"ph":"X","name":"Total InstantiateFunction","ts":0,"dur":10000000,"args":{"count":2}}]'
  # One second, and half a million counted events.
  trace "$b/busy.cpp.json" '[
   {"ph":"X","name":"Total ExecuteCompiler","ts":0,"dur":1000000,"args":{"count":1}},
   {"ph":"X","name":"Total CheckConstraintSatisfaction","ts":0,"dur":900000,"args":{"count":500000}}]'
  # Two seconds, ten counted events, and two directories down.
  trace "$b/sub/dir/deep.cpp.json" '[
   {"ph":"X","name":"Total ExecuteCompiler","ts":0,"dur":2000000,"args":{"count":1}},
   {"ph":"X","name":"Total InstantiateFunction","ts":0,"dur":500000,"args":{"count":5}},
   {"ph":"X","name":"Total ParseClass","ts":0,"dur":250000,"args":{"count":4}}]'
  # The traces of a compiler check compiled to stdout, and of the compiler
  # identification. Neither leaves an object.
  objectless "$b/-.json" '[
   {"ph":"X","name":"Total ExecuteCompiler","ts":0,"dur":900000,"args":{"count":1}}]'
  objectless "$b/CMakeFiles/a-CMakeCXXCompilerId.json" '[
   {"ph":"X","name":"Total ExecuteCompiler","ts":0,"dur":900000,"args":{"count":1}}]'
  echo '{"not": "a trace"}' > "$b/compile_commands.json"
  echo 'not json at all'    > "$b/stray.json"
  # Files beside objects which are not traces: JSON with no events, JSON which
  # is no object, text which is no JSON, and bytes which are not UTF-8.
  echo '{"not": "a trace"}' > "$b/eventless.cpp.json"; : > "$b/eventless.cpp.o"
  echo '42'                 > "$b/number.cpp.json";    : > "$b/number.cpp.o"
  echo '{"traceEvents": ['  > "$b/truncated.cpp.json"; : > "$b/truncated.cpp.o"
  printf '"\xff"\n'         > "$b/latin1.cpp.json";    : > "$b/latin1.cpp.o"

  run "$b" --top 5
  exits "a summary succeeds" 0
  check "only traces beside objects are units" yes '^3 translation units$'
  check "a compiler check is not ranked"       no  'CMakeCXXCompilerId|s  -$'

  section 'phase totals' > "$work/phases"
  check "a phase sums over units" yes '^ +7 +10\.50s  InstantiateFunction$' "$work/phases"
  in_order "phases ranked by count" "$work/phases" \
    '^ +500000 +0\.90s  CheckConstraintSatisfaction$' 'InstantiateFunction$' \
    '^ +4 +0\.25s  ParseClass$' '^ +3 +13\.00s  ExecuteCompiler$'

  section 'slowest translation units' > "$work/bytime"
  in_order "units ranked by time" "$work/bytime" \
    '^ +10\.00s  slow\.cpp$' '^ +2\.00s  deep\.cpp$' '^ +1\.00s  busy\.cpp$'
  section 'heaviest translation units' > "$work/bycount"
  in_order "units ranked by count" "$work/bycount" \
    '^ +500001  busy\.cpp$' '^ +10  deep\.cpp$' '^ +3  slow\.cpp$'

  run "$b" --top 1
  section 'phase totals' > "$work/phases"
  check "--top bounds the phases"           no 'InstantiateFunction$' "$work/phases"
  section 'slowest translation units' > "$work/bytime"
  check "--top bounds the ranking by time"  no 'deep\.cpp$' "$work/bytime"
  section 'heaviest translation units' > "$work/bycount"
  check "--top bounds the ranking by count" no 'deep\.cpp$' "$work/bycount"

  # Earlier counts: one phase unmoved, one risen, one fallen, one gone, and
  # ParseClass absent.
  echo '{"CheckConstraintSatisfaction": 400000, "ExecuteCompiler": 3,
         "InstantiateFunction": 9, "Gone": 5}' > "$work/earlier.json"
  now='{"CheckConstraintSatisfaction": 500000, "ExecuteCompiler": 3,
        "InstantiateFunction": 7, "ParseClass": 4}'

  run "$b" --write-baseline "$work/written.json"
  check   "a written baseline is announced" yes "^baseline written to $work/written\.json$"
  json_is "a baseline holds the counts" "$work/written.json" "$now"

  run "$b" --baseline "$work/earlier.json"
  section 'against baseline' > "$work/moved"
  in_order "moved phases, the largest move first" "$work/moved" \
    '^ +400000 +500000 +\+100000  x1\.25  CheckConstraintSatisfaction$' \
    '^ +5 +0 +-5  x0\.00  Gone$' \
    '^ +0 +4 +\+4  \(new\)  ParseClass$' \
    '^ +9 +7 +-2  x0\.78  InstantiateFunction$'
  check "an unmoved phase is not listed" no 'ExecuteCompiler' "$work/moved"

  cp "$work/earlier.json" "$work/rolling.json"
  run "$b" --baseline "$work/rolling.json" --write-baseline "$work/rolling.json"
  check   "a baseline is compared before it is rewritten" yes 'x1\.25  CheckConstraintSatisfaction$'
  json_is "a baseline compared is rewritten" "$work/rolling.json" "$now"

  mkdir -p "$work/empty"
  run "$work/empty"
  exits "no traces is an error" nonzero
  check "no traces says so" yes 'no -ftime-trace output under'
}

self_controls() {
  local s="$work/self" long
  long=$(printf 'y%.0s' {1..150})
  # Self times: grandparent 8-5-1=2s, parent 5-2=3s, grandchild 2s, sibling 1s,
  # and the ParseClass span 4s. The parent starts with the grandparent, and
  # comes first in the file. The sibling starts as the parent ends. The spans
  # enclosing a phase of compilation lie apart, each with a second of its own.
  trace "$s/nest.cpp.json" '[
   {"ph":"X","name":"Total ExecuteCompiler","ts":0,"dur":20000000,"args":{"count":1}},
   {"ph":"X","name":"Total InstantiateFunction","ts":0,"dur":7000000,"args":{"count":2}},
   {"ph":"X","name":"InstantiateFunction","ts":0,"dur":5000000,"args":{"detail":"parent"}},
   {"ph":"X","name":"InstantiateClass","ts":0,"dur":8000000,"args":{"detail":"grandparent"}},
   {"ph":"X","name":"InstantiateFunction","ts":1000000,"dur":2000000,"args":{"detail":"grandchild"}},
   {"ph":"i","name":"InstantMarker","ts":2000000},
   {"ph":"X","name":"InstantiateClass","ts":5000000,"dur":1000000,"args":{"detail":"sibling"}},
   {"ph":"X","name":"ParseClass","ts":9000000,"dur":4000000,"args":{"detail":"'"$long"'"}},
   {"ph":"X","name":"ExecuteCompiler","ts":13000000,"dur":1000000},
   {"ph":"X","name":"Frontend","ts":14000000,"dur":1000000},
   {"ph":"X","name":"Backend","ts":15000000,"dur":1000000},
   {"ph":"X","name":"PerformPendingInstantiations","ts":16000000,"dur":1000000},
   {"ph":"X","name":"Source","ts":17000000,"dur":1000000,"args":{"detail":"header.hpp"}}]'
  trace "$s/nestling.cpp.json" '[
   {"ph":"X","name":"Total ExecuteCompiler","ts":0,"dur":1000000,"args":{"count":1}},
   {"ph":"X","name":"InstantiateClass","ts":0,"dur":500000,"args":{"detail":"fledgling"}}]'
  # The slowest unit: one span, and no children.
  trace "$s/lone.cpp.json" '[
   {"ph":"X","name":"Total ExecuteCompiler","ts":0,"dur":30000000,"args":{"count":1}},
   {"ph":"X","name":"InstantiateFunction","ts":0,"dur":25000000,"args":{"detail":"childless"}}]'

  run "$s" --self nest --top 10
  exits "--self succeeds" 0
  check "several matches take the slowest" yes '^several matched; taking the slowest, nest\.cpp$'
  check "the faster match is not shown"    no  'fledgling'
  check "a unit's total is its compile"    yes '^nest\.cpp: 20\.00s total$'
  section 'self time by phase' > "$work/phases"
  in_order "phases ranked by self time" "$work/phases" \
    '^ +5\.00s  InstantiateFunction$' '^ +4\.00s  ParseClass$' '^ +3\.00s  InstantiateClass$'
  check "a span loses its children's time" yes '^ +3\.00s  InstantiateFunction: parent$'
  check "a span keeps its grandchildren's" yes '^ +2\.00s  InstantiateClass: grandparent$'
  check "a span ending as another starts is not its parent" \
                                           yes '^ +1\.00s  InstantiateClass: sibling$'
  check "a span keeps the time not in children" \
                                           yes '^ +2\.00s  InstantiateFunction: grandchild$'
  check "an entity's detail is cut at 100" yes 'ParseClass: y{100}$'
  check "an entity's detail is cut at 100, not more" no 'y{101}'
  check "an event which is no span is ignored" no 'InstantMarker'
  check "summary events are not spans"     no 'Total'
  for enclosing in ExecuteCompiler Frontend Backend PerformPendingInstantiations Source; do
    check "an enclosing $enclosing span is left out" no "s  $enclosing(:|$)"
  done

  run "$s" --self nest --top 2
  section 'self time by phase' > "$work/phases"
  check "--top bounds the phases"   no 'InstantiateClass$' "$work/phases"
  section 'self time by entity' > "$work/entities"
  check "--top bounds the entities" no 'grandparent' "$work/entities"

  run "$s" --self nestling
  check "one match is taken without remark" no  'several matched'
  check "one match is the unit shown"       yes 'fledgling'

  run "$s" --self
  check "no fragment takes the slowest of all" yes 'taking the slowest, lone\.cpp$'
  check "a childless span keeps its time" yes '^ +25\.00s  InstantiateFunction: childless$'

  run "$s" --self absent
  exits "no matching unit is an error" nonzero
  check "no matching unit says so" yes "no translation unit matching 'absent'"
}

detail_controls() {
  local d="$work/ninja" object="CMakeFiles/T.dir/src/Ratio.cpp.o" unit unit_object
  mkdir -p "$d/CMakeFiles/T.dir/src" "$work/probe"
  : > "$d/build.ninja"
  # Ratio is in the name of a custom command's output and of a linked
  # executable, neither of which is compiled.
  cat > "$d/targets.txt" <<'EOF'
all: phony
CMakeFiles/gen.dir/Ratio.cpp.o: CUSTOM_COMMAND
RatioTool: CXX_EXECUTABLE_LINKER__T_Debug
CMakeFiles/T.dir/src/Ratio.cpp.o: CXX_COMPILER__T_unscanned_Debug
CMakeFiles/T.dir/src/Broken.cpp.o: CXX_COMPILER__T_unscanned_Debug
CMakeFiles/T.dir/src/Twin1.cpp.o: CXX_COMPILER__T_unscanned_Debug
CMakeFiles/T.dir/src/Twin2.cpp.o: CXX_COMPILER__T_unscanned_Debug
EOF
  for unit in Ratio Broken Twin1 Twin2; do
    unit_object="CMakeFiles/T.dir/src/$unit.cpp.o"
    printf '%s\t%s\n' "$unit_object" "fakecc -DX -ftime-trace -MD -MT $unit_object -MF $unit_object.d -o $unit_object -c /src/$unit.cpp"
  done > "$d/commands.txt"
  echo "the build's object" > "$d/$object"

  HOME=/home/fixture run "$d" --detail CheckConstraintSatisfaction --source Ratio --out-dir "$work/probe" --top 10
  exits "--detail succeeds" 0
  check "the recompile is announced" yes "^recompiling $object at full granularity$"
  check "every event is counted, by site" yes '^8 CheckConstraintSatisfaction events over 5 distinct sites$'
  in_order "sites ranked by count" "$out" \
    '^ +3 +6\.0ms  /src/a\.hpp:10$' '^ +2 +0\.6ms  /src/a\.hpp:20$'
  check "an event with no detail is a site"   yes '^ +1 +0\.3ms  <none>$'
  check "a detail with no location is a site" yes '^ +1 +0\.3ms  std::same_as<int, int>$'
  check "the home directory is abbreviated"   yes '^ +1 +0\.4ms  ~/b\.hpp:3$'
  check "other events are not sites"          no  'widget'
  exists "the probe's trace is in --out-dir" yes "$work/probe/time_trace_probe.json"
  exists "no trace is left beside the build's object" no "$d/${object%.o}.json"
  check "the build's object is left alone" yes "^the build's object$" "$d/$object"

  HOME=/home/fixture run "$d" --detail CheckConstraintSatisfaction --source Ratio --out-dir "$work/probe" --top 1
  check "--top bounds the sites" no 'a\.hpp:20'

  run "$d" --detail Missing --source Ratio --out-dir "$work/probe"
  exits "an event the unit lacks is an error" nonzero
  check "an event the unit lacks says so" yes "^no 'Missing' events; this TU recorded:$"
  check "the events recorded are listed"  yes '^  ParseClass$'
  check "summary events are not listed"   no  '^  Total'

  run "$d" --detail CheckConstraintSatisfaction --source Twin --out-dir "$work/probe"
  exits "an ambiguous --source is an error" nonzero
  in_order "an ambiguous --source names its matches" "$out" '^ambiguous --source; matches:$' \
    '^  CMakeFiles/T\.dir/src/Twin1\.cpp\.o$' '^  CMakeFiles/T\.dir/src/Twin2\.cpp\.o$'

  run "$d" --detail CheckConstraintSatisfaction --source Nowhere --out-dir "$work/probe"
  exits "an unmatched --source is an error" nonzero
  check "an unmatched --source says so" yes "^no object matching 'Nowhere' in $d$"

  run "$d" --detail CheckConstraintSatisfaction --source Broken --out-dir "$work/probe"
  exits "a failed recompile gives the compiler's status" 3
  check "a failed recompile shows the compiler's error" yes 'Broken\.cpp:1:1: error: fixture'
}

# Stand-ins for ninja and clang, first in PATH.
mkdir -p "$tmp/bin"
cat > "$tmp/bin/ninja" <<'EOF'
#!/bin/bash
# ninja -C <dir> -t targets all, or ninja -C <dir> -t commands <target>,
# answered from targets.txt and commands.txt in <dir>. The command for the
# target follows a command for a prerequisite, as ninja prints them.
dir=$2
if [[ ! -f $dir/build.ninja ]]; then
  echo "ninja: error: loading 'build.ninja': No such file or directory" >&2; exit 1
fi
case $4 in
  targets)  cat "$dir/targets.txt" ;;
  commands) echo 'echo a prerequisite'
            awk -F '\t' -v target="$5" '$1 == target {print $2}' "$dir/commands.txt" ;;
esac
EOF
# Writes the object named by -o, and the depfile named by -MF. With
# -ftime-trace, it also writes the trace beside the object, holding fine.json's
# events at -ftime-trace-granularity=0, and coarse.json's otherwise. A source
# named Broken fails to compile.
cat > "$tmp/bin/fakecc" <<'EOF'
#!/bin/bash
here=$(dirname "$0"); trace=no; granularity=coarse; object=; depfile=
while [[ $# -gt 0 ]]; do
  case $1 in
    -ftime-trace)               trace=yes ;;
    -ftime-trace-granularity=0) granularity=fine ;;
    -o)                         object=$2; shift ;;
    -MF)                        depfile=$2; shift ;;
    *Broken*)                   echo "Broken.cpp:1:1: error: fixture" >&2; exit 3 ;;
  esac
  shift
done
: > "$object" || exit 1
if [[ -n $depfile ]]; then echo "$object: fixture" > "$depfile" || exit 1; fi
if [[ $trace == yes ]]; then
  printf '{"traceEvents": %s}\n' "$(cat "$here/$granularity.json")" > "${object%.o}.json" || exit 1
fi
EOF
chmod +x "$tmp/bin/ninja" "$tmp/bin/fakecc"
cat > "$tmp/bin/coarse.json" <<'EOF'
[{"ph":"X","name":"Total CheckConstraintSatisfaction","ts":0,"dur":9000,"args":{"count":8}}]
EOF
cat > "$tmp/bin/fine.json" <<'EOF'
[{"ph":"X","name":"Total CheckConstraintSatisfaction","ts":0,"dur":9000,"args":{"count":8}},
 {"ph":"X","name":"CheckConstraintSatisfaction","ts":0,"dur":1000,"args":{"detail":"</src/a.hpp:10:5>"}},
 {"ph":"X","name":"CheckConstraintSatisfaction","ts":1000,"dur":2000,"args":{"detail":"</src/a.hpp:10:9>"}},
 {"ph":"X","name":"CheckConstraintSatisfaction","ts":3000,"dur":3000,"args":{"detail":"</src/a.hpp:10:5>"}},
 {"ph":"X","name":"CheckConstraintSatisfaction","ts":6000,"dur":500,"args":{"detail":"</src/a.hpp:20:1, col:9>"}},
 {"ph":"X","name":"CheckConstraintSatisfaction","ts":6500,"dur":100,"args":{"detail":"</src/a.hpp:20:3>"}},
 {"ph":"X","name":"CheckConstraintSatisfaction","ts":7000,"dur":300},
 {"ph":"X","name":"CheckConstraintSatisfaction","ts":7500,"dur":300,"args":{"detail":"std::same_as<int, int>"}},
 {"ph":"X","name":"CheckConstraintSatisfaction","ts":8000,"dur":400,"args":{"detail":"</home/fixture/b.hpp:3:1>"}},
 {"ph":"X","name":"ParseClass","ts":8500,"dur":100,"args":{"detail":"widget"}}]
EOF
export PATH="$tmp/bin:$PATH"

run_controls() { # run_controls <script>: sets fails, and total, the controls run
  subject=$1 fails=0 total=0
  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"
  summary_controls
  self_controls
  detail_controls
}

# Each mutant breaks one behaviour, and is (description, old text, new text).
# Two are left out as equivalent on any input ninja writes:
#   - dropping `".o:" in l` from compile_command, since a line which ends in
#     "o: CXX_COMPILER__<rule>" names an object;
#   - dropping the test for compile_commands.json in traces, since no object
#     lies beside it.
mutants() { # mutants <dir>: writes <dir>/<n>/compile_time_report.py, and
            # prints a line "<n> <occurrences of the old text> <description>"
  python3 - "$script" "$1" <<'EOF'
import os, sys
MUTATIONS = [
    ('every JSON file a trace',          'if not os.path.exists(path[:-len(".json")] + ".o"):', 'if False:'),
    ('a trace with no events read',      'if not isinstance(d, dict) or "traceEvents" not in d:', 'if not isinstance(d, dict):'),
    ('JSON which is no object read',     'if not isinstance(d, dict) or "traceEvents" not in d:', 'if "traceEvents" not in d:'),
    ('text which is no JSON read',       'except (json.JSONDecodeError, UnicodeDecodeError):', 'except UnicodeDecodeError:'),
    ('bytes which are not UTF-8 read',   'except (json.JSONDecodeError, UnicodeDecodeError):', 'except json.JSONDecodeError:'),
    ('one directory only',               'for root, _, files in os.walk(build_dir):', 'for root, files in [(build_dir, os.listdir(build_dir))]:'),
    ('a phase total ignores its count',  'out[name[len("Total "):]] = (e.get("args", {}).get("count", 0),', 'out[name[len("Total "):]] = (1,'),
    ('a phase counted from one unit',    'agg[phase] += count', 'agg[phase] = count'),
    ('a phase timed from one unit',      'dur[phase] += d', 'dur[phase] = d'),
    ('phases ranked by time',            'for phase, count in agg.most_common(top):', 'for phase, count in sorted(agg.items(), key=lambda x: -dur[x[0]])[:top]:'),
    ('every phase listed',               'for phase, count in agg.most_common(top):', 'for phase, count in agg.most_common():'),
    ('units ranked by time ascending',   'key=lambda x: -x[1])[:top]:', 'key=lambda x: x[1])[:top]:'),
    ('every unit ranked by time',        'key=lambda x: -x[1])[:top]:', 'key=lambda x: -x[1]):'),
    ('every unit ranked by count',       'for tu, count in per_tu.most_common(top):', 'for tu, count in per_tu.most_common():'),
    ('a unit counted by its last phase', 'per_tu[tu] = sum(c for c, _ in totals(events).values())', 'per_tu[tu] = count'),
    ('an empty build accepted',          'if not tus:\n        sys.exit(f"no -ftime-trace output', 'if False:\n        sys.exit(f"no -ftime-trace output'),
    ('an unmoved phase listed',          '        if b == n:\n            continue\n', ''),
    ('moves ranked by name',             'key=lambda p: -abs(agg.get(p, 0) - base.get(p, 0))', 'key=lambda p: p'),
    ('a gone phase left out',            'for phase in sorted(set(base) | set(agg),', 'for phase in sorted(set(agg),'),
    ('a new phase left out',             'for phase in sorted(set(base) | set(agg),', 'for phase in sorted(set(base),'),
    ('a new phase as a ratio',           'ratio = f"  x{n / b:.2f}" if b else "  (new)"', 'ratio = f"  x{n / max(b, 1):.2f}"'),
    ('written before compared',          '    if a.baseline:\n        compare(agg, a.baseline)\n    if a.write_baseline:\n        with open(a.write_baseline, "w", encoding="utf-8") as f:\n            json.dump(dict(agg), f, indent=1, sort_keys=True)\n',
                                         '    if a.write_baseline:\n        with open(a.write_baseline, "w", encoding="utf-8") as f:\n            json.dump(dict(agg), f, indent=1, sort_keys=True)\n    if a.baseline:\n        compare(agg, a.baseline)\n'),
    ('a baseline holds times',           'json.dump(dict(agg), f,', 'json.dump(dict(dur), f,'),
    ('every event a span',               'if e.get("ph") == "X" and not', 'if not'),
    ('summary events are spans',         'and not e.get("name", "").startswith("Total ")\n', '\n'),
    ('ExecuteCompiler is a span',        '("ExecuteCompiler", "Frontend"', '("Frontend"'),
    ('Frontend is a span',               '"Frontend", "Backend",', '"Backend",'),
    ('Backend is a span',                '"Backend",\n', '\n'),
    ('PerformPendingInstantiations is a span', '"PerformPendingInstantiations", "Source")', '"Source")'),
    ('Source is a span',                 '"PerformPendingInstantiations", "Source")', '"PerformPendingInstantiations",)'),
    ('a parent may sort after its child', 'spans.sort(key=lambda e: (e["ts"], -e.get("dur", 0)))', 'spans.sort(key=lambda e: e["ts"])'),
    ('a span ending as another starts is its parent', 'stack[-1][0] + stack[-1][1] <= ts', 'stack[-1][0] + stack[-1][1] < ts'),
    ('a child taken from every ancestor', '        if stack:\n            stack[-1][2] -= dur\n', '        for frame in stack:\n            frame[2] -= dur\n'),
    ('no child taken',                   '            stack[-1][2] -= dur\n', '            pass\n'),
    ('entities merged by name',          'by_entity[(e["name"], e.get("args", {}).get("detail", ""))] += slf', 'by_entity[(e["name"], "")] += slf'),
    ('a unit timed as zero',             '            return e.get("dur", 0)\n    return 0', '            return 0\n    return 0'),
    ('several matches take the fastest', 'hits = [max(hits, key=lambda t: wall(tus[t]))]', 'hits = [min(hits, key=lambda t: wall(tus[t]))]'),
    ('no unit matched accepted',         'if not hits:\n        sys.exit(f"no translation unit matching', 'if False:\n        sys.exit(f"no translation unit matching'),
    ('self phases unbounded',            'for name, d in by_phase.most_common(top):', 'for name, d in by_phase.most_common():'),
    ('self entities unbounded',          'for (name, det), d in by_entity.most_common(top):', 'for (name, det), d in by_entity.most_common():'),
    ('a detail uncut',                   '{det[:100]}', '{det}'),
    ('a custom command compiled',        'if l.endswith("o: CXX_COMPILER__" + l.split("CXX_COMPILER__")[-1])\n            and ', 'if '),
    ('the first of several matches',     'if len(hits) > 1:\n        sys.exit("ambiguous', 'if False:\n        sys.exit("ambiguous'),
    ('no object matched accepted',       'if not hits:\n        sys.exit(f"no object matching', 'if False:\n        sys.exit(f"no object matching'),
    ('the first command run',            '.stdout.strip().splitlines()[-1]', '.stdout.strip().splitlines()[0]'),
    ('the default granularity',          'cmd = cmd.replace("-ftime-trace", "-ftime-trace -ftime-trace-granularity=0")', 'pass'),
    ('the build object overwritten',     'cmd = re.sub(r"-o \\S+\\.o", f"-o {probe}", cmd)', 'pass'),
    ('a failed recompile read',          'if r.returncode:\n        sys.exit(r.returncode)', 'if False:\n        sys.exit(r.returncode)'),
    ('a failed recompile exits 1',       'sys.exit(r.returncode)', 'sys.exit(1)'),
    ('an absent event accepted',         'if not hits:\n        names = sorted(', 'if False:\n        names = sorted('),
    ('summary events listed',            '        names = sorted({e.get("name", "") for e in events\n                        if not e.get("name", "").startswith("Total ")})',
                                         '        names = sorted({e.get("name", "") for e in events})'),
    ('sites by file',                    'key = f"{m.group(\'file\')}:{m.group(\'line\')}" if m else d', 'key = m.group(\'file\') if m else d'),
    ('sites by column',                  'LOC = re.compile(r"<(?P<file>[^:<>]+):(?P<line>\\d+):\\d+")', 'LOC = re.compile(r"<(?P<file>[^:<>]+):(?P<line>\\d+:\\d+)")'),
    ('a site timed by its last event',   'dur[key] += e.get("dur", 0)', 'dur[key] = e.get("dur", 0)'),
    ('sites unbounded',                  'for key, n in count.most_common(top):', 'for key, n in count.most_common():'),
    ('the home directory spelt out',     'short = key.replace(os.path.expanduser("~"), "~")', 'short = key'),
    ('--out-dir ignored',                'a.out_dir or a.build_dir', 'a.build_dir'),
]
script, directory = sys.argv[1:]
with open(script, encoding='utf-8') as f:
    source = f.read()
for n, (description, old, new) in enumerate(MUTATIONS):
    os.makedirs(os.path.join(directory, str(n)))
    with open(os.path.join(directory, str(n), 'compile_time_report.py'), 'w', encoding='utf-8') as f:
        f.write(source.replace(old, new))
    print(n, source.count(old), description)
EOF
}

if [[ ${1:-} == --mutations ]]; then
  verbose=no survivors=0
  run_controls "$script"
  echo "unmutated: $fails of $total controls fail$( ((fails)) && echo '  <-- must be none')"
  if ((fails)); then survivors=1; fi
  mutants "$tmp/mutants" > "$tmp/mutants.txt" || exit 1
  while read -r n occurrences description; do
    if [[ $occurrences -ne 1 ]]; then
      echo "$description: the text to mutate occurs $occurrences times"; survivors=1; continue
    fi
    run_controls "$tmp/mutants/$n/compile_time_report.py"
    echo "$description: $fails of $total controls fail$( ((fails)) || echo '  <-- SURVIVED')"
    if ((fails == 0)); then survivors=1; fi
  done < "$tmp/mutants.txt"
  exit $survivors
fi

run_controls "$script"
if [[ $fails -eq 0 ]]; then echo "compile_time_report selftest: all $total controls pass"; else
  echo "compile_time_report selftest: $fails of $total controls failed"; fi
exit $((fails > 0))
