#!/bin/bash
# Controls for generate_coverage_report.sh, against stand-ins for the tools it
# runs, and a mutation check of the controls.
#
#   generate_coverage_report_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the script and against each mutant of the script.
# The script must fail no control, and each mutant at least one. The last line
# gives the verdict. If the script fails a control, no mutant runs. A mutant
# whose text is not found exactly once counts as a survivor.
#
# Each control runs a copy of the script as CI runs it, from the root of a
# scratch repository, on a scratch build of TestAll. Stand-ins for lcov,
# genhtml, ctest, ninja, llvm-cov and check_tracefile.py log each call, and
# most fail when a control asks them to. Stand-ins for uname and brew give the
# platform and Homebrew's prefix a control chooses. So the selftest checks
# what the script decides and passes, and needs no build. The claims:
#
#   - the report is written to coverage_reports beside the last directory
#     named build in the build's physical path, under the rest of that path.
#     A Setup.txt in the build names a subdirectory of the report, by its
#     first line. Only genhtml makes the report's directory, so a run which
#     fails before genhtml makes none. Before genhtml runs, the report's
#     directory is deleted, and nothing beside it. A report's directory whose
#     physical path is not within coverage_reports is refused before any tool
#     runs, and nothing is deleted;
#   - ninja, taken from the build's cache rather than from PATH, deletes the
#     objects of sources the build no longer has. Then every notes file
#     without its object is deleted, anywhere in the build, so that the
#     capture reads only live notes files;
#   - the counters are zeroed, then ctest runs the suite in the build, then
#     lcov captures, removes the foreign files, and summarises. Then
#     check_tracefile.py checks the removal, and genhtml writes the report;
#   - the gcov tool is the one beside g++-N with N's version, or a wrapper of
#     llvm-cov gcov beside clang++, or gcov for any other compiler. A gcov_tool
#     in the environment changes nothing;
#   - each lcov call, check_tracefile.py and genhtml get the options the
#     script's comments give reasons for. genhtml runs once, given the three
#     error categories lcov 2.5 knows. On Darwin, removal takes the
#     toolchains' paths too, and an unused pattern is no error;
#   - on Darwin, genhtml is given GNU c++filt from Homebrew's binutils, under
#     the prefix brew gives, and the script refuses to start without brew or
#     the c++filt. On Linux, brew is not run, and genhtml is given no c++filt;
#   - a missing, empty or second argument is refused with status 2 and the
#     usage on standard error alone. A missing build directory, a build
#     directory not within a directory named build, and a build without a
#     cache stop the script too. In each case no tool runs;
#   - each step which fails stops the script, with a non-zero status and an
#     error naming the step, and no later step runs. lcov's reason for
#     refusing a summary is shown.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script=$here/../generate_coverage_report.sh
# Physical, since the script reports the build's physical path.
tmp=$(cd "$(mktemp -d)" && pwd -P)
trap 'chmod -R u+w "$tmp" 2> /dev/null; rm -rf "$tmp"' EXIT

case "$*" in
  ''|--mutations) ;;
  *) echo "Usage: $0 [--mutations]" >&2; exit 2;;
esac

# The stand-ins. Each of lcov, genhtml, ctest, the cache's ninja and
# check_tracefile.py logs its call as one line of $FAKE/log, and fails with a
# status of its own when $FAIL names its step.
# brew gives $tmp/homebrew as Homebrew's prefix, so the script looks for GNU
# c++filt here.
gnu_cxxfilt=$tmp/homebrew/opt/binutils/bin/c++filt
mkdir -p "$tmp/bin" "$tmp/cache" "$tmp/llvm/bin" "${gnu_cxxfilt%/*}"

# lcov writes the tracefile a capture or a removal asks for, and summarises as
# lcov does: the figures on standard output, an error on standard error. A
# removal or a summary refuses an input which is absent. The capture records
# the notes files it would read.
cat > "$tmp/bin/lcov" <<'EOF'
#!/bin/bash
echo "lcov $*" >> "$FAKE/log"
args=("$@") step= input= output= directory=
for ((i = 0; i < ${#args[@]}; i++)); do
  case ${args[i]} in
    --zerocounters) step=zerocounters ;;
    --capture)      step=capture ;;
    --remove)       step=remove;  input=${args[i+1]} ;;
    --summary)      step=summary; input=${args[i+1]} ;;
    --output-file)  output=${args[i+1]} ;;
    --directory)    directory=${args[i+1]} ;;
  esac
done
if [[ -n $input && ! -f $input ]]; then
  echo "lcov: ERROR: (missing) '$input' is not a readable file" >&2; exit 1
fi
if [[ $FAIL == "$step" ]]; then echo "lcov: ERROR: the fixture's $step fails" >&2; exit 7; fi
case $step in
  capture) find "$directory" -name '*.gcno' | sort > "$FAKE/notes_at_capture"
           printf 'SF:/fixture/a.cpp\nend_of_record\n' > "$output" ;;
  remove)  cp "$input" "$output" ;;
  summary) printf 'Reading tracefile %s.\nSummary coverage rate:\n' "$input"
           printf '  lines.......: 50.0%% (1 of 2 lines)\n  functions...: 100.0%% (1 of 1 function)\n' ;;
esac
EOF

