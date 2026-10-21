#!/bin/bash
# Controls for symbol_table_report.py, and a mutation check of the controls.
#
#   symbol_table_report_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the script and against each mutant of the script.
# The script must fail no control, and each mutant at least one.
#
# The controls build their own fixture: a git repository, and within it a
# build directory holding an executable and its objects, compiled and linked
# by c++ and read by nm, which must both be on PATH. The selftest writes the
# fixture's CMakeCache.txt and compiler record itself, and a stand-in for
# ninja answers from files in the build directory. So each control can hold
# the shape under test, and the selftest needs neither CMake nor ninja. The
# claims:
#
#   - a record holds the commit, whether the tree is clean, the toolchain as
#     the compiler record names it, the cache's build type and flags, the
#     number of symbols nm lists for the executable, and the total length in
#     bytes of their names;
#   - that total is the same for a fixture built at another path, although
#     the size of the string table holding the names is not;
#   - each object ninja links into the target is keyed by its source's path
#     relative to the repository, and holds the symbols nm finds defined in
#     it. An object in the build directory which ninja does not link is left
#     out;
#   - of several compiler records, the one written by the cache's CMake is
#     read, and none, or two, is an error;
#   - a build with work to do, a generator other than Ninja, a compiler record
#     with an entry missing or empty or naming a path which no longer exists,
#     a record naming both standard libraries or neither, or naming the one
#     the executable's symbols contradict, and an executable which is missing,
#     or for which nm lists nothing, are each an error;
#   - so are an empty selection of objects, an object ninja's compilation
#     database does not know, a source outside the repository, two objects
#     from one source, and a failure of ninja, nm or git.
#
# A control which checks that something is absent also checks that the run
# succeeded, and that something beside it is present.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script="$here/../symbol_table_report.py"
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
verbose=yes

fail() {
  fails=$((fails+1))
  if [[ $verbose == yes ]]; then echo "FAIL: $*"; fi
}

# run <argument>...: the subject's output in $out, its errors in $err, and its
# exit status in $rc
run() { python3 -B "$subject" "$@" > "$out" 2> "$err"; rc=$?; }

check() { # check <name> <yes|no> <pattern> [file, by default $err]
  local got
  total=$((total+1))
  if grep -qE -- "$3" "${4:-$err}"; then got=yes; else got=no; fi
  if [[ $got != "$2" ]]; then fail "$1 (expected $2, got $got, pattern: $3)"; fi
}

exits() { # exits <name> <status|nonzero>
  total=$((total+1))
  if [[ $2 == nonzero ]]; then
    if [[ $rc -eq 0 ]]; then fail "$1 (expected a non-zero exit, got 0)"; fi
  elif [[ $rc -ne $2 ]]; then fail "$1 (expected exit $2, got $rc: $(cat "$err"))"; fi
}

refuses() { # refuses <name> <pattern>: the run failed, saying why, and wrote no record
  exits "$1 is refused" nonzero
  check "$1 says why" yes "$2"
  total=$((total+1))
  if [[ -s $out ]]; then fail "$1 wrote a record"; fi
}

value() { # value <python expression over the record d>
  python3 -c 'import json, sys
d = json.load(open(sys.argv[1]))
print(eval(sys.argv[2]))' "$out" "$1" 2>&1
}

field() { # field <name> <python expression over the record d> <expected>
  local got
  total=$((total+1))
  got=$(value "$2")
  if [[ $got != "$3" ]]; then fail "$1 (expected $3, got $got)"; fi
}

# with_entry <name> <value>: the compiler record saved in $work/record, with
# the value of the entry <name> replaced by <value>
with_entry() { sed "s|^set($1 \"[^\"]*\")|set($1 \"$2\")|" "$work/record"; }

# The fixture -----------------------------------------------------------------

