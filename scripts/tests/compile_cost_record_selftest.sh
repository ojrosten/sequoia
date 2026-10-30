#!/bin/bash
# Controls for compile_cost_record.py, and a mutation check of the controls.
#
#   compile_cost_record_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the script and against each mutant of the script.
# The script must fail no control, and each mutant at least one.
#
# The controls build their own fixture: a git repository, and within it a
# build directory whose CMakeCache.txt and compiler record the selftest
# writes. Stand-ins answer for ninja, for valgrind and for the compiler, so
# the selftest needs neither CMake, ninja nor valgrind:
#   - the stand-in ninja answers `-t compdb` from a file of the build
#     directory;
#   - the stand-in valgrind prints a version, or records its arguments, runs
#     the command which follows its options, and leaves the files of counts
#     which the control put beside the source's name. Each file has the shape of one which
#     cachegrind 3.26 wrote for a compilation by g++-15 (run 41050414618);
#   - the stand-in compiler records its directory and arguments, writes to
#     each file which its -o and -MF name, and exits with the status the
#     control set.
# The claims:
#   - the record holds the commit and whether the tree is clean; a toolchain
#     of the compiler and the standard library which the build directory's
#     compiler record names, the compiler's implicit include directories,
#     valgrind's path and version, and the cache's build type and C++ flags;
#     and in compile_cost, the boundary, and for each source the
#     instructions of the files of counts its compilation left, summed,
#     keyed by the source as given;
#   - each compilation sees PATH, LC_ALL and TMPDIR alone, whatever the
#     caller's environment;
#   - a compiler which the compiler record names Clang is given
#     --no-default-config first, and another compiler nothing;
#   - each source is compiled once, in the compilation's directory, by its
#     command with the files of -o and of -MF, where there is one, replaced
#     by /dev/null, so that the build directory gains no file; valgrind is
#     given the options the boundary names;
#   - a source is matched to its compilation by the path both resolve to;
#   - each of these is refused, with a message naming it, and no record: two
#     sources which resolve to one file, a source absent, or outside the
#     repository; no compilation of a source, or two; a command which runs
#     another compiler, or has no -o, two, or two -MF, or ends with -o; a
#     compilation which fails; no file of counts, a file with no
#     summary or two, or a summary which is no count; no instructions; no
#     compiler record of the cache's CMake; a failure of ninja, or a database
#     which is not JSON; valgrind absent, or unable to start;
#   - no source is refused as usage.
#
# A control which checks that something is absent also checks that the run
# succeeded, and that something beside it is present.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script="$here/../compile_cost_record.py"
tmp=$(cd "$(mktemp -d)" && pwd -P); trap 'rm -rf "$tmp"' EXIT
verbose=yes
python=$(command -v python3)

fail() {
  fails=$((fails+1))
  if [[ $verbose == yes ]]; then echo "FAIL: $*"; fi
}

# run <argument>...: the subject's output in $out, its errors in $err, and its
# exit status in $rc. The subject imports symbol_table_report.py from the
# scripts directory, wherever the subject is.
run() { PATH="$work/bin:$tmp/bin:$PATH" PYTHONPATH="$here/.." "$python" -B "$subject" "$@" > "$out" 2> "$err"; rc=$?; }

check() { # check <name> <yes|no> <pattern> [file, by default $err]
  local got
  total=$((total+1))
  if grep -qsE -- "$3" "${4:-$err}"; then got=yes; else got=no; fi
  if [[ $got != "$2" ]]; then fail "$1 (expected $2, got $got, pattern: $3)"; fi
}

exits() { # exits <name> <status|nonzero>
  total=$((total+1))
  if [[ $2 == nonzero ]]; then
    if [[ $rc -eq 0 ]]; then fail "$1 (expected a non-zero exit, got 0)"; fi
  elif [[ $rc -ne $2 ]]; then fail "$1 (expected exit $2, got $rc: $(cat "$err"))"; fi
}

refuses() { # refuses <name> <pattern>: the run failed, saying why, and wrote no field
  exits "$1 is refused" nonzero
  check "$1 says why" yes "$2"
  total=$((total+1))
  if [[ -s $out ]]; then fail "$1 wrote a field"; fi
}