# genhtml makes its output directory, as genhtml does, and writes an index
# there.
cat > "$tmp/bin/genhtml" <<'EOF'
#!/bin/bash
echo "genhtml $*" >> "$FAKE/log"
args=("$@") output= input=
for ((i = 0; i < ${#args[@]}; i++)); do
  case ${args[i]} in
    -o)     output=${args[i+1]} ;;
    *.info) input=${args[i]} ;;
  esac
done
if [[ ! -f $input ]]; then echo "genhtml: ERROR: no tracefile '$input'" >&2; exit 1; fi
if [[ $FAIL == genhtml ]]; then echo "genhtml: ERROR: the fixture's genhtml fails" >&2; exit 7; fi
mkdir -p "$output" && echo report > "$output/index.html"
EOF

cat > "$tmp/bin/ctest" <<'EOF'
#!/bin/bash
echo "ctest $* (in $(pwd -P))" >> "$FAKE/log"
if [[ $FAIL == ctest ]]; then echo "The following tests FAILED" >&2; exit 8; fi
EOF

cat > "$tmp/bin/uname" <<'EOF'
#!/bin/bash
echo "$FAKE_UNAME"
EOF

# brew logs its call to $FAKE/brew, and gives $FAKE_HOMEBREW as the prefix.
# If that is `absent`, it fails as a shell does on a missing command.
cat > "$tmp/bin/brew" <<'EOF'
#!/bin/bash
echo "brew $*" >> "$FAKE/brew"
if [[ $FAKE_HOMEBREW == absent ]]; then echo "brew: command not found" >&2; exit 127; fi
echo "$FAKE_HOMEBREW"
EOF

# A ninja on PATH, which the script must not run, and the ninja which the
# build's cache names. The latter deletes the objects listed in the build's
# dead_objects.txt, as cleandead deletes the outputs a build no longer has.
cat > "$tmp/bin/ninja" <<'EOF'
#!/bin/bash
echo "PATH's ninja $*" >> "$FAKE/log"
EOF
cat > "$tmp/cache/ninja" <<'EOF'
#!/bin/bash
echo "ninja $*" >> "$FAKE/log"
if [[ $FAIL == cleandead ]]; then echo "ninja: error: the fixture's cleandead fails" >&2; exit 9; fi
while read -r dead; do rm "$2/$dead"; done < "$2/dead_objects.txt"
EOF

# Logs its arguments, each in brackets, to $FAKE/llvm-cov.
cat > "$tmp/llvm/bin/llvm-cov" <<'EOF'
#!/bin/bash
printf '[%s]' "$@" > "$FAKE/llvm-cov"
EOF

cat > "$tmp/check_tracefile.py" <<'EOF'
import os, sys
with open(os.path.join(os.environ['FAKE'], 'log'), 'a') as log:
    log.write(' '.join(['check_tracefile.py'] + sys.argv[1:]) + '\n')
if os.environ['FAIL'] == 'check':
    print('check_tracefile.py: the fixture\'s check fails', file=sys.stderr)
    sys.exit(6)
EOF

chmod +x "$tmp/bin/"* "$tmp/cache/ninja" "$tmp/llvm/bin/llvm-cov"

fail() { echo "FAIL: $1"; fails=$((fails+1)); }

# check <name> <yes|no> <pattern> <file>
check() {
  if grep -qE -- "$3" "$4" 2> /dev/null; then got=yes; else got=no; fi
  [ "$got" = "$2" ] || fail "$1 (expected $2, got $got, pattern: $3)"
}

# called <name> <line>: the log holds the line, exactly
called() {
  grep -qFx -- "$2" "$case_dir/log" && return
  fail "$1: no call"
  echo "  expected: $2"
  grep -F -- "${2%% *} " "$case_dir/log" | sed 's/^/  logged:   /'
}

# uncalled <name> <prefix>: no line of the log begins with the prefix
uncalled() {
  ! grep -q "^$2" "$case_dir/log" || fail "$1: $(grep "^$2" "$case_dir/log" | head -1)"
}

# exits <name> <status|nonzero>
exits() {
  if [ "$2" = nonzero ]; then
    [ "$rc" -ne 0 ] || fail "$1 (expected a non-zero exit, got 0)"
  else
    [ "$rc" -eq "$2" ] || fail "$1 (expected exit $2, got $rc)"
  fi
}

# in_order <name> <prefix>...: lines of the log beginning with each prefix, in
# that order
in_order() {
  local name=$1 previous=0 line prefix
  shift
  for prefix; do
    line=$(grep -n "^$prefix" "$case_dir/log" | head -1 | cut -d: -f1)
    if [ -z "$line" ] || [ "$line" -le "$previous" ]; then
      fail "$name (absent or out of order: $prefix)"; return
    fi
    previous=$line
  done
}

# exists <name> <yes|no> <path>
exists() {
  if [ -e "$3" ]; then got=yes; else got=no; fi
  [ "$got" = "$2" ] || fail "$1 (expected $2, got $got: $3)"
}

# fixture <case> <compiler> [<repository> [<build>]]
# A scratch repository, $repo, holding the script and check_tracefile.py's
# stand-in. It lies at <repository> within the case's directory, by default at
# repo. It holds a build, $b, compiled by <compiler>, at <build> within the
# repository, by default at build/TestAll/gcc-env-coverage. The build's notes
# files are:
#   - Live.cpp.gcno, beside its object, and its data file;
#   - Orphan.cpp.gcno, and deeper/Nested.cpp.gcno, with no object;
#   - Dropped.cpp.gcno, whose object ninja's cleandead deletes.
# In the cache, an entry whose name begins with CMAKE_CXX_COMPILER comes before
# the compiler's own. The g++ the controls name lies in a directory whose name
# holds `g++-` too.
fixture() {
  case_dir=$cases/$1
  repo=$case_dir/${3:-repo}
  b=$repo/${4:-build/TestAll/gcc-env-coverage}
  local src=$b/CMakeFiles/T.dir/src
  mkdir -p "$repo/scripts" "$src/deeper"
  cp "$subject" "$repo/scripts/generate_coverage_report.sh"
  cp "$tmp/check_tracefile.py" "$repo/scripts/check_tracefile.py"
  chmod +x "$repo/scripts/generate_coverage_report.sh"
  cat > "$b/CMakeCache.txt" <<EOF
# This is the CMakeCache file.
//A wrapper around 'ar' adding the appropriate '--plugin' option for the GCC compiler
CMAKE_CXX_COMPILER_AR:FILEPATH=$tmp/g++-13/bin/gcc-ar-15
//CXX compiler
CMAKE_CXX_COMPILER:FILEPATH=$2
//Program used to build from build.ninja files.
CMAKE_MAKE_PROGRAM:FILEPATH=$tmp/cache/ninja
CMAKE_CXX_COMPILER_RANLIB:FILEPATH=$tmp/g++-13/bin/gcc-ranlib-15
EOF
  : > "$src/Live.cpp.o"; : > "$src/Live.cpp.gcno"; : > "$src/Live.cpp.gcda"
  : > "$src/Orphan.cpp.gcno"; : > "$src/deeper/Nested.cpp.gcno"
  : > "$src/Dropped.cpp.o"; : > "$src/Dropped.cpp.gcno"
  echo CMakeFiles/T.dir/src/Dropped.cpp.o > "$b/dead_objects.txt"
  : > "$case_dir/log"
  platform=Linux failing= cxxfilt=absent homebrew=$tmp/homebrew from=$repo invoke=./scripts/generate_coverage_report.sh
}

# run <argument>...
# Runs the script as $invoke, from $from, with $platform, $failing, $cxxfilt
# and $homebrew. Their defaults run it as CI does, from the repository's root.
# Sets $rc. The script's standard output is in $case_dir/out, its standard
# error in $case_dir/err, and both in $case_dir/all.
run() {
  rm -f "$gnu_cxxfilt"
  case $cxxfilt in
    present)      printf '#!/bin/sh\n' > "$gnu_cxxfilt"; chmod +x "$gnu_cxxfilt" ;;
    unexecutable) printf '#!/bin/sh\n' > "$gnu_cxxfilt" ;;
  esac
  (cd "$from" && FAKE=$case_dir PATH="$tmp/bin:$PATH" FAKE_UNAME=$platform \
                 FAIL=$failing FAKE_HOMEBREW=$homebrew \
                 "$invoke" "$@" > "$case_dir/out" 2> "$case_dir/err")
  rc=$?
  cat "$case_dir/out" "$case_dir/err" > "$case_dir/all"
}