# fixture: a repository at $repo holding a built target at $build, and the
# files the stand-in ninja answers from, which a control may then change
fixture() {
  repo="$work/repo"; build="$repo/build/TestAll/fixture"; objects="$build/CMakeFiles/TestAll.dir"
  mkdir -p "$repo/TestAll" "$repo/Tests" "$objects" "$build/CMakeFiles/stale.dir" "$build/ninja"
  # Both translation units instantiate seq_fixture_shared<int>, so that the
  # executable keeps one copy of what both objects define. Main.cpp defines
  # a name holding a character which UTF-8 encodes in two bytes, so that the
  # names' total can tell bytes from characters.
  cat > "$repo/Tests/Shared.hpp" <<'EOF'
#include <string>
template<class T> std::string seq_fixture_shared(T t) { return std::to_string(t); }
EOF
  cat > "$repo/Tests/A.cpp" <<'EOF'
#include "Shared.hpp"
std::string seq_fixture_a(int n) { return seq_fixture_shared(n) + "a"; }
EOF
  cat > "$repo/Tests/B.cpp" <<'EOF'
#include "Shared.hpp"
std::string seq_fixture_b(int n) { return seq_fixture_shared(n) + "b"; }
int seq_fixture_b2() { return 2; }
EOF
  cat > "$repo/TestAll/Main.cpp" <<'EOF'
#include <string>
std::string seq_fixture_a(int);
std::string seq_fixture_b(int);
int seq_fixture_café() { return 0; }
int main() { return static_cast<int>((seq_fixture_a(1) + seq_fixture_b(2)).size()) + seq_fixture_café(); }
EOF
  printf 'int seq_fixture_stale() { return 0; }\n' > "$repo/Tests/Stale.cpp"
  printf '/build\n' > "$repo/.gitignore"
  git -C "$repo" init -q
  git -C "$repo" add -A
  git -C "$repo" -c user.name=fixture -c user.email=fixture@example.com commit -q -m fixture
  local unit
  # Debug information puts the paths of the objects, or of their sources,
  # into the executable's string table.
  for unit in Tests/A Tests/B TestAll/Main; do
    c++ -std=c++17 -g -c "$repo/$unit.cpp" -o "$objects/${unit##*/}.cpp.o" \
      || { echo "the fixture does not compile"; exit 1; }
  done
  c++ -c "$repo/Tests/Stale.cpp" -o "$build/CMakeFiles/stale.dir/Stale.cpp.o" \
    || { echo "the fixture does not compile"; exit 1; }
  c++ -o "$build/TestAll" "$objects/A.cpp.o" "$objects/B.cpp.o" "$objects/Main.cpp.o" \
    || { echo "the fixture does not link"; exit 1; }

  nm_path=$(command -v nm)
  cache_entries > "$build/CMakeCache.txt"
  mkdir -p "$build/CMakeFiles/4.4.3"
  compiler_record > "$build/CMakeFiles/4.4.3/CMakeCXXCompiler.cmake"
  # A record left by an earlier CMake, with a toolchain no control expects.
  mkdir -p "$build/CMakeFiles/4.1.2"
  compiler_record | sed 's/"Fixture"/"Earlier"/' > "$build/CMakeFiles/4.1.2/CMakeCXXCompiler.cmake"

  printf 'ninja: Entering directory `%s'"'"'\nninja: no work to do.\n' "$build" > "$build/ninja/dry-run"
  printf '%s\n' CMakeFiles/TestAll.dir/A.cpp.o CMakeFiles/TestAll.dir/B.cpp.o \
    CMakeFiles/TestAll.dir/Main.cpp.o "$repo/Tests/A.cpp" "$repo/Tests/Shared.hpp" > "$build/ninja/inputs"
  compilations > "$build/ninja/compdb"
}

cache_entries() {
  cat <<EOF
# This is the CMakeCache file.
//Choose the type of build.
CMAKE_BUILD_TYPE:STRING=Debug
CMAKE_CXX_FLAGS:STRING=-fixture-flag
CMAKE_CXX_FLAGS_DEBUG:STRING=-g
CMAKE_EXE_LINKER_FLAGS:STRING=
CMAKE_EXE_LINKER_FLAGS_DEBUG:STRING=-fixture-link-flag
CMAKE_GENERATOR:INTERNAL=Ninja
CMAKE_MAKE_PROGRAM:FILEPATH=$tmp/bin/ninja
CMAKE_NM:FILEPATH=$nm_path
CMAKE_HOME_DIRECTORY:INTERNAL=$repo/TestAll
CMAKE_CACHE_MAJOR_VERSION:INTERNAL=4
CMAKE_CACHE_MINOR_VERSION:INTERNAL=4
CMAKE_CACHE_PATCH_VERSION:INTERNAL=3
EOF
}