field() { # field <name> <python expression over the field d> <expected>
  local got
  total=$((total+1))
  got=$(python3 -c 'import json, sys
d = json.load(open(sys.argv[1]))
print(eval(sys.argv[2]))' "$out" "$2" 2>&1)
  if [[ $got != "$3" ]]; then fail "$1 (expected $3, got $got)"; fi
}

# The stand-ins --------------------------------------------------------------

mkdir -p "$tmp/bin" "$tmp/templates"

# The stand-in ninja: answers `-C <dir> -t compdb` from <dir>/ninja/compdb,
# and exits with the status in <dir>/ninja/status, if there is one.
cat > "$tmp/bin/ninja" <<'EOF'
#!/bin/bash
[[ $1 == -C && $3 == -t && $4 == compdb && $# -eq 4 ]] \
  || { echo "stand-in ninja: unexpected arguments: $*" >&2; exit 2; }
cat "$2/ninja/compdb"
exit "$(cat "$2/ninja/status" 2>/dev/null || echo 0)"
EOF

# The stand-in valgrind: appends its options, one call to a line, to
# @LOG@/valgrind, runs the command which follows them, and copies each file
# of @COUNTS@/<the command's last word's name> to the path its
# --cachegrind-out-file names, its %p replaced by the file's name. The
# script gives a compilation an environment of its own, so the fixture writes
# its paths into each stand-in, in place of @LOG@ and @COUNTS@.
cat > "$tmp/templates/valgrind" <<'EOF'
#!/bin/bash
if [[ $* == --version ]]; then echo valgrind-fixture-3.26; exit 0; fi
options=()
while [[ $1 == --* ]]; do options+=("$1"); shift; done
echo "${options[*]}" >> @LOG@/valgrind
pattern=''
for option in "${options[@]}"; do
  [[ $option == --cachegrind-out-file=* ]] && pattern=${option#--cachegrind-out-file=}
done
"$@"; status=$?
source=${*: -1}
for counts in @COUNTS@/"${source##*/}"/*; do
  [ -e "$counts" ] && cp "$counts" "${pattern/\%p/${counts##*/}}"
done
exit $status
EOF

# The stand-in compiler: appends its directory and arguments to @LOG@/cc,
# writes its environment to @LOG@/cc-environment, writes to the files which
# its -o and -MF name, and exits with the status in @LOG@/cc-status, if there
# is one.
cat > "$tmp/templates/fixture-cc" <<'EOF'
#!/bin/bash
echo "$PWD $*" >> @LOG@/cc
env > @LOG@/cc-environment
while [ $# -gt 0 ]; do
  case $1 in -o|-MF) [ $# -gt 1 ] && echo written > "$2"; shift;; esac
  shift
done
exit "$(cat @LOG@/cc-status 2>/dev/null || echo 0)"
EOF
chmod +x "$tmp/bin/ninja"

# The fixture -----------------------------------------------------------------

# counts <file> <instructions>: a file of counts with the shape of
# cachegrind's, whose summary is <instructions>
counts() {
  mkdir -p "$(dirname "$1")"
  cat > "$1" <<EOF
cmd: /usr/libexec/gcc/x86_64-linux-gnu/15/cc1plus -quiet fixture.cpp
events: Ir
fl=./csu/../csu/libc-start.c
fn=__libc_start_main@@GLIBC_2.34
128 2
134 3
fn=???
0 952897
summary: $2
EOF
}

# compilation <directory> <file> <arguments>: an entry of the compilation
# database, compiled by the stand-in compiler
compilation() {
  printf '{"directory": "%s", "command": "%s %s", "file": "%s", "output": "x.o"}' "$1" "$work/bin/fixture-cc" "$3" "$2"
}

# fixture: a repository at $repo holding a build directory at $build, whose
# compilation database holds a compilation of each source the controls name
fixture() {
  repo="$work/repo"; build="$repo/build/TestAll/fixture"
  mkdir -p "$repo/TestAll" "$repo/Tests" "$build/ninja" "$build/CMakeFiles/TestAll.dir"
  local source
  for source in A B Twice NoOutput TwoOutputs TwoDependencyFiles EndsWithOutput OtherCompiler Unknown Other/A; do
    mkdir -p "$(dirname "$repo/Tests/$source.cpp")"
    echo "// $source" > "$repo/Tests/$source.cpp"
  done
  echo '// outside' > "$work/Outside.cpp"
  printf '/build\n' > "$repo/.gitignore"
  git -C "$repo" init -q
  git -C "$repo" add -A
  git -C "$repo" -c user.name=fixture -c user.email=fixture@example.com commit -q -m fixture
  cat > "$build/CMakeCache.txt" <<EOF
CMAKE_GENERATOR:INTERNAL=Ninja
CMAKE_MAKE_PROGRAM:FILEPATH=$tmp/bin/ninja
CMAKE_HOME_DIRECTORY:INTERNAL=$repo/TestAll
CMAKE_CACHE_MAJOR_VERSION:INTERNAL=4
CMAKE_CACHE_MINOR_VERSION:INTERNAL=4
CMAKE_CACHE_PATCH_VERSION:INTERNAL=3
CMAKE_BUILD_TYPE:STRING=Debug
CMAKE_CXX_FLAGS:STRING=-fixture-flag
CMAKE_CXX_FLAGS_DEBUG:STRING=-g
CMAKE_EXE_LINKER_FLAGS:STRING=-not-recorded
EOF
  mkdir -p "$build/CMakeFiles/4.4.3"
  cat > "$build/CMakeFiles/4.4.3/CMakeCXXCompiler.cmake" <<EOF
set(CMAKE_CXX_COMPILER "$work/bin/fixture-cc")
set(CMAKE_CXX_COMPILER_ID "Fixture")
set(CMAKE_CXX_COMPILER_VERSION "23.1.4")
set(CMAKE_CXX_COMPILER_LINKER "/fixture/ld")
set(CMAKE_CXX_COMPILER_LINKER_ID "FixtureLD")
set(CMAKE_CXX_COMPILER_LINKER_VERSION "4.5")
set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "$repo/Tests;$repo/TestAll")
set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "stdc++")
EOF
  local object=CMakeFiles/TestAll.dir
  # B's file is relative to the directory, as ninja may write it. Other/A.cpp
  # shares A.cpp's name, and has no file of counts.
  { printf '['
    compilation "$build" "$repo/Tests/A.cpp" "-DX -MD -MT $object/A.o -MF $object/A.o.d -o $object/A.o -c $repo/Tests/A.cpp"
    printf ',\n'
    compilation "$build" "../../../Tests/B.cpp" "-DY -o $object/B.o -c $repo/Tests/B.cpp"
    printf ',\n'
    compilation "$build" "$repo/Tests/Other/A.cpp" "-DZ -o $object/OtherA.o -c $repo/Tests/Other/A.cpp"
    printf ',\n'
    compilation "$build" "$repo/Tests/Twice.cpp" "-o $object/T1.o -c $repo/Tests/Twice.cpp"
    printf ',\n'
    compilation "$build" "$repo/Tests/Twice.cpp" "-o $object/T2.o -c $repo/Tests/Twice.cpp"
    printf ',\n'
    compilation "$build" "$repo/Tests/NoOutput.cpp" "-c $repo/Tests/NoOutput.cpp"
    printf ',\n'
    compilation "$build" "$repo/Tests/TwoOutputs.cpp" "-o $object/O1.o -o $object/O2.o -c $repo/Tests/TwoOutputs.cpp"
    printf ',\n'
    compilation "$build" "$repo/Tests/TwoDependencyFiles.cpp" \
                "-MF $object/D1.d -MF $object/D2.d -o $object/D.o -c $repo/Tests/TwoDependencyFiles.cpp"
    printf ',\n'
    compilation "$build" "$repo/Tests/EndsWithOutput.cpp" "-c $repo/Tests/EndsWithOutput.cpp -o"
    printf ',\n{"directory": "%s", "command": "%s -o x.o -c %s", "file": "%s", "output": "x.o"}' \
      "$build" "$work/bin/fixture-cc-other" "$repo/Tests/OtherCompiler.cpp" "$repo/Tests/OtherCompiler.cpp"
    printf ']\n'
  } > "$build/ninja/compdb"

  FIXTURE_LOG="$work/log" FIXTURE_COUNTS="$work/counts"
  mkdir -p "$FIXTURE_LOG" "$work/bin"
  local stand_in
  for stand_in in valgrind fixture-cc; do
    sed "s|@LOG@|'$FIXTURE_LOG'|g; s|@COUNTS@|'$FIXTURE_COUNTS'|g" "$tmp/templates/$stand_in" > "$work/bin/$stand_in"
    chmod +x "$work/bin/$stand_in"
  done
  # A's compilation runs three processes, and B's one, with counts whose
  # digits differ in every place, so that a process dropped or counted twice
  # shows.
  counts "$FIXTURE_COUNTS/A.cpp/101" 7000000000
  counts "$FIXTURE_COUNTS/A.cpp/102" 400000
  counts "$FIXTURE_COUNTS/A.cpp/103" 21
  counts "$FIXTURE_COUNTS/B.cpp/201" 800000000
  local name
  for name in Twice NoOutput TwoOutputs TwoDependencyFiles EndsWithOutput OtherCompiler; do
    counts "$FIXTURE_COUNTS/$name.cpp/1" 5
  done
}

# The controls ----------------------------------------------------------------

# compilation_environment: the variables of the last compilation's
# environment, sorted, on one line, without PWD, SHLVL and _, which the
# stand-ins' bash sets itself
compilation_environment() {
  grep -svE '^(PWD|SHLVL|_)=' "$FIXTURE_LOG/cc-environment" | LC_ALL=C sort | tr '\n' ' ' | sed 's/ $//'
  echo
}

field_controls() {
  fixture
  run "$build" Tests/A.cpp Tests/B.cpp
  exits "a record is written" 0
  field "the commit" 'd["commit"]' "$(git -C "$repo" rev-parse HEAD)"
  field "a clean tree" 'd["clean"]' True
  field "the compiler, from the compiler record" 'sorted(d["toolchain"]["compiler"].items())' \
        "[('id', 'Fixture'), ('path', '$work/bin/fixture-cc'), ('version', '23.1.4')]"
  field "the standard library, and the implicit include directories" 'd["toolchain"]["standard_library"]' \
        "{'implicit_include_directories': ['$repo/Tests', '$repo/TestAll'], 'name': 'libstdc++'}"
  field "valgrind's path and version" 'd["toolchain"]["valgrind"]' \
        "{'path': '$work/bin/valgrind', 'version': 'valgrind-fixture-3.26'}"
  field "the build type and C++ flags, and no others" 'sorted(d["toolchain"]["build"].items())' \
        "[('CMAKE_BUILD_TYPE', 'Debug'), ('CMAKE_CXX_FLAGS', '-fixture-flag'), ('CMAKE_CXX_FLAGS_DEBUG', '-g')]"
  field "the toolchain, and nothing else" 'sorted(d["toolchain"])' "['build', 'compiler', 'standard_library', 'valgrind']"
  field "the boundary" 'd["compile_cost"]["boundary"]' \
    "instructions under valgrind --tool=cachegrind --cache-sim=no --trace-children=yes, summed over the processes of the compilation which ninja's compilation database holds, its outputs to /dev/null, Clang given --no-default-config first, in an environment of PATH=/usr/bin:/bin, LC_ALL=C, TMPDIR=/tmp alone"
  field "the sources, keyed as given" 'sorted(d["compile_cost"]["units"])' "['Tests/A.cpp', 'Tests/B.cpp']"
  field "a compilation's processes, summed" 'd["compile_cost"]["units"]["Tests/A.cpp"]' 7000400021
  field "a compilation of one process" 'd["compile_cost"]["units"]["Tests/B.cpp"]' 800000000
  field "the cost, and nothing else" 'sorted(d["compile_cost"])' "['boundary', 'units']"
  field "nothing else" 'sorted(d)' "['clean', 'commit', 'compile_cost', 'toolchain']"

  total=$((total+1))
  [[ $(cat "$FIXTURE_LOG/cc" 2> /dev/null | wc -l) -eq 2 ]] || fail "each source compiled once: $(cat "$FIXTURE_LOG/cc" 2> /dev/null)"
  check "A's command, its outputs to /dev/null, in its directory" yes \
        "^$build -DX -MD -MT CMakeFiles/TestAll.dir/A.o -MF /dev/null -o /dev/null -c $repo/Tests/A.cpp\$" \
        "$FIXTURE_LOG/cc"
  check "B's command, which names no dependency file" yes "^$build -DY -o /dev/null -c $repo/Tests/B.cpp\$" \
        "$FIXTURE_LOG/cc"
  total=$((total+1))
  [[ -z $(ls -A "$build/CMakeFiles/TestAll.dir") ]] \
    || fail "the build directory gained files: $(ls -A "$build/CMakeFiles/TestAll.dir")"
  check "valgrind is given cachegrind, no cache simulation, and the children" yes \
        '^--tool=cachegrind --cache-sim=no --trace-children=yes --cachegrind-out-file=[^ ]*/%p$' "$FIXTURE_LOG/valgrind"

  check "the compilation's environment, its fixed variables alone" yes \
        '^LC_ALL=C PATH=/usr/bin:/bin TMPDIR=/tmp$' <(compilation_environment)
  FIXTURE_PADDING=xxxxxxxxxx LC_ALL=fr_FR.UTF-8 TMPDIR="$work" run "$build" Tests/B.cpp
  exits "a run with another environment" 0
  check "the caller's environment, its padding, locale and TMPDIR, is not the compilation's" yes \
        '^LC_ALL=C PATH=/usr/bin:/bin TMPDIR=/tmp$' <(compilation_environment)

  # A compiler record naming Clang: clang is given --no-default-config, so that
  # it reads no configuration file, and the command's own words follow.
  sed -i.bak 's/^set(CMAKE_CXX_COMPILER_ID "Fixture")$/set(CMAKE_CXX_COMPILER_ID "Clang")/' \
    "$build/CMakeFiles/4.4.3/CMakeCXXCompiler.cmake"
  run "$build" Tests/B.cpp
  exits "a record of Clang's compilation" 0
  check "clang reads no configuration file" yes "^$build --no-default-config -DY -o /dev/null -c $repo/Tests/B.cpp\$" \
        "$FIXTURE_LOG/cc"
  mv "$build/CMakeFiles/4.4.3/CMakeCXXCompiler.cmake.bak" "$build/CMakeFiles/4.4.3/CMakeCXXCompiler.cmake"

  run "$build" Tests/B.cpp
  exits "one source" 0
  field "one source, and no other" 'd["compile_cost"]["units"]' "{'Tests/B.cpp': 800000000}"

  echo '// untracked' > "$repo/Tests/Untracked.cpp"
  run "$build" Tests/B.cpp
  exits "a record of a tree with an untracked file" 0
  field "an untracked file makes the tree dirty" 'd["clean"]' False
  rm "$repo/Tests/Untracked.cpp"
}

source_controls() {
  fixture
  run "$build" Tests/A.cpp Tests/A.cpp
  refuses "a source given twice" 'two of the sources resolve to one file: Tests/A.cpp Tests/A.cpp'
  run "$build" Tests/A.cpp Tests/../Tests/A.cpp
  refuses "a source given twice, spelt two ways" 'two of the sources resolve to one file: Tests/A.cpp Tests/../Tests/A.cpp'
  run "$build" Tests/Absent.cpp
  refuses "an absent source" 'Tests/Absent\.cpp is not a file within the repository'
  run "$build" Tests
  refuses "a directory for a source" 'Tests is not a file within the repository'
  run "$build" ../Outside.cpp
  refuses "a source outside the repository" '\.\./Outside\.cpp is not a file within the repository'
  run "$build"
  exits "no source is refused as usage" 2
  check "no source says how to call" yes 'usage:'

  run "$build" Tests/Unknown.cpp
  refuses "a source with no compilation" 'has 0 compilations of Tests/Unknown\.cpp, rather than one'
  run "$build" Tests/Twice.cpp
  refuses "a source with two compilations" 'has 2 compilations of Tests/Twice\.cpp, rather than one'
  total=$((total+1))
  [[ ! -s $FIXTURE_LOG/cc ]] || fail "a refused source was compiled: $(cat "$FIXTURE_LOG/cc")"
}

command_controls() {
  fixture
  run "$build" Tests/NoOutput.cpp
  refuses "a command with no -o" 'NoOutput\.cpp has 0 -o options, rather than one:'
  run "$build" Tests/TwoOutputs.cpp
  refuses "a command with two -o" 'TwoOutputs\.cpp has 2 -o options, rather than one:'
  run "$build" Tests/TwoDependencyFiles.cpp
  refuses "a command with two -MF" 'TwoDependencyFiles\.cpp has 2 -MF options, rather than none or one:'
  run "$build" Tests/EndsWithOutput.cpp
  refuses "a command ending with -o" 'EndsWithOutput\.cpp ends with -o, which names no file:'
  run "$build" Tests/OtherCompiler.cpp
  refuses "a command running another compiler" "OtherCompiler\\.cpp does not run the compiler record's compiler"
  total=$((total+1))
  [[ ! -s $FIXTURE_LOG/cc ]] || fail "a refused command was run: $(cat "$FIXTURE_LOG/cc")"

  echo 3 > "$FIXTURE_LOG/cc-status"
  run "$build" Tests/A.cpp
  refuses "a compilation which fails" 'the compilation of Tests/A\.cpp failed under valgrind with exit status 3'
  rm "$FIXTURE_LOG/cc-status"

  run "$build" Tests/A.cpp Tests/B.cpp
  exits "the fixture is restored" 0
}

counts_controls() {
  fixture
  rm -r "$FIXTURE_COUNTS/B.cpp"
  run "$build" Tests/B.cpp
  refuses "no file of counts" 'valgrind left no file of counts'

  counts "$FIXTURE_COUNTS/B.cpp/201" 800000000
  grep -v '^summary:' "$FIXTURE_COUNTS/B.cpp/201" > "$work/no-summary"
  mv "$work/no-summary" "$FIXTURE_COUNTS/B.cpp/201"
  run "$build" Tests/B.cpp
  refuses "a file with no summary" 'has 0 summary lines, rather than one'

  counts "$FIXTURE_COUNTS/B.cpp/201" 800000000
  echo 'summary: 800000000' >> "$FIXTURE_COUNTS/B.cpp/201"
  run "$build" Tests/B.cpp
  refuses "a file with two summaries" 'has 2 summary lines, rather than one'

  local summary
  for summary in 8e8 -800000000 '' '800000000 12'; do
    counts "$FIXTURE_COUNTS/B.cpp/201" "$summary"
    run "$build" Tests/B.cpp
    refuses "a summary of '$summary'" "has a summary which is no count of instructions: '$summary'"
  done

  counts "$FIXTURE_COUNTS/B.cpp/201" 0
  run "$build" Tests/B.cpp
  refuses "no instructions" 'valgrind counted no instructions'

  # A count of zero beside a count which is not is no refusal: the files are
  # summed before the total is judged.
  counts "$FIXTURE_COUNTS/B.cpp/202" 800000000
  run "$build" Tests/B.cpp
  exits "one process of no instructions beside one of some" 0
  field "the processes' instructions summed" 'd["compile_cost"]["units"]["Tests/B.cpp"]' 800000000
}

ninja_controls() {
  fixture
  mv "$build/CMakeFiles/4.4.3" "$build/CMakeFiles/4.1.2"
  run "$build" Tests/A.cpp
  refuses "a build directory with no compiler record of its CMake" 'holds 0 C\+\+ compiler records for CMake 4\.4\.3'
  mv "$build/CMakeFiles/4.1.2" "$build/CMakeFiles/4.4.3"

  echo 4 > "$build/ninja/status"
  run "$build" Tests/A.cpp
  refuses "a failure of ninja" 'failed with exit status 4'
  rm "$build/ninja/status"

  echo 'not json' > "$build/ninja/compdb"
  run "$build" Tests/A.cpp
  refuses "a database which is not JSON" "ninja's compilation database for .* cannot be read"

  # The run's PATH holds git and cat alone, which the script and the stand-in
  # ninja need, so that no valgrind of the machine's is found.
  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  mkdir "$work/no-valgrind"
  ln -s "$(command -v git)" "$work/no-valgrind/git"
  ln -s "$(command -v cat)" "$work/no-valgrind/cat"
  PATH="$work/no-valgrind" PYTHONPATH="$here/.." "$python" -B "$subject" "$build" Tests/A.cpp > "$out" 2> "$err"; rc=$?
  refuses "valgrind absent" 'valgrind could not be run: there is none on PATH'

  # A valgrind whose interpreter is absent is found on PATH, and fails to
  # start.
  printf '#!/absent/interpreter\n' > "$work/bin/valgrind"
  run "$build" Tests/A.cpp
  refuses "a valgrind which cannot start" 'valgrind could not be run: .*No such file'
}

run_controls() { # run_controls <script>: sets fails, and total, the controls run
  subject=$1 fails=0 total=0
  local control
  for control in field_controls source_controls command_controls counts_controls ninja_controls; do
    work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
    "$control"
  done
}

# Each mutant breaks one behaviour, and is (description, old text, new text).
mutants() { # mutants <dir>: writes <dir>/<n>/compile_cost_record.py, and
            # prints a line "<n> <occurrences of the old text> <description>"
  python3 - "$script" "$1" <<'EOF'
import os, sys
MUTATIONS = [
    ('the object written',               'OUTPUT_OPTIONS = {"-o": 1, "-MF": 0}', 'OUTPUT_OPTIONS = {"-MF": 0}'),
    ('the dependency file written',      'OUTPUT_OPTIONS = {"-o": 1, "-MF": 0}', 'OUTPUT_OPTIONS = {"-o": 1}'),
    ('no -o accepted',                   'OUTPUT_OPTIONS = {"-o": 1,', 'OUTPUT_OPTIONS = {"-o": 0,'),
    ('two options accepted',             'if count < least or count > 1:', 'if count < least or count > 2:'),
    ('an option at the end accepted',    'if words[-1] == option:', 'if False:'),
    ('the option itself replaced',       'os.devnull if i > 0 and words[i - 1] in OUTPUT_OPTIONS else word',
                                         'os.devnull if word in OUTPUT_OPTIONS else word'),
    ('the word after any option replaced', 'words[i - 1] in OUTPUT_OPTIONS else word', 'words[i - 1].startswith("-") else word'),
    ('another compiler accepted',        'if not words or words[0] != compiler["path"]:', 'if not words:'),
    ('clang\'s configuration read',      'COMPILER_OPTIONS = {"Clang": ["--no-default-config"]}', 'COMPILER_OPTIONS = {}'),
    ('every compiler given clang\'s options', 'COMPILER_OPTIONS.get(compiler["id"], [])', 'COMPILER_OPTIONS["Clang"]'),
    ('clang\'s options after the command\'s', 'return [discarded[0], *COMPILER_OPTIONS.get(compiler["id"], []), *discarded[1:]]',
                                         'return [*discarded, *COMPILER_OPTIONS.get(compiler["id"], [])]'),
    ('a source matched by its name',     'if os.path.realpath(os.path.join(compilation["directory"], '
                                         'compilation["file"])) == path]',
                                         'if os.path.basename(compilation["file"]) == os.path.basename(path)]'),
    ('a file not resolved from its directory',
                                         'os.path.realpath(os.path.join(compilation["directory"], compilation["file"]))',
                                         'os.path.realpath(compilation["file"])'),
    ('the first of two compilations',    'if len(found) != 1:\n        sys.exit(f"ninja', 'if not found:\n        sys.exit(f"ninja'),
    ('no compilation crashes',           'if len(found) != 1:\n        sys.exit(f"ninja', 'if len(found) > 1:\n        sys.exit(f"ninja'),
    ('no files accepted',                'if not names:\n        sys.exit', 'if False:\n        sys.exit'),
    ('the first file alone',             'total = sum(map(instructions_in, names))', 'total = instructions_in(names[0])'),
    ('the largest file alone',           'total = sum(map(instructions_in, names))', 'total = max(map(instructions_in, names))'),
    ('the first summary of several',     'if len(summaries) != 1:', 'if not summaries:'),
    ('any summary accepted',             'if not summaries[0].isdecimal():', 'if False:'),
    ('zero accepted',                    'if total == 0:\n        sys.exit', 'if False:\n        sys.exit'),
    ('a zero process refused',           'return int(summaries[0])',
                                         'return int(summaries[0]) or sys.exit("valgrind counted no instructions")'),
    ('the cache simulated',              '"--cache-sim=no", ', ''),
    ('the children not followed',        ', "--trace-children=yes"]', ']'),
    ('callgrind run',                    '"--tool=cachegrind"', '"--tool=callgrind"'),
    ('run in the wrong directory',       'cwd=compilation["directory"], ', ''),
    ('a failure accepted',               'if r.returncode:\n            sys.exit(f"the compilation',
                                         'if False:\n            sys.exit(f"the compilation'),
    ('the caller\'s environment passed on', 'env=COMPILATION_ENVIRONMENT, ', ''),
    ('the caller\'s locale passed on',   '"LC_ALL": "C", ', ''),
    ('the caller\'s TMPDIR passed on',   ', "TMPDIR": "/tmp"}', '}'),
    ('the caller\'s environment added to', 'env=COMPILATION_ENVIRONMENT, ', 'env={**os.environ, **COMPILATION_ENVIRONMENT}, '),
    ('valgrind sought on the compilation\'s PATH', 'r = subprocess.run([valgrind, ', 'r = subprocess.run(["valgrind", '),
    ('valgrind absent unchecked',        'if valgrind is None:', 'if False:'),
    ('valgrind absent crashes',          'except OSError as e:\n            sys.exit(f"valgrind could not be run',
                                         'except ValueError as e:\n            sys.exit(f"valgrind could not be run'),
    ('a source twice counted once',      'if len(set(paths)) != len(paths):', 'if False:'),
    ('a source twice by its spelling',   'if len(set(paths)) != len(paths):', 'if len(set(sources)) != len(sources):'),
    ('a directory accepted',             ' or not os.path.isfile(path):', ' or not os.path.exists(path):'),
    ('a source outside accepted',        'if os.path.commonpath([path, root]) != root or ', 'if '),
    ('a bad database crashes',           'except json.JSONDecodeError as e:\n        sys.exit(f"ninja',
                                         'except ValueError as e:\n        raise\n        sys.exit(f"ninja'),
    ('a unit keyed by its path',         'units = {source: instructions_of_compiling',
                                         'units = {os.path.join(root, source): instructions_of_compiling'),
    ('the boundary not given',           '"compile_cost": {"boundary": BOUNDARY, "units": units}', '"compile_cost": {"units": units}'),
    ('the compiler not given',           '            "compiler": compiler,\n', ''),
    ('the linker given as the compiler', 'tool_of(compiler_record, "CMAKE_CXX_COMPILER")', 'tool_of(compiler_record, "CMAKE_CXX_COMPILER_LINKER")'),
    ('the include directories not given', '                "implicit_include_directories": compiler_record["CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES"].split(";")},',
                                         '},'),
    ('valgrind\'s version not given',    '"version": symbol_table_report.run([valgrind, "--version"]).strip()', '"version": ""'),
    ('the linker\'s flags given too',    'build_settings(cache, ["CMAKE_CXX_FLAGS"])', 'build_settings(cache, ["CMAKE_CXX_FLAGS", "CMAKE_EXE_LINKER_FLAGS"])'),
    ('the tree called clean',            '**symbol_table_report.tree_state(root),', '"commit": symbol_table_report.tree_state(root)["commit"], "clean": True,'),
]
script, directory = sys.argv[1:]
with open(script, encoding='utf-8') as f:
    source = f.read()
for n, (description, old, new) in enumerate(MUTATIONS):
    os.makedirs(os.path.join(directory, str(n)))
    with open(os.path.join(directory, str(n), 'compile_cost_record.py'), 'w', encoding='utf-8') as f:
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
    run_controls "$tmp/mutants/$n/compile_cost_record.py"
    echo "$description: $fails of $total controls fail$( ((fails)) || echo '  <-- SURVIVED')"
    if ((fails == 0)); then survivors=1; fi
  done < "$tmp/mutants.txt"
  exit $survivors
fi

run_controls "$script"
if [[ $fails -eq 0 ]]; then echo "compile_cost_record selftest: all $total controls pass"; else
  echo "compile_cost_record selftest: $fails of $total controls failed"; fi
exit $((fails > 0))