# The options the script passes to each call, as the stand-ins log them.
capture_options='--keep-going --filter range --rc geninfo_unexecuted_blocks=1 --rc check_data_consistency=0'
capture_options+=' --ignore-errors empty --ignore-errors inconsistent,inconsistent --ignore-errors format,format'
read_options='--rc check_data_consistency=0 --rc derive_function_end_line=0'
remove_options='--keep-going --ignore-errors empty --ignore-errors format'
darwin_remove_options="$remove_options --ignore-errors unused"
darwin_patterns='/usr/* /opt/homebrew/* /Library/Developer/* /Applications/Xcode.app/*'
categories='--ignore-errors range --ignore-errors empty --ignore-errors category'

# The start of each call which depends on the build $b alone, and the report's
# directory when the repository is $repo.
calls_in_build() {
  capture_call="lcov --directory $b --capture --all --output-file $b/coverage_capture.info"
  remove_call="lcov --remove $b/coverage_capture.info"
  check_call="check_tracefile.py --capture $b/coverage_capture.info --filtered $b/coverage.info"
  check_call+=" --summary $b/coverage_summary.txt --removed"
  report=$repo/coverage_reports/TestAll/gcc-env-coverage
}

# The calls every run makes in the build $b, when nothing fails.
common_calls() {
  called "$1: ninja from the cache deletes dead outputs" "ninja -C $b -t cleandead"
  called "$1: the counters are zeroed" "lcov --zerocounters --directory $b"
  called "$1: ctest runs the suite in the build" "ctest -T Test (in $b)"
  called "$1: the summary reads the filtered tracefile unrepaired" "lcov --summary $b/coverage.info $read_options"
  uncalled "$1: PATH's ninja is not run" "PATH's ninja"
}

linux_gcc_controls() {
  fixture linux_gcc "$tmp/g++-13/bin/g++-15"
  run build/TestAll/gcc-env-coverage
  calls_in_build
  exits "a run on Linux succeeds" 0
  common_calls "Linux"
  called "the capture uses gcov-N beside g++-N" "$capture_call --gcov-tool $tmp/g++-13/bin/gcov-15 $capture_options"
  check "the gcov tool is printed" yes "^gcov: $tmp/g\+\+-13/bin/gcov-15$" "$case_dir/out"
  called "on Linux, removal takes /usr alone" \
         "$remove_call /usr/* --output-file $b/coverage.info $remove_options $read_options"
  called "on Linux, the check is told of /usr alone" "$check_call /usr/*"
  check "the summary is kept for the check" yes '^  lines\.+: 50\.0% \(1 of 2 lines\)$' "$b/coverage_summary.txt"
  check "the summary is printed" yes '^  functions\.+: 100\.0% \(1 of 1 function\)$' "$case_dir/out"
  called "genhtml is given its three categories, and plain --demangle-cpp" \
         "genhtml --demangle-cpp --suppress-aliases -o $report $b/coverage.info $categories $read_options"
  exists "the report is in coverage_reports, under the build's path within build" yes \
         "$repo/coverage_reports/TestAll/gcc-env-coverage/index.html"
  check "the build directory is printed" yes "^Test Dir: $b$" "$case_dir/out"
  [ "$(grep -c '^genhtml ' "$case_dir/log")" -eq 1 ] || fail "genhtml runs once, to write the report"
  in_order "the steps run in order" "ninja " "lcov --zerocounters" "ctest " "lcov --directory .* --capture" \
           "lcov --remove" "lcov --summary" "check_tracefile.py" "genhtml --demangle-cpp"
  [ "$(cat "$case_dir/notes_at_capture" 2> /dev/null)" = "$b/CMakeFiles/T.dir/src/Live.cpp.gcno" ] \
    || fail "the capture reads only the live notes file: $(cat "$case_dir/notes_at_capture" 2> /dev/null)"
  exists "a data file is left alone" yes "$b/CMakeFiles/T.dir/src/Live.cpp.gcda"
}