compiler_record() {
  cat <<EOF
set(CMAKE_CXX_COMPILER "$(command -v c++)")
set(CMAKE_CXX_COMPILER_ARG1 "")
set(CMAKE_CXX_COMPILER_ID "Fixture")
set(CMAKE_CXX_COMPILER_VERSION "1.2.3")
set(CMAKE_CXX_COMPILER_LINKER "/fixture/ld")
set(CMAKE_CXX_COMPILER_LINKER_ID "FixtureLD")
set(CMAKE_CXX_COMPILER_LINKER_VERSION "4.5")
set(CMAKE_COMPILER_IS_GNUCXX )
set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "$repo/Tests;$repo/TestAll")
set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "$platform_libraries")
EOF
}

compilations() {
  local unit
  printf '['
  for unit in Tests/A Tests/B TestAll/Main; do
    printf '{"directory": "%s", "command": "c++", "file": "%s",\n "output": "CMakeFiles/TestAll.dir/%s.cpp.o"},\n' \
      "$build" "$repo/$unit.cpp" "${unit##*/}"
  done
  printf '{"directory": "%s", "command": "c++", "file": "../../../Tests/Stale.cpp",\n "output": "%s"},\n' \
    "$build" CMakeFiles/stale.dir/Stale.cpp.o
  printf '{"directory": "%s", "command": "", "file": "x", "output": "all"}]\n' "$build"
}

# The stand-in ninja: answers `-C <dir> -n <target>`, `-C <dir> -t inputs
# <target>` and `-C <dir> -t compdb` from <dir>/ninja/{dry-run,inputs,compdb},
# and exits with the status in <dir>/ninja/status-<answer>, if there is one.
mkdir -p "$tmp/bin"
cat > "$tmp/bin/ninja" <<'EOF'
#!/bin/bash
[[ $1 == -C ]] || { echo "stand-in ninja: expected -C first" >&2; exit 2; }
dir=$2; shift 2
case "$*" in
  "-n "*)          answer=dry-run ;;
  "-t inputs "*)   answer=inputs ;;
  "-t compdb")     answer=compdb ;;
  *) echo "stand-in ninja: unexpected arguments: $*" >&2; exit 2 ;;
esac
cat "$dir/ninja/$answer"
exit "$(cat "$dir/ninja/status-$answer" 2>/dev/null || echo 0)"
EOF
chmod +x "$tmp/bin/ninja"

case "$(uname -s)" in
  Darwin) platform=Mach-O; standard_library=libc++
          platform_libraries="c++";             other_libraries="stdc++;m" ;;
  *)      platform=ELF;    standard_library=libstdc++
          platform_libraries="stdc++;m;gcc_s;c"; other_libraries="c++;m"   ;;
esac