darwin_clang_controls() {
  fixture darwin_clang "$tmp/llvm/bin/clang++"
  platform=Darwin cxxfilt=present
  run build/TestAll/gcc-env-coverage
  calls_in_build
  exits "a run on Darwin succeeds" 0
  [ "$(cat "$case_dir/brew" 2> /dev/null)" = "brew --prefix" ] \
    || fail "on Darwin, brew gives Homebrew's prefix: $(cat "$case_dir/brew" 2> /dev/null)"
  common_calls "Darwin"
  called "the capture uses a wrapper of llvm-cov beside clang++" \
         "$capture_call --gcov-tool $b/llvm-gcov.sh $capture_options"
  (FAKE=$case_dir "$b/llvm-gcov.sh" "a dir/x.gcda" -b > /dev/null 2>&1)
  [ "$(cat "$case_dir/llvm-cov" 2> /dev/null)" = "[gcov][a dir/x.gcda][-b]" ] \
    || fail "the wrapper runs llvm-cov gcov with its arguments: $(cat "$case_dir/llvm-cov" 2> /dev/null)"
  called "on Darwin, removal takes the toolchains' paths, and an unused pattern is no error" \
         "$remove_call $darwin_patterns --output-file $b/coverage.info $darwin_remove_options $read_options"
  called "on Darwin, the check is told of the toolchains' paths" "$check_call $darwin_patterns"
  called "genhtml is given its three categories, and GNU c++filt" \
         "genhtml --demangle-cpp $gnu_cxxfilt --suppress-aliases -o $report $b/coverage.info $categories $read_options"
}

# The gcov tool, and the c++filt, the script chooses in other builds.
choice_controls() {
  fixture other_compiler /usr/bin/c++
  run build/TestAll/gcc-env-coverage
  exits "a build by another compiler succeeds" 0
  check "another compiler's tool is gcov" yes '^gcov: gcov$' "$case_dir/out"
  check "the capture uses gcov for another compiler" yes '^lcov --directory .* --gcov-tool gcov ' "$case_dir/log"

  fixture environment "$tmp/g++-13/bin/g++-15"
  (export gcov_tool=/custom/gcov; run build/TestAll/gcc-env-coverage; echo "$rc" > "$case_dir/rc")
  rc=$(cat "$case_dir/rc")
  exits "a gcov_tool in the environment succeeds" 0
  check "a gcov_tool in the environment is ignored" yes \
        "^lcov --directory .* --gcov-tool $tmp/g\+\+-13/bin/gcov-15 " "$case_dir/log"

  fixture linux_cxxfilt "$tmp/g++-13/bin/g++-15"
  cxxfilt=present
  run build/TestAll/gcc-env-coverage
  exits "a run on Linux with GNU c++filt where Homebrew puts it succeeds" 0
  check "on Linux, Homebrew's GNU c++filt is not given to genhtml" yes \
        '^genhtml --demangle-cpp --suppress-aliases ' "$case_dir/log"
  exists "on Linux, brew is not run" no "$case_dir/brew"
}

# Where the report goes.
output_controls() {
  fixture discriminated "$tmp/g++-13/bin/g++-15"
  printf 'Clang\nsecond line\n' > "$b/Setup.txt"
  local reports=$repo/coverage_reports/TestAll/gcc-env-coverage
  mkdir -p "$reports/Clang"
  : > "$reports/Clang/stale.html"; : > "$reports/kept.html"
  run build/TestAll/gcc-env-coverage
  exits "a build with a Setup.txt succeeds" 0
  exists "a Setup.txt names a subdirectory of the report by its first line" yes "$reports/Clang/index.html"
  exists "an earlier report's page is deleted" no "$reports/Clang/stale.html"
  exists "a page beside the report's directory is kept" yes "$reports/kept.html"

  fixture escaping "$tmp/g++-13/bin/g++-15"
  printf '../..\n' > "$b/Setup.txt"
  reports=$repo/coverage_reports/TestAll/gcc-env-coverage
  mkdir -p "$reports"
  : > "$reports/kept.html"
  run build/TestAll/gcc-env-coverage
  exits "a Setup.txt naming .. is refused" 1
  check "a Setup.txt naming .. says so" yes \
        "^error: the report's directory, $repo/coverage_reports, is not within $repo/coverage_reports$" "$case_dir/err"
  exists "a Setup.txt naming .. deletes nothing" yes "$reports/kept.html"
  [ ! -s "$case_dir/log" ] || fail "a Setup.txt naming ..: a tool ran: $(head -1 "$case_dir/log")"

  fixture linked_report "$tmp/g++-13/bin/g++-15"
  mkdir -p "$case_dir/outside" "$repo/coverage_reports/TestAll"
  : > "$case_dir/outside/kept.html"
  ln -s "$case_dir/outside" "$repo/coverage_reports/TestAll/gcc-env-coverage"
  run build/TestAll/gcc-env-coverage
  exits "a report's directory linked from outside is refused" 1
  check "a report's directory linked from outside says so" yes \
        "^error: the report's directory, $case_dir/outside, is not within $repo/coverage_reports$" "$case_dir/err"
  exists "a report's directory linked from outside is kept" yes "$case_dir/outside/kept.html"
  [ ! -s "$case_dir/log" ] || fail "a report's directory linked from outside: a tool ran: $(head -1 "$case_dir/log")"

  fixture linked "$tmp/g++-13/bin/g++-15"
  mkdir -p "$case_dir/elsewhere"
  ln -s "$b" "$case_dir/elsewhere/link"
  run "$case_dir/elsewhere/link"
  exits "a build named through a link succeeds" 0
  check "a build named through a link is printed by its physical path" yes "^Test Dir: $b$" "$case_dir/out"
  exists "a build named through a link is reported where its physical path says" yes \
         "$repo/coverage_reports/TestAll/gcc-env-coverage/index.html"

  fixture elsewhere "$tmp/g++-13/bin/g++-15"
  from=$repo/build invoke=../scripts/generate_coverage_report.sh
  run TestAll/gcc-env-coverage
  calls_in_build
  exits "a run from another directory succeeds" 0
  called "a run from another directory finds check_tracefile.py beside the script" "$check_call /usr/*"
  exists "a run from another directory writes the same report" yes \
         "$repo/coverage_reports/TestAll/gcc-env-coverage/index.html"

  # The repository lies within a directory named build, which lies within one
  # whose name ends in build. So does the project's directory within build.
  fixture nested "$tmp/g++-13/bin/g++-15" prebuild/build/repo build/Rebuild/gcc-env-coverage
  run build/Rebuild/gcc-env-coverage
  exits "a repository within a build directory succeeds" 0
  exists "the report is placed by the last directory named build" yes \
         "$repo/coverage_reports/Rebuild/gcc-env-coverage/index.html"
}

# usage_refused <description>: the last run was refused with status 2, the
# usage on standard error alone, and no tool run.
usage_refused() {
  exits "$1 is refused" 2
  check "$1 gives the usage" yes '^Usage: .*generate_coverage_report\.sh <build directory>$' "$case_dir/err"
  [ ! -s "$case_dir/out" ] || fail "$1: the script wrote to standard output: $(head -1 "$case_dir/out")"
  [ ! -s "$case_dir/log" ] || fail "$1: a tool ran: $(head -1 "$case_dir/log")"
}

# What stops the script before any tool runs.
refusal_controls() {
  fixture no_argument "$tmp/g++-13/bin/g++-15"
  run
  usage_refused "no argument"

  fixture empty_argument "$tmp/g++-13/bin/g++-15"
  run ''
  usage_refused "an empty argument"

  fixture second_argument "$tmp/g++-13/bin/g++-15"
  run build/TestAll/gcc-env-coverage build/TestAll/gcc-env-coverage
  usage_refused "a second argument"

  fixture absent "$tmp/g++-13/bin/g++-15"
  run build/TestAll/absent
  exits "a missing build directory is refused" nonzero
  check "a missing build directory stops the script at once" no '^Test Dir:' "$case_dir/out"
  [ ! -s "$case_dir/log" ] || fail "a missing build directory: a tool ran: $(head -1 "$case_dir/log")"

  # A path with a directory whose name ends in build, and none named build.
  fixture unbuilt "$tmp/g++-13/bin/g++-15" repo prebuild/TestAll/gcc-env-coverage
  run prebuild/TestAll/gcc-env-coverage
  exits "a build directory not within build is refused" 2
  check "a build directory not within build says so" yes \
        "^error: $b is not within a directory named build$" "$case_dir/err"
  [ ! -s "$case_dir/log" ] || fail "a build directory not within build: a tool ran: $(head -1 "$case_dir/log")"

  local state
  for state in absent unexecutable; do
    fixture "darwin_cxxfilt_$state" "$tmp/llvm/bin/clang++"
    platform=Darwin cxxfilt=$state
    run build/TestAll/gcc-env-coverage
    exits "on Darwin, a GNU c++filt $state is refused" 1
    check "on Darwin, a GNU c++filt $state is named" yes \
          "^error: on macOS, genhtml needs GNU c\+\+filt at ${gnu_cxxfilt//+/\\+}$" "$case_dir/err"
    [ ! -s "$case_dir/log" ] || fail "on Darwin, a GNU c++filt $state: a tool ran: $(head -1 "$case_dir/log")"
  done

  fixture darwin_brewless "$tmp/llvm/bin/clang++"
  platform=Darwin cxxfilt=present homebrew=absent
  run build/TestAll/gcc-env-coverage
  exits "on Darwin, a failing brew is refused" 1
  check "on Darwin, a failing brew is named" yes \
        '^error: on macOS, genhtml needs GNU c\+\+filt from Homebrew, and brew --prefix failed$' "$case_dir/err"
  [ ! -s "$case_dir/log" ] || fail "on Darwin, a failing brew: a tool ran: $(head -1 "$case_dir/log")"

  fixture uncached "$tmp/g++-13/bin/g++-15"
  rm "$b/CMakeCache.txt"
  run build/TestAll/gcc-env-coverage
  exits "a build without a cache is refused" nonzero
  check "a build without a cache names it" yes 'CMakeCache\.txt' "$case_dir/err"
  [ ! -s "$case_dir/log" ] || fail "a build without a cache: a tool ran: $(head -1 "$case_dir/log")"
}

# failed <step> <error pattern> <prefix of the next step's call>
# Checks a run in which <step> failed: a non-zero status, an error on standard
# error, and no call of the next step.
failed() {
  exits "a failing $1 fails the script" nonzero
  check "a failing $1 is named" yes "$2" "$case_dir/err"
  [ -z "$3" ] || uncalled "a failing $1 stops the script" "$3"
}