# string_table_oracle <executable>: the size of its string table, as reported
# by readelf or otool. The path controls compare it between two fixtures, so
# that they can tell the names' total from it
string_table_oracle() {
  if [[ $platform == ELF ]]; then
    local size
    size=$(readelf -SW "$1" | sed -nE 's/.*\] \.strtab +STRTAB +[0-9a-f]+ +[0-9a-f]+ +([0-9a-f]+) .*/\1/p')
    [[ -n $size ]] && echo $((16#$size))
  else
    otool -l "$1" | awk '$1 == "strsize" {print $2}'
  fi
}

# name_bytes_oracle <executable>: the total length in bytes of the names nm
# lists in its POSIX format, the first field of each line
name_bytes_oracle() { nm -P "$1" | LC_ALL=C awk '{ total += length($1) } END { print total }'; }

# defined_oracle <object>: the symbols defined in it, as plain nm lists them
defined_oracle() { nm "$1" | awk 'NF == 3' | wc -l | tr -d ' '; }

record_controls() {
  local head symbols bytes
  fixture
  head=$(git -C "$repo" rev-parse HEAD)
  symbols=$(nm "$build/TestAll" | wc -l | tr -d ' ')
  bytes=$(name_bytes_oracle "$build/TestAll")
  if [[ -z $bytes || $symbols -eq 0 ]]; then echo "the oracles read nothing of the fixture"; exit 1; fi
  if ! nm "$build/TestAll" | LC_ALL=C grep -q 'seq_fixture_café'; then
    echo "nm lists no name of the fixture whose bytes outnumber its characters"; exit 1
  fi

  run "$build" TestAll
  exits "a record is written" 0
  field "the commit"               'd["commit"]'                            "$head"
  field "a clean tree"             'd["clean"]'                             True
  field "the target"               'd["target"]'                            TestAll
  field "the compiler"             'sorted(d["toolchain"]["compiler"].items())' \
                                   "[('id', 'Fixture'), ('path', '$(command -v c++)'), ('version', '1.2.3')]"
  field "the linker"               'sorted(d["toolchain"]["linker"].items())' \
                                   "[('id', 'FixtureLD'), ('path', '/fixture/ld'), ('version', '4.5')]"
  field "the standard library"     'd["toolchain"]["standard_library"]'     "$standard_library"
  field "the nm"                   'd["toolchain"]["nm"]["path"]'           "$nm_path"
  field "the nm version"           'd["toolchain"]["nm"]["version"]' \
                                   "$("$nm_path" --version | grep -m1 '[0-9]' | sed 's/^ *//; s/ *$//')"
  field "the cache"                'sorted(d["cache"].items())' \
    "[('CMAKE_BUILD_TYPE', 'Debug'), ('CMAKE_CXX_FLAGS', '-fixture-flag'), ('CMAKE_CXX_FLAGS_DEBUG', '-g'), \
('CMAKE_EXE_LINKER_FLAGS', ''), ('CMAKE_EXE_LINKER_FLAGS_DEBUG', '-fixture-link-flag')]"
  field "the symbols, as nm counts them" 'd["executable"]["symbols"]'           "$symbols"
  field "the names' total"         'd["executable"]["symbol_name_bytes"]'   "$bytes"
  field "the objects linked, keyed by source" 'sorted(d["defined_symbols_by_object"])' \
                                   "['TestAll/Main.cpp', 'Tests/A.cpp', 'Tests/B.cpp']"
  field "an object's defined symbols" 'd["defined_symbols_by_object"]["Tests/A.cpp"]' \
                                     "$(defined_oracle "$objects/A.cpp.o")"
  field "another object's"           'd["defined_symbols_by_object"]["Tests/B.cpp"]' \
                                     "$(defined_oracle "$objects/B.cpp.o")"
  field "objects differ in their counts" \
    'd["defined_symbols_by_object"]["Tests/B.cpp"] - d["defined_symbols_by_object"]["Tests/A.cpp"]' 1

  # The record names the build's own nm, whatever nm is first on PATH.
  mkdir -p "$work/decoy"; printf '#!/bin/sh\nexit 3\n' > "$work/decoy/nm"; chmod +x "$work/decoy/nm"
  PATH="$work/decoy:$PATH" run "$build" TestAll
  exits "the cache's nm is run, not PATH's" 0

  echo '// edited' >> "$repo/Tests/A.cpp"
  run "$build" TestAll
  exits "a record of a dirty tree is written" 0
  field "a dirty tree" 'd["clean"]' False
  git -C "$repo" checkout -q -- Tests/A.cpp

  : > "$repo/Tests/Untracked.cpp"
  git -C "$repo" config status.showUntrackedFiles no
  run "$build" TestAll
  exits "a record of a tree with an untracked file is written" 0
  field "an untracked file, however git is configured, makes the tree dirty" 'd["clean"]' False
  rm "$repo/Tests/Untracked.cpp"

  echo '[1/2] Building CXX object CMakeFiles/TestAll.dir/A.cpp.o' > "$build/ninja/dry-run"
  run "$build" TestAll
  refuses "a build with work to do" 'ninja has work to do for TestAll'
  echo '[1/1] echo "ninja: no work to do."' > "$build/ninja/dry-run"
  run "$build" TestAll
  refuses "a build whose work quotes ninja's verdict" 'ninja has work to do for TestAll'
  printf 'ninja: Entering directory `x'"'"'\nninja: no work to do.\n' > "$build/ninja/dry-run"

  sed -i.bak 's/^CMAKE_GENERATOR:INTERNAL=Ninja$/CMAKE_GENERATOR:INTERNAL=Unix Makefiles/' "$build/CMakeCache.txt"
  run "$build" TestAll
  refuses "a generator other than Ninja" "generator 'Unix Makefiles'"
  mv "$build/CMakeCache.txt.bak" "$build/CMakeCache.txt"

  run "$work" TestAll
  refuses "a directory with no cache" 'CMakeCache\.txt cannot be read'

  run "$build" Absent
  refuses "an absent executable" 'Absent is not a file'
  run "$build" CMakeFiles
  refuses "a directory in the executable's place" 'CMakeFiles is not a file'
}

record_selection_controls() {
  fixture
  run "$build" TestAll
  exits "of two records, one is written by the cache's CMake" 0
  field "that record is read" 'd["toolchain"]["compiler"]["id"]' Fixture

  mv "$build/CMakeFiles/4.4.3" "$build/CMakeFiles/4.4.4"
  run "$build" TestAll
  refuses "no record for the cache's CMake" 'holds 0 C\+\+ compiler records for CMake 4\.4\.3'
  mkdir -p "$build/CMakeFiles/4.4.3"
  run "$build" TestAll
  refuses "a directory for the cache's CMake with no record" 'holds 0 C\+\+ compiler records'
  rm -r "$build/CMakeFiles/4.4.3"; mv "$build/CMakeFiles/4.4.4" "$build/CMakeFiles/4.4.3"
  mkdir -p "$build/CMakeFiles/4.4.3-rc1"
  cp "$build/CMakeFiles/4.4.3/CMakeCXXCompiler.cmake" "$build/CMakeFiles/4.4.3-rc1/"
  run "$build" TestAll
  refuses "two records for the cache's CMake" 'holds 2 C\+\+ compiler records for CMake 4\.4\.3'
  rm -r "$build/CMakeFiles/4.4.3-rc1"
  mkdir -p "$build/CMakeFiles/4.4.30"; cp "$build/CMakeFiles/4.4.3/CMakeCXXCompiler.cmake" "$build/CMakeFiles/4.4.30/"
  run "$build" TestAll
  exits "a record for a later patch release is not the cache's" 0
  rm -r "$build/CMakeFiles/4.4.30"

  sed -i.bak '/^CMAKE_CACHE_PATCH_VERSION/d' "$build/CMakeCache.txt"
  run "$build" TestAll
  refuses "a cache without CMake's version" 'CMakeCache\.txt has no entry CMAKE_CACHE_PATCH_VERSION'
  mv "$build/CMakeCache.txt.bak" "$build/CMakeCache.txt"

  local record="$build/CMakeFiles/4.4.3/CMakeCXXCompiler.cmake" name
  cp "$record" "$work/record"
  for name in CMAKE_CXX_COMPILER CMAKE_CXX_COMPILER_ID CMAKE_CXX_COMPILER_VERSION CMAKE_CXX_COMPILER_LINKER \
              CMAKE_CXX_COMPILER_LINKER_ID CMAKE_CXX_COMPILER_LINKER_VERSION CMAKE_CXX_IMPLICIT_LINK_LIBRARIES \
              CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES; do
    grep -v "^set($name \"" "$work/record" > "$record"
    run "$build" TestAll
    refuses "a record without $name" "has no entry $name\$"
    with_entry "$name" "" > "$record"
    run "$build" TestAll
    refuses "a record with an empty $name" "has an empty entry $name\$"
  done

  printf 'set(CMAKE_CXX_COMPILER_ID "\xff")\n' > "$record"
  run "$build" TestAll
  refuses "a record which is not UTF-8" 'CMakeCXXCompiler\.cmake cannot be read'
  with_entry CMAKE_CXX_COMPILER "$work/gone/c++" > "$record"
  run "$build" TestAll
  refuses "a record naming a compiler which has gone" "names $work/gone/c\+\+, which no longer exists"
  with_entry CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "$repo/Tests;$work/gone/include" > "$record"
  run "$build" TestAll
  refuses "a record naming an include directory which has gone" "names $work/gone/include, which no longer exists"

  with_entry CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "m;c" > "$record"
  run "$build" TestAll
  refuses "a record naming no standard library" 'names 0 standard libraries'
  with_entry CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "c++;stdc++" > "$record"
  run "$build" TestAll
  refuses "a record naming both standard libraries" 'names 2 standard libraries'
  with_entry CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "$other_libraries" > "$record"
  run "$build" TestAll
  refuses "a record naming the standard library the symbols contradict" 'the symbols of .* say otherwise'
  with_entry CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "gcc;c++abi;stdc++fs;$platform_libraries" > "$record"
  run "$build" TestAll
  exits "a library whose name holds the standard library's is another library" 0
  cp "$work/record" "$record"
}

object_controls() {
  fixture
  printf '%s\n' "$repo/Tests/A.cpp" > "$build/ninja/inputs"
  run "$build" TestAll
  refuses "no objects" 'ninja lists no objects among the inputs of TestAll'

  printf '%s\n' CMakeFiles/TestAll.dir/A.cpp.o CMakeFiles/TestAll.dir/Unknown.cpp.o > "$build/ninja/inputs"
  run "$build" TestAll
  refuses "an object the compilation database does not know" 'has no entry for CMakeFiles/TestAll.dir/Unknown\.cpp\.o'

  printf '%s\n' CMakeFiles/TestAll.dir/A.cpp.o CMakeFiles/stale.dir/Stale.cpp.o > "$build/ninja/inputs"
  run "$build" TestAll
  exits "a source named relative to the build directory" 0
  field "is keyed relative to the repository" 'sorted(d["defined_symbols_by_object"])' \
                                              "['Tests/A.cpp', 'Tests/Stale.cpp']"

  printf '%s\n' CMakeFiles/TestAll.dir/A.cpp.o > "$build/ninja/inputs"
  compilations | sed "s|\"$repo/Tests/A.cpp\"|\"$work/Outside.cpp\"|" > "$build/ninja/compdb"
  : > "$work/Outside.cpp"
  run "$build" TestAll
  refuses "a source outside the repository" "lies outside the repository"
  compilations | sed "s|\"$repo/Tests/A.cpp\"|\"$work/repository-like/Tests/A.cpp\"|" > "$build/ninja/compdb"
  run "$build" TestAll
  refuses "a source beside the repository whose path starts with the repository's" "lies outside the repository"

  printf '%s\n' CMakeFiles/TestAll.dir/A.cpp.o CMakeFiles/TestAll.dir/B.cpp.o > "$build/ninja/inputs"
  compilations | sed 's|/Tests/B\.cpp"|/Tests/A.cpp"|' > "$build/ninja/compdb"
  run "$build" TestAll
  refuses "two objects from one source" 'are both compiled from Tests/A\.cpp'

  echo 'not json' > "$build/ninja/compdb"
  run "$build" TestAll
  refuses "a compilation database which is not JSON" "compilation database .* cannot be read"
  echo '[{"directory": "x"}]' > "$build/ninja/compdb"
  run "$build" TestAll
  refuses "a compilation database entry without its output" "compilation database .* cannot be read"
  echo '{"output": "x"}' > "$build/ninja/compdb"
  run "$build" TestAll
  refuses "a compilation database which is not a list" "compilation database .* cannot be read"
  compilations > "$build/ninja/compdb"

  echo 1 > "$build/ninja/status-inputs"
  run "$build" TestAll
  refuses "a failure of ninja" "ninja -C $build -t inputs TestAll failed with exit status 1"
  rm "$build/ninja/status-inputs"
}

tool_controls() {
  fixture
  cat > "$work/silent-nm" <<'EOF'
#!/bin/sh
[ "$1" = --version ] && echo "fixture nm 9.9"
exit 0
EOF
  chmod +x "$work/silent-nm"
  sed -i.bak "s|^CMAKE_NM:FILEPATH=.*|CMAKE_NM:FILEPATH=$work/silent-nm|" "$build/CMakeCache.txt"
  run "$build" TestAll
  refuses "an nm which lists nothing" 'nm lists no symbols for'
  printf '#!/bin/sh\nexit 4\n' > "$work/silent-nm"
  run "$build" TestAll
  refuses "a failure of nm" 'failed with exit status 4'
  sed -i.bak "s|^CMAKE_NM:FILEPATH=.*|CMAKE_NM:FILEPATH=$work/absent-nm|" "$build/CMakeCache.txt"
  run "$build" TestAll
  refuses "an nm which cannot be run" 'absent-nm could not be run'
  cat > "$work/silent-nm" <<EOF
#!/bin/sh
[ "\$1" = --version ] && { echo "nm, without its number"; exit 0; }
exec $nm_path "\$@"
EOF
  sed -i.bak "s|^CMAKE_NM:FILEPATH=.*|CMAKE_NM:FILEPATH=$work/silent-nm|" "$build/CMakeCache.txt"
  run "$build" TestAll
  refuses "an nm which prints no version" 'prints no version'
  sed -i.bak "s|^CMAKE_NM:FILEPATH=.*|CMAKE_NM:FILEPATH=$nm_path|" "$build/CMakeCache.txt"

  rm -rf "$repo/.git"
  GIT_CEILING_DIRECTORIES=$work run "$build" TestAll
  refuses "a source directory outside a repository" 'git -C .* rev-parse --show-toplevel failed'
}

# The checkout's path ---------------------------------------------------------

# A fixture built at a longer path holds the same names, and the paths its
# debug information holds are longer. So the string table's size differs
# between the two fixtures. The names' total must not.
path_controls() {
  local bytes table
  fixture
  run "$build" TestAll
  exits "a record is written at one path" 0
  bytes=$(value 'd["executable"]["symbol_name_bytes"]')
  table=$(string_table_oracle "$build/TestAll")

  work="$work/a/longer/path"; mkdir -p "$work"
  fixture
  if [[ -z $table || $(string_table_oracle "$build/TestAll") == "$table" ]]; then
    echo "the fixture's string table has the same size at both paths, so it cannot tell the rival"; exit 1
  fi
  run "$build" TestAll
  exits "a record is written at a longer path" 0
  field "the names' total does not depend on the path" 'd["executable"]["symbol_name_bytes"]' "$bytes"
}

run_controls() { # run_controls <script>: sets fails, and total, the controls run
  subject=$1 fails=0 total=0
  local control
  for control in record_controls path_controls record_selection_controls object_controls tool_controls; do
    work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
    "$control"
  done
}

# Each mutant breaks one behaviour, and is (description, old text, new text).
mutants() { # mutants <dir>: writes <dir>/<n>/symbol_table_report.py, and
            # prints a line "<n> <occurrences of the old text> <description>"
  python3 - "$script" "$1" <<'EOF'
import os, sys
MUTATIONS = [
    ('a failed command read',            'if r.returncode:\n        sys.exit(f"{\' \'.join(command)}',
                                         'if False:\n        sys.exit(f"{\' \'.join(command)}'),
    ('a command not started crashes',    'except OSError as e:\n        sys.exit(f"{command[0]} could not be run',
                                         'except ValueError as e:\n        sys.exit(f"{command[0]} could not be run'),
    ('an absent entry read as empty',    'if name not in entries:\n        sys.exit', 'if False:\n        sys.exit'),
    ('any generator accepted',           'if generator != "Ninja":', 'if False:'),
    ('a build with work accepted',       'if "ninja: no work to do." not in', 'if "ninja: no work to do." in'),
    ('the dry run anywhere in a line',   'not in ninja(cache, build_dir, "-n", target).splitlines():',
                                         'not in ninja(cache, build_dir, "-n", target):'),
    ('any record of the release',        'if (d == version or d.startswith(version + "-"))',
                                         'if (d.startswith(version))'),
    ('the first of several records',     'if len(matches) != 1:', 'if not matches:'),
    ('a directory without a record',     '\n                     and os.path.isfile(os.path.join(files, d, '
                                         '"CMakeCXXCompiler.cmake")))',
                                         ')'),
    ('an empty record entry accepted',   'if not entry(record, name, path):', 'if entry(record, name, path) is None:'),
    ('the linker id unchecked',          '"CMAKE_CXX_COMPILER_LINKER_ID",\n', '\n'),
    ('the include directories unchecked for staleness',
                                         'for named in [record["CMAKE_CXX_COMPILER"], '
                                         '*record["CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES"].split(";")]:',
                                         'for named in [record["CMAKE_CXX_COMPILER"]]:'),
    ('the compiler unchecked for staleness',
                                         'for named in [record["CMAKE_CXX_COMPILER"], *record',
                                         'for named in [*record'),
    ('a library matched within a name',  'if library in linked]', 'if any(library in name for name in linked)]'),
    ('two standard libraries accepted',  'if len(named) != 1:', 'if not named:'),
    ('no standard library accepted',     'if len(named) != 1:', 'if len(named) > 1:'),
    ('the symbols not consulted',        'if any(LIBCXX_NAMESPACE in name for name in listed)'
                                         ' != (library == "libc++"):',
                                         'if False:'),
    ('an empty listing accepted',        'if not listed:\n        sys.exit(f"nm lists no symbols',
                                         'if False:\n        sys.exit(f"nm lists no symbols'),
    ('the PATH nm run',                  'listed = symbols(nm, executable)', 'listed = symbols("nm", executable)'),
    ('an object counts every symbol',    'objects_by_source.values())))',
                                         'objects_by_source.values())))\n    counts = {k: len(symbols(nm, '
                                         'os.path.join(build_dir, o))) for k, o in objects_by_source.items()}'),
    ('every input an object',            'if line.endswith(".o")]', 'if line]'),
    ('no objects accepted',              'if not linked:\n        sys.exit(f"ninja lists no objects',
                                         'if False:\n        sys.exit(f"ninja lists no objects'),
    ('an unknown object skipped',        'sys.exit(f"ninja\'s compilation database has no entry for {obj}")',
                                         'continue'),
    ('a source outside kept',            'if os.path.commonpath([source, root]) != root:', 'if False:'),
    ('a source matched by prefix',       'if os.path.commonpath([source, root]) != root:',
                                         'if not source.startswith(root):'),
    ('a source keyed absolutely',        'key = os.path.relpath(source, root)', 'key = source'),
    ('a source not resolved from the build directory',
                                         'os.path.join(compilation["directory"], compilation["file"])',
                                         'compilation["file"]'),
    ('a shared source overwritten',      'if key in out:', 'if False:'),
    ('a bad database crashes',           'except (json.JSONDecodeError, TypeError, KeyError) as e:',
                                         'except json.JSONDecodeError as e:'),
    ('the newline counted',              'len(name.encode("utf-8")) for name in listed',
                                         'len(name.encode("utf-8")) + 1 for name in listed'),
    ('characters counted, not bytes',    'len(name.encode("utf-8")) for name in listed',
                                         'len(name) for name in listed'),
    ('the names demangled',              '[nm, "--format=just-symbols", *options, path]',
                                         '[nm, "--format=just-symbols", "--demangle", *options, path]'),
    ('the POSIX lines summed',           '[nm, "--format=just-symbols", *options, path]', '[nm, "-P", *options, path]'),
    ('debugging symbols listed',         'listed = symbols(nm, executable)', 'listed = symbols(nm, executable, "-a")'),
    ('the defined names alone summed',   'for name in listed)},',
                                         'for name in symbols(nm, executable, "--defined-only"))},'),
    ('the nm version from the first line', 'if re.search(r"\\d", line)]', 'if line]'),
    ('no nm version accepted',           'if not lines:\n        sys.exit(f"{nm} --version',
                                         'if False:\n        sys.exit(f"{nm} --version'),
    ('a dirty tree called clean',        '"clean": run(["git", "-C", root, "status", "--porcelain", '
                                         '"--untracked-files=all"]) == "",',
                                         '"clean": True,'),
    ('untracked files as git is configured',
                                         '"status", "--porcelain", "--untracked-files=all"]',
                                         '"status", "--porcelain"]'),
    ('the type\'s flags left out',       'if build_type:\n        flags +=', 'if False:\n        flags +='),
    ('the linker\'s flags left out',     'flags = ["CMAKE_CXX_FLAGS", "CMAKE_EXE_LINKER_FLAGS"]',
                                         'flags = ["CMAKE_CXX_FLAGS"]'),
    ('the linker named as the compiler', '"linker":   {"path":    record["CMAKE_CXX_COMPILER_LINKER"],',
                                         '"linker":   {"path":    record["CMAKE_CXX_COMPILER"],'),
    ('the executable a directory',       'if not os.path.isfile(executable):', 'if not os.path.exists(executable):'),
    ('a record not UTF-8 crashes',       'text = f.read()\n    except (OSError, UnicodeDecodeError) as e:',
                                         'text = f.read()\n    except OSError as e:'),
]
script, directory = sys.argv[1:]
with open(script, encoding='utf-8') as f:
    source = f.read()
for n, (description, old, new) in enumerate(MUTATIONS):
    os.makedirs(os.path.join(directory, str(n)))
    with open(os.path.join(directory, str(n), 'symbol_table_report.py'), 'w', encoding='utf-8') as f:
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
    run_controls "$tmp/mutants/$n/symbol_table_report.py"
    echo "$description: $fails of $total controls fail$( ((fails)) || echo '  <-- SURVIVED')"
    if ((fails == 0)); then survivors=1; fi
  done < "$tmp/mutants.txt"
  exit $survivors
fi

run_controls "$script"
if [[ $fails -eq 0 ]]; then echo "symbol_table_report selftest on $platform: all $total controls pass"; else
  echo "symbol_table_report selftest on $platform: $fails of $total controls failed"; fi
exit $((fails > 0))