failure_controls() {
  local step
  for step in cleandead zerocounters ctest capture remove summary check genhtml; do
    fixture "failing_$step" "$tmp/g++-13/bin/g++-15"
    failing=$step
    run build/TestAll/gcc-env-coverage
    case $step in
      cleandead)    failed "$step" "^error: exit status 9 from: $tmp/cache/ninja -C $b -t cleandead$" "lcov"
                    exists "a failing cleandead stops the sweep" yes "$b/CMakeFiles/T.dir/src/Orphan.cpp.gcno" ;;
      zerocounters) failed "$step" "^error: exit status 7 from: lcov --zerocounters --directory $b$" "ctest" ;;
      ctest)        failed "$step" "^error: exit status 8 from: ctest -T Test$" "lcov --directory"
                    exists "a failing suite makes no report directory" no "$repo/coverage_reports" ;;
      capture)      failed "$step" "^error: exit status 7 from: lcov --directory $b --capture " "lcov --remove" ;;
      remove)       failed "$step" "^error: exit status 7 from: lcov --remove " "lcov --summary" ;;
      summary)      failed "$step" "^error: exit status 7 from: lcov --summary $b/coverage.info " "check_tracefile.py"
                    check "a refused summary is shown" yes \
                          "^lcov: ERROR: the fixture's summary fails$" "$case_dir/all" ;;
      check)        failed "$step" "^error: exit status 6 from: python3 $repo/scripts/check_tracefile\.py --capture " \
                           "genhtml" ;;
      genhtml)      failed "$step" "^error: exit status 7 from: genhtml --demangle-cpp " "" ;;
    esac
  done

  # An earlier report which cannot be deleted.
  fixture failing_emptying "$tmp/g++-13/bin/g++-15"
  local locked=$repo/coverage_reports/TestAll/gcc-env-coverage/locked
  mkdir -p "$locked"
  : > "$locked/page.html"
  chmod a-w "$locked"
  run build/TestAll/gcc-env-coverage
  chmod u+w "$locked"
  failed "emptying" "^error: exit status [0-9]+ from: rm -rf $repo/coverage_reports/TestAll/gcc-env-coverage$" \
         "genhtml"

  # A notes file which cannot be deleted.
  fixture failing_rm "$tmp/g++-13/bin/g++-15"
  mkdir -p "$b/CMakeFiles/T.dir/locked"
  : > "$b/CMakeFiles/T.dir/locked/Locked.cpp.gcno"
  chmod a-w "$b/CMakeFiles/T.dir/locked"
  run build/TestAll/gcc-env-coverage
  chmod u+w "$b/CMakeFiles/T.dir/locked"
  failed rm "^error: exit status [0-9]+ from: rm $b/CMakeFiles/T.dir/locked/Locked\.cpp\.gcno$" "lcov"
}

# controls <script>
# Runs the controls against <script>, and counts in $fails those that fail.
controls() {
  fails=0
  cases=$tmp/cases
  chmod -R u+w "$cases" 2> /dev/null
  rm -rf "$cases"
  mkdir "$cases"
  subject=$1

  linux_gcc_controls
  darwin_clang_controls
  choice_controls
  output_controls
  refusal_controls
  failure_controls
}

# Each mutant breaks one behaviour that the controls claim. Its entry holds a
# description, the text it replaces, and the replacement. One mutant is left
# out, as equivalent: `$# -gt 1` for `$# -ne 1`, since `-z "$1"` refuses a
# missing argument.
mutations=(
  'a second argument accepted'        'if [[ $# -ne 1 || -z "$1" ]]; then'   'if [[ $# -lt 1 || -z "$1" ]]; then'
  'an empty argument accepted'        'if [[ $# -ne 1 || -z "$1" ]]; then'   'if [[ $# -ne 1 ]]; then'
  'a refused usage succeeds'          $'directory>" >&2\n  exit 2'           $'directory>" >&2\n  exit 0'
  'the usage on standard output'      '<build directory>" >&2'               '<build directory>"'
  'the logical path'                  '"$test_exe_dir_relative" && pwd -P)'  '"$test_exe_dir_relative" && pwd)'
  'a missing directory read as here'  '"$test_exe_dir_relative" && pwd -P)'  '"$test_exe_dir_relative"; pwd -P)'
  'a path without build accepted'     '!= */build/* ]]'                      '== "" ]]'
  'a name ending in build accepted'   '!= */build/* ]]'                      '!= *build/* ]]'
  'the refusal of a path succeeds'    $'named build" >&2\n  exit 2'          $'named build" >&2\n  exit 0'
  'the refusal on standard output'    'named build" >&2'                     'named build"'
  'the prefix to the first build'     '"${test_exe_dir%/build/*}"'           '"${test_exe_dir%%/build/*}"'
  'the suffix after the first build'  '"${test_exe_dir##*/build/}"'          '"${test_exe_dir#*/build/}"'
  'the prefix to a name ending build' '"${test_exe_dir%/build/*}"'           '"${test_exe_dir%build/*}"'
  'the suffix after a name ending build'  '"${test_exe_dir##*/build/}"'      '"${test_exe_dir##*build/}"'
  'the report within the build'       'output_dir="${path_prefix}/coverage_reports/${path_suffix}"'
                                      'output_dir="${test_exe_dir}/coverage_reports"'
  'the report directory made early'   'echo "Output Dir: ${output_dir}"'
                                      'echo "Output Dir: ${output_dir}"; mkdir -p "${output_dir}"'
  'an earlier report kept'            'run_checked rm -rf "${output_dir}"'   ':'
  'a failed emptying ignored'         'run_checked rm -rf "${output_dir}"'   'rm -rf "${output_dir}" 2> /dev/null || :'
  'any report directory emptied'      'if [[ "${physical_output}" != "${physical_reports}"/?* ]]; then'
                                      'if false; then'
  'coverage_reports itself emptied'   '"${physical_reports}"/?* ]]'          '"${physical_reports}"* ]]'
  'the logical path checked'          'physical_output=$(cd "${output_dir}" && pwd -P)'
                                      'physical_output=${output_dir}'
  'an escaping report succeeds'       $'${physical_reports}" >&2\n    exit 1'  $'${physical_reports}" >&2\n    exit 0'
  'an escaping report on standard output'  'is not within ${physical_reports}" >&2'
                                      'is not within ${physical_reports}"'
  'Setup.txt ignored'                 'if [[ -f "${setup_file}" ]]; then'    'if false; then'
  'Setup.txt read whole'              'head -n 1 "${setup_file}"'            'cat "${setup_file}"'
  'errors ignored'                    'set -e'                               'set +e'
  'a failed step ignored'             '"$@" && return'                       '"$@"; return'
  'a failed step exits 0'             $'>&2\n  exit 1\n}'                    $'>&2\n  exit 0\n}'
  'a failed step on standard output'  'from: $*" >&2'                        'from: $*"'
  'the status not named'              'local status=$?'                      'local status=1'
  'a missing cache read as empty'     $'//p\' "${test_exe_dir}/CMakeCache.txt")\nrun_checked'
                                      $'//p\' "${test_exe_dir}/CMakeCache.txt" 2> /dev/null || true)\nrun_checked'
  "PATH's ninja"                      'run_checked "${make_program}" -C'     'run_checked ninja -C'
  'cleandead not asked for'           '-t cleandead'                         '-t targets'
  'the build not given to ninja'      '"${make_program}" -C "${test_exe_dir}" -t'  '"${make_program}" -t'
  "an object named with .gcno's"      '[[ -f "${notes%.gcno}.o" ]]'          '[[ -f "${notes}.o" ]]'
  'notes with objects deleted'        '[[ -f "${notes%.gcno}.o" ]] ||'       '[[ -f "${notes%.gcno}.o" ]] &&'
  'an undeletable notes file kept'    'run_checked rm "${notes}"'            'rm -f "${notes}" 2> /dev/null || true'
  'no sweep'                          '[[ -f "${notes%.gcno}.o" ]] || run_checked rm "${notes}"'  ':'
  'notes at the top only'             'find "${test_exe_dir}" -name'         'find "${test_exe_dir}" -maxdepth 1 -name'
  'every profile file swept'          "-name '*.gcno'"                       "-name '*.gc*'"
  'counters not zeroed'               'run_checked lcov --zerocounters --directory "${test_exe_dir}"'  ':'
  'ctest outside the build'           'pushd "${test_exe_dir}"'              'pushd .'
  'the build left the working dir'    $'\npopd\n'                            $'\n:\n'
  'ctest without a dashboard'         'run_checked ctest -T Test'            'run_checked ctest'
  "the environment's gcov_tool honoured"  'case "${cxx##*/}" in'
                                      '[[ -n "${gcov_tool}" ]] || case "${cxx##*/}" in'
  "the compiler's entry not exact"    $'\'s/^CMAKE_CXX_COMPILER:[^=]*=//p\''  $'\'s/^CMAKE_CXX_COMPILER[^=]*=//p\''
  'g++-N not recognised'              '  g++-*)'                             '  gxx-*)'
  "gcov-N from PATH"                  'gcov_tool="${cxx%/*}/gcov-${cxx##*g++-}"'  'gcov_tool="gcov-${cxx##*g++-}"'
  'the version after the first g++-'  '${cxx##*g++-}'                        '${cxx#*g++-}'
  'clang++ not recognised'            '  clang++)'                           '  clang)'
  'the wrapper runs llvm-cov alone'   'llvm-cov" gcov "$@"'                  'llvm-cov" "$@"'
  "the wrapper's arguments split"     'gcov "$@"'                            'gcov $@'
  'llvm-cov from PATH'                '"${cxx%/*}" > "${gcov_tool}"'         '"" > "${gcov_tool}"'
  'the wrapper not executable'        'chmod +x "${gcov_tool}"'              ':'
  "another compiler's gcov beside it" '*)        gcov_tool="gcov"'           '*)        gcov_tool="${cxx%/*}/gcov"'
  'no --all'                          '--capture --all --output-file'        '--capture --output-file'
  'no gcov tool'                      '--capture --all --output-file "${capture}" --gcov-tool "${gcov_tool}"'
                                      '--capture --all --output-file "${capture}"'
  'the capture stops at an error'     '                 --keep-going --filter range'  '                 --filter range'
  'no range filter'                   '--keep-going --filter range --rc'     '--keep-going --rc'
  'unexecuted blocks counted'         ' --rc geninfo_unexecuted_blocks=1'    ''
  'the capture repairs'               '"${consistency_options[@]}" \'        '\'
  'consistency checked everywhere'    'consistency_options=(--rc check_data_consistency=0)'  'consistency_options=()'
  'empty an error at capture'         ' --ignore-errors empty --ignore-errors inconsistent'
                                      ' --ignore-errors inconsistent'
  'inconsistent an error at capture'  ' --ignore-errors inconsistent,inconsistent'  ''
  'format an error at capture'        ' --ignore-errors format,format'       ''
  'one tracefile for both'            'capture="${test_exe_dir}/coverage_capture.info"'
                                      'capture="${test_exe_dir}/coverage.info"'
  'end lines derived at reads'        '"${consistency_options[@]}" --rc derive_function_end_line=0)'
                                      '"${consistency_options[@]}")'
  '/usr kept'                         "foreign=('/usr/*')"                   'foreign=()'
  'Darwin taken for Linux'            $'== Darwin ]]; then\n  foreign+='    $'== Linux ]]; then\n  foreign+='
  'Homebrew kept'                     "foreign+=('/opt/homebrew/*' "         "foreign+=("
  'the CommandLineTools kept'         "'/Library/Developer/*' "              ''
  'Xcode kept'                        " '/Applications/Xcode.app/*')"        ')'
  'an unused pattern an error'        '  remove_options+=(--ignore-errors unused)'  '  :'
  'the removal stops at an error'     'remove_options=(--keep-going '        'remove_options=('
  'empty an error at removal'         'remove_options=(--keep-going --ignore-errors empty --ignore-errors format)'
                                      'remove_options=(--keep-going --ignore-errors format)'
  'format an error at removal'        '--ignore-errors empty --ignore-errors format)'  '--ignore-errors empty)'
  'the patterns expanded'             '"${capture}" "${foreign[@]}" --output-file'
                                      '"${capture}" ${foreign[@]} --output-file'
  'the removal repairs'               '"${remove_options[@]}" "${read_options[@]}"'  '"${remove_options[@]}"'
  'a refused summary unnamed'         'run_checked lcov --summary'           'lcov --summary'
  "lcov's reason for a refusal hidden"  '"${read_options[@]}" > "${summary}"'
                                      '"${read_options[@]}" > "${summary}" 2> /dev/null'
  'the summary not kept'              '"${read_options[@]}" > "${summary}"'  '"${read_options[@]}"'
  'the summary not printed'           $'> "${summary}"\ncat "${summary}"'    $'> "${summary}"'
  'the summary repairs'               'lcov --summary "${info}" "${read_options[@]}"'  'lcov --summary "${info}"'
  'the check told of /usr alone'      '--removed "${foreign[@]}"'            "--removed '/usr/*'"
  'the check not run'                 'run_checked python3 "${script_dir}/check_tracefile.py"'
                                      ': python3 "${script_dir}/check_tracefile.py"'
  'the check from the working dir'    'script_dir=$(cd "$(dirname "$0")" && pwd -P)'  'script_dir=scripts'
  'GNU c++filt never given'           '  demangle+=("${gnu_cxxfilt}")'       '  :'
  'GNU c++filt on Linux too'          $'== Darwin ]]; then\n  if ! homebrew'
                                      $'== Linux || true ]]; then\n  if ! homebrew'
  'brew asked on Linux'               'platform=$(uname -s)'
                                      'platform=$(uname -s); brew --prefix > /dev/null'
  'a failing brew accepted'           'if ! homebrew=$(brew --prefix); then'
                                      'if ! homebrew=$(brew --prefix || :); then'
  'a failing brew succeeds'           $'--prefix failed" >&2\n    exit 1'    $'--prefix failed" >&2\n    exit 0'
  "a failing brew on standard output"  'brew --prefix failed" >&2'          'brew --prefix failed"'
  "Homebrew's prefix assumed"         'gnu_cxxfilt="${homebrew}/opt'         'gnu_cxxfilt="/opt/homebrew/opt'
  'a missing c++filt accepted'        'if [[ ! -x "${gnu_cxxfilt}" ]]; then'  'if false; then'
  'an unexecutable c++filt accepted'  '! -x "${gnu_cxxfilt}"'                '! -e "${gnu_cxxfilt}"'
  'a missing c++filt succeeds'        $'${gnu_cxxfilt}" >&2\n    exit 1'     $'${gnu_cxxfilt}" >&2\n    exit 0'
  'a missing c++filt on standard output'  'needs GNU c++filt at ${gnu_cxxfilt}" >&2'
                                      'needs GNU c++filt at ${gnu_cxxfilt}"'
  'no demangling'                     'run_checked genhtml "${demangle[@]}"'  'run_checked genhtml'
  'aliases kept'                      ' --suppress-aliases'                  ''
  'range an error in the report'     ' --ignore-errors range --ignore-errors empty'  ' --ignore-errors empty'
  'empty an error in the report'      '--ignore-errors range --ignore-errors empty'  '--ignore-errors range'
  'category an error in the report'   ' --ignore-errors category "'          ' "'
  'genhtml probed first'              $'\nrun_checked genhtml'
                                      $'\ngenhtml -o "${output_dir}" /dev/null 2>&1 | :\nrun_checked genhtml'
  'genhtml repairs'                   '--ignore-errors category "${read_options[@]}"'  '--ignore-errors category'
  "genhtml's failure unnamed"         'run_checked genhtml "${demangle[@]}"'  'genhtml "${demangle[@]}"'
)

if [ "$*" != --mutations ]; then
  controls "$script"
  if [ "$fails" -eq 0 ]; then echo "generate_coverage_report: all controls pass"; else exit 1; fi
  exit 0
fi

source_text=$(cat "$script")
controls "$script" > "$tmp/controls.txt" 2>&1
if [ "$fails" -ne 0 ]; then
  cat "$tmp/controls.txt"
  echo "generate_coverage_report: the unmutated script fails $fails of its controls, so no mutant was run"
  exit 1
fi
if [ $((${#mutations[@]} % 3)) -ne 0 ]; then
  echo "generate_coverage_report: the mutations array does not hold whole entries of three"; exit 1
fi
survivors=0
i=0
while [ "$i" -lt ${#mutations[@]} ]; do
  description=${mutations[i]} old=${mutations[i+1]} new=${mutations[i+2]}
  i=$((i+3))
  after=${source_text#*"$old"}
  if [ "$after" = "$source_text" ] || [ "${after#*"$old"}" != "$after" ]; then
    echo "$description: the text to mutate does not occur exactly once  <-- SURVIVED"
    survivors=$((survivors+1))
    continue
  fi
  printf '%s\n' "${source_text%%"$old"*}$new$after" > "$tmp/mutant.sh"
  controls "$tmp/mutant.sh" > "$tmp/controls.txt" 2>&1
  echo "$description: $fails controls fail$([ "$fails" -ne 0 ] || echo '  <-- SURVIVED')"
  [ "$fails" -ne 0 ] || survivors=$((survivors+1))
done
mutants=$((${#mutations[@]} / 3))
if [ "$survivors" -ne 0 ]; then
  echo "generate_coverage_report: $survivors of $mutants mutants survived"
  exit 1
fi
echo "generate_coverage_report: $mutants mutants, every one killed"
