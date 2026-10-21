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
# executable is in this platform's format alone; the controls of the other
# format, and of every format the script refuses, read files the selftest
# writes byte by byte. The claims:
#
#   - a record holds the commit, whether the tree is clean, the toolchain as
#     the compiler record names it, the cache's build type and flags, the
#     executable's symbols as nm counts them, and its string table's size as
#     readelf or otool reports it;
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
#     from one source, and a failure of ninja, nm or git;
#   - the string table's size is read from a 64-bit little-endian ELF or
#     Mach-O file, including an ELF file with more sections than its header
#     can count. Any other file, and one whose symbol table cannot be found
#     or does not link to a string table, is an error.
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

field() { # field <name> <python expression over the record d> <expected>
  local got
  total=$((total+1))
  got=$(python3 -c 'import json, sys
d = json.load(open(sys.argv[1]))
print(eval(sys.argv[2]))' "$out" "$2" 2>&1)
  if [[ $got != "$3" ]]; then fail "$1 (expected $3, got $got)"; fi
}

# The fixture ------------------------------------------------------------------

# fixture: a repository at $repo holding a built target at $build, and the
# files the stand-in ninja answers from, which a control may then change
fixture() {
  repo="$work/repo"; build="$repo/build/TestAll/fixture"; objects="$build/CMakeFiles/TestAll.dir"
  mkdir -p "$repo/TestAll" "$repo/Tests" "$objects" "$build/CMakeFiles/stale.dir" "$build/ninja"
  # Both translation units instantiate seq_fixture_shared<int>, so that the
  # executable keeps one copy of what both objects define.
  cat > "$repo/Tests/Shared.hpp" <<'EOF'
#include <string>
template<class T> std::string seq_fixture_shared(T t) { return std::to_string(t); }
EOF
  printf '#include "Shared.hpp"\nstd::string seq_fixture_a(int n) { return seq_fixture_shared(n) + "a"; }\n' > "$repo/Tests/A.cpp"
  printf '#include "Shared.hpp"\nstd::string seq_fixture_b(int n) { return seq_fixture_shared(n) + "b"; }\nint seq_fixture_b2() { return 2; }\n' > "$repo/Tests/B.cpp"
  printf '#include <string>\nstd::string seq_fixture_a(int);\nstd::string seq_fixture_b(int);\nint main() { return static_cast<int>((seq_fixture_a(1) + seq_fixture_b(2)).size()); }\n' > "$repo/TestAll/Main.cpp"
  printf 'int seq_fixture_stale() { return 0; }\n' > "$repo/Tests/Stale.cpp"
  printf '/build\n' > "$repo/.gitignore"
  git -C "$repo" init -q
  git -C "$repo" add -A
  git -C "$repo" -c user.name=fixture -c user.email=fixture@example.com commit -q -m fixture
  local unit
  for unit in Tests/A Tests/B TestAll/Main; do
    c++ -std=c++17 -c "$repo/$unit.cpp" -o "$objects/${unit##*/}.cpp.o" || { echo "the fixture does not compile"; exit 1; }
  done
  c++ -c "$repo/Tests/Stale.cpp" -o "$build/CMakeFiles/stale.dir/Stale.cpp.o" || { echo "the fixture does not compile"; exit 1; }
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
    printf '{"directory": "%s", "command": "c++", "file": "%s", "output": "CMakeFiles/TestAll.dir/%s.cpp.o"},\n' \
      "$build" "$repo/$unit.cpp" "${unit##*/}"
  done
  printf '{"directory": "%s", "command": "c++", "file": "../../../Tests/Stale.cpp", "output": "CMakeFiles/stale.dir/Stale.cpp.o"},\n' "$build"
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
  Darwin) platform=Mach-O; platform_libraries="c++";             other_libraries="stdc++;m" ;;
  *)      platform=ELF;    platform_libraries="stdc++;m;gcc_s;c"; other_libraries="c++;m"   ;;
esac

# string_table_oracle <executable>: the size of its string table, as readelf
# or otool reports it
string_table_oracle() {
  if [[ $platform == ELF ]]; then
    local size
    size=$(readelf -SW "$1" | sed -nE 's/.*\] \.strtab +STRTAB +[0-9a-f]+ +[0-9a-f]+ +([0-9a-f]+) .*/\1/p')
    [[ -n $size ]] && echo $((16#$size))
  else
    otool -l "$1" | awk '$1 == "strsize" {print $2}'
  fi
}

# defined_oracle <object>: the symbols defined in it, as plain nm lists them
defined_oracle() { nm "$1" | awk 'NF == 3' | wc -l | tr -d ' '; }

record_controls() {
  local head symbols bytes
  fixture
  head=$(git -C "$repo" rev-parse HEAD)
  symbols=$(nm "$build/TestAll" | wc -l | tr -d ' ')
  bytes=$(string_table_oracle "$build/TestAll")
  if [[ -z $bytes || $symbols -eq 0 ]]; then echo "the oracles read nothing of the fixture"; exit 1; fi

  run "$build" TestAll
  exits "a record is written" 0
  field "the commit"               'd["commit"]'                            "$head"
  field "a clean tree"             'd["clean"]'                             True
  field "the target"               'd["target"]'                            TestAll
  field "the compiler"             'sorted(d["toolchain"]["compiler"].items())' \
                                   "[('id', 'Fixture'), ('path', '$(command -v c++)'), ('version', '1.2.3')]"
  field "the linker"               'sorted(d["toolchain"]["linker"].items())' \
                                   "[('id', 'FixtureLD'), ('path', '/fixture/ld'), ('version', '4.5')]"
  field "the standard library"     'd["toolchain"]["standard_library"]'     "$( [[ $platform == ELF ]] && echo libstdc++ || echo libc++)"
  field "the nm"                   'd["toolchain"]["nm"]["path"]'           "$nm_path"
  field "the nm version"           'd["toolchain"]["nm"]["version"]'        "$("$nm_path" --version | grep -m1 '[0-9]' | sed 's/^ *//; s/ *$//')"
  field "the cache"                'sorted(d["cache"].items())' \
    "[('CMAKE_BUILD_TYPE', 'Debug'), ('CMAKE_CXX_FLAGS', '-fixture-flag'), ('CMAKE_CXX_FLAGS_DEBUG', '-g'), ('CMAKE_EXE_LINKER_FLAGS', ''), ('CMAKE_EXE_LINKER_FLAGS_DEBUG', '-fixture-link-flag')]"
  field "the format"               'd["executable"]["format"]'              "$platform"
  field "the symbols, as nm counts them" 'd["executable"]["symbols"]'      "$symbols"
  field "the string table's size"  'd["executable"]["string_table_bytes"]'  "$bytes"
  field "the objects linked, keyed by source" 'sorted(d["defined_symbols_by_object"])' \
                                   "['TestAll/Main.cpp', 'Tests/A.cpp', 'Tests/B.cpp']"
  field "an object's defined symbols"  'd["defined_symbols_by_object"]["Tests/A.cpp"]' "$(defined_oracle "$objects/A.cpp.o")"
  field "another object's"             'd["defined_symbols_by_object"]["Tests/B.cpp"]' "$(defined_oracle "$objects/B.cpp.o")"
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
  mkdir -p "$build/CMakeFiles/4.4.3-rc1"; cp "$build/CMakeFiles/4.4.3/CMakeCXXCompiler.cmake" "$build/CMakeFiles/4.4.3-rc1/"
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
    sed "s|^set($name \"[^\"]*\")|set($name \"\")|" "$work/record" > "$record"
    run "$build" TestAll
    refuses "a record with an empty $name" "has an empty entry $name\$"
  done

  printf 'set(CMAKE_CXX_COMPILER_ID "\xff")\n' > "$record"
  run "$build" TestAll
  refuses "a record which is not UTF-8" 'CMakeCXXCompiler\.cmake cannot be read'
  sed "s|^set(CMAKE_CXX_COMPILER \"[^\"]*\")|set(CMAKE_CXX_COMPILER \"$work/gone/c++\")|" "$work/record" > "$record"
  run "$build" TestAll
  refuses "a record naming a compiler which has gone" "names $work/gone/c\+\+, which no longer exists"
  sed "s|^set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES \"[^\"]*\")|set(CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES \"$repo/Tests;$work/gone/include\")|" \
    "$work/record" > "$record"
  run "$build" TestAll
  refuses "a record naming an include directory which has gone" "names $work/gone/include, which no longer exists"

  sed "s|^set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES \"[^\"]*\")|set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES \"m;c\")|" "$work/record" > "$record"
  run "$build" TestAll
  refuses "a record naming no standard library" 'names 0 standard libraries'
  sed "s|^set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES \"[^\"]*\")|set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES \"c++;stdc++\")|" "$work/record" > "$record"
  run "$build" TestAll
  refuses "a record naming both standard libraries" 'names 2 standard libraries'
  sed "s|^set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES \"[^\"]*\")|set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES \"$other_libraries\")|" "$work/record" > "$record"
  run "$build" TestAll
  refuses "a record naming the standard library the symbols contradict" 'the symbols of .* say otherwise'
  sed "s|^set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES \"[^\"]*\")|set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES \"gcc;c++abi;stdc++fs;${platform_libraries}\")|" \
    "$work/record" > "$record"
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
  field "is keyed relative to the repository" 'sorted(d["defined_symbols_by_object"])' "['Tests/A.cpp', 'Tests/Stale.cpp']"

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
  printf '#!/bin/sh\n[ "$1" = --version ] && echo "fixture nm 9.9"\nexit 0\n' > "$work/silent-nm"; chmod +x "$work/silent-nm"
  sed -i.bak "s|^CMAKE_NM:FILEPATH=.*|CMAKE_NM:FILEPATH=$work/silent-nm|" "$build/CMakeCache.txt"
  run "$build" TestAll
  refuses "an nm which lists nothing" 'nm lists no symbols for'
  printf '#!/bin/sh\nexit 4\n' > "$work/silent-nm"
  run "$build" TestAll
  refuses "a failure of nm" 'failed with exit status 4'
  sed -i.bak "s|^CMAKE_NM:FILEPATH=.*|CMAKE_NM:FILEPATH=$work/absent-nm|" "$build/CMakeCache.txt"
  run "$build" TestAll
  refuses "an nm which cannot be run" 'absent-nm could not be run'
  printf '#!/bin/sh\n[ "$1" = --version ] && { echo "nm, without its number"; exit 0; }\nexec %s "$@"\n' "$nm_path" > "$work/silent-nm"
  sed -i.bak "s|^CMAKE_NM:FILEPATH=.*|CMAKE_NM:FILEPATH=$work/silent-nm|" "$build/CMakeCache.txt"
  run "$build" TestAll
  refuses "an nm which prints no version" 'prints no version'
  sed -i.bak "s|^CMAKE_NM:FILEPATH=.*|CMAKE_NM:FILEPATH=$nm_path|" "$build/CMakeCache.txt"

  rm -rf "$repo/.git"
  GIT_CEILING_DIRECTORIES=$work run "$build" TestAll
  refuses "a source directory outside a repository" 'git -C .* rev-parse --show-toplevel failed'
}

# The formats --------------------------------------------------------------------

# synthetic <path> <kind>: writes a file of the kind named, byte by byte
synthetic() {
  python3 - "$1" "$2" <<'EOF'
import struct, sys
path, kind = sys.argv[1:]

def elf(sections, ident=b"\x7fELF\x02\x01", count_in_section_0=False):
    """An ELF header, then its section headers. Each section is
    (sh_type, sh_size, sh_link)."""
    shoff = 64
    headers = b"".join(struct.pack("<IIQQQQIIQQ", 0, t, 0, 0, 0, size, link, 0, 0, 0)
                       for t, size, link in sections)
    shnum = 0 if count_in_section_0 else len(sections)
    header = ident.ljust(16, b"\0") + struct.pack("<HHIQQQIHHHHHH", 2, 62, 1, 0, 0, shoff, 0, 64, 0, 0, 64, shnum, 0)
    return header + headers

def mach_o(commands, magic=b"\xcf\xfa\xed\xfe"):
    """A Mach-O header, then its load commands, each (cmd, payload)."""
    body = b"".join(struct.pack("<II", cmd, 8 + len(payload)) + payload for cmd, payload in commands)
    return magic + struct.pack("<IIIIIII", 0x100000c, 0, 2, len(commands), len(body), 0, 0) + body

symtab = lambda strsize: (2, struct.pack("<IIII", 0, 10, 0, strsize))
segment = (0x19, b"\0" * 64)
files = {
    "elf":              elf([(0, 0, 0), (2, 48, 2), (3, 1234, 0), (3, 9999, 0)]),
    "elf-dynsym":       elf([(0, 0, 0), (11, 48, 4), (2, 48, 3), (3, 1234, 0), (3, 77, 0)]),
    "elf-many":         elf([(0, 4, 0), (2, 48, 2), (3, 5678, 0), (3, 99, 0)], count_in_section_0=True),
    "elf-no-symtab":    elf([(0, 0, 0), (3, 1234, 0)]),
    "elf-two-symtabs":  elf([(0, 0, 0), (2, 48, 3), (2, 48, 3), (3, 1234, 0)]),
    "elf-link-other":   elf([(0, 0, 0), (2, 48, 2), (1, 1234, 0)]),
    "elf-link-beyond":  elf([(0, 0, 0), (2, 48, 7), (3, 1234, 0)]),
    "elf-32":           elf([(0, 0, 0), (2, 48, 2), (3, 1234, 0)], ident=b"\x7fELF\x01\x01"),
    "elf-big-endian":   elf([(0, 0, 0), (2, 48, 2), (3, 1234, 0)], ident=b"\x7fELF\x02\x02"),
    "elf-truncated":    elf([(0, 0, 0), (2, 48, 2), (3, 1234, 0)])[:100],
    "mach-o":           mach_o([segment, symtab(4321), segment]),
    "mach-o-no-symtab": mach_o([segment]),
    "mach-o-two-symtabs": mach_o([symtab(1), symtab(2)]),
    "mach-o-zero-command": mach_o([segment])[:32] + struct.pack("<II", 0x19, 0) + b"\0" * 64,
    "mach-o-truncated": mach_o([symtab(4321), segment])[:-10],
    "mach-o-32":        mach_o([symtab(4321)], magic=b"\xce\xfa\xed\xfe"),
    "mach-o-fat":       b"\xca\xfe\xba\xbe" + b"\0" * 60,
    "pe":               b"MZ" + b"\0" * 62,
    "empty":            b"",
}
with open(path, "wb") as f:
    f.write(files[kind])
EOF
}

# string_table <kind>: the subject's format_and_string_table_size on a
# synthetic file of that kind, its output in $out, its errors in $err, and its
# exit status in $rc
string_table() {
  synthetic "$work/$1" "$1"
  python3 -B -c 'import importlib.util, sys
spec = importlib.util.spec_from_file_location("subject", sys.argv[1])
subject = importlib.util.module_from_spec(spec)
spec.loader.exec_module(subject)
print(*subject.format_and_string_table_size(sys.argv[2]))' "$subject" "$work/$1" > "$out" 2> "$err"
  rc=$?
}

format_controls() {
  string_table elf
  exits "an ELF file is read" 0
  check "an ELF file's string table is the symbol table's" yes '^ELF 1234$' "$out"
  string_table elf-dynsym
  exits "an ELF file with a dynamic symbol table is read" 0
  check "a dynamic symbol table is not the symbol table" yes '^ELF 1234$' "$out"
  string_table elf-many
  exits "an ELF file counting its sections in section 0 is read" 0
  check "every section counted in section 0 is read" yes '^ELF 5678$' "$out"
  string_table elf-no-symtab;      refuses "an ELF file with no symbol table" 'has 0 symbol tables, rather than one'
  string_table elf-two-symtabs;    refuses "an ELF file with two symbol tables" 'has 2 symbol tables, rather than one'
  string_table elf-link-other;     refuses "a symbol table linked to another section" 'does not link to a string table'
  string_table elf-link-beyond;    refuses "a symbol table linked beyond the sections" 'does not link to a string table'
  string_table elf-32;             refuses "a 32-bit ELF file" 'not a 64-bit little-endian one'
  string_table elf-big-endian;     refuses "a big-endian ELF file" 'not a 64-bit little-endian one'
  string_table elf-truncated;      refuses "a truncated ELF file" 'is truncated'

  string_table mach-o
  exits "a Mach-O file is read" 0
  check "a Mach-O file's string table is LC_SYMTAB's" yes '^Mach-O 4321$' "$out"
  string_table mach-o-no-symtab;   refuses "a Mach-O file with no LC_SYMTAB" 'has 0 LC_SYMTAB commands, rather than one'
  string_table mach-o-two-symtabs; refuses "a Mach-O file with two LC_SYMTABs" 'has 2 LC_SYMTAB commands, rather than one'
  string_table mach-o-zero-command; refuses "a load command of size 0" 'has a load command of size 0'
  string_table mach-o-truncated;   refuses "a truncated Mach-O file" 'is truncated'

  local kind
  for kind in mach-o-32 mach-o-fat pe empty; do
    string_table "$kind"
    refuses "a file of kind $kind" 'is neither a 64-bit little-endian ELF file nor'
  done
}

run_controls() { # run_controls <script>: sets fails, and total, the controls run
  subject=$1 fails=0 total=0
  local control
  for control in record_controls record_selection_controls object_controls tool_controls format_controls; do
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
    ('a failed command read',            'if r.returncode:\n        sys.exit(f"{\' \'.join(command)}', 'if False:\n        sys.exit(f"{\' \'.join(command)}'),
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
    ('a directory without a record',     '\n                     and os.path.isfile(os.path.join(files, d, "CMakeCXXCompiler.cmake")))',
                                         ')'),
    ('an empty record entry accepted',   'if not entry(record, name, path):', 'if entry(record, name, path) is None:'),
    ('the linker id unchecked',          '"CMAKE_CXX_COMPILER_LINKER_ID",\n', '\n'),
    ('the include directories unchecked for staleness',
                                         'for named in [record["CMAKE_CXX_COMPILER"], *record["CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES"].split(";")]:',
                                         'for named in [record["CMAKE_CXX_COMPILER"]]:'),
    ('the compiler unchecked for staleness',
                                         'for named in [record["CMAKE_CXX_COMPILER"], *record',
                                         'for named in [*record'),
    ('a library matched within a name',  'if library in linked]', 'if any(library in name for name in linked)]'),
    ('two standard libraries accepted',  'if len(named) != 1:', 'if not named:'),
    ('no standard library accepted',     'if len(named) != 1:', 'if len(named) > 1:'),
    ('the symbols not consulted',        'if any(LIBCXX_NAMESPACE in line for line in listed) != (library == "libc++"):',
                                         'if False:'),
    ('an empty listing accepted',        'if not listed:\n        sys.exit(f"nm lists no symbols', 'if False:\n        sys.exit(f"nm lists no symbols'),
    ('the PATH nm run',                  'listed = symbols(nm, executable)', 'listed = symbols("nm", executable)'),
    ('an object counts every symbol',    'objects_by_source.values())))',
                                         'objects_by_source.values())))\n    counts = {k: len(symbols(nm, os.path.join(build_dir, o)))'
                                         ' for k, o in objects_by_source.items()}'),
    ('every input an object',            'if line.endswith(".o")]', 'if line]'),
    ('no objects accepted',              'if not linked:\n        sys.exit(f"ninja lists no objects', 'if False:\n        sys.exit(f"ninja lists no objects'),
    ('an unknown object skipped',        'sys.exit(f"ninja\'s compilation database has no entry for {obj}")', 'continue'),
    ('a source outside kept',            'if os.path.commonpath([source, root]) != root:', 'if False:'),
    ('a source matched by prefix',       'if os.path.commonpath([source, root]) != root:', 'if not source.startswith(root):'),
    ('a source keyed absolutely',        'key = os.path.relpath(source, root)', 'key = source'),
    ('a source not resolved from the build directory',
                                         'os.path.join(compilation["directory"], compilation["file"])', 'compilation["file"]'),
    ('a shared source overwritten',      'if key in out:', 'if False:'),
    ('a bad database crashes',           'except (json.JSONDecodeError, TypeError, KeyError) as e:', 'except json.JSONDecodeError as e:'),
    ('ELF of any class',                 'if (elf_class, data_encoding) != (2, 1):', 'if data_encoding != 1:'),
    ('ELF of any byte order',            'if (elf_class, data_encoding) != (2, 1):', 'if elf_class != 2:'),
    ('the section count in the header only', 'if shnum == 0 and shoff:', 'if False:'),
    ('the first symbol table of several','if len(symbol_tables) != 1:', 'if not symbol_tables:'),
    ('a symbol table linked anywhere',   'if link >= len(sections) or sections[link][0] != SHT_STRTAB:', 'if link >= len(sections):'),
    ('a link beyond the sections crashes', 'if link >= len(sections) or sections[link][0]', 'if sections[link][0]'),
    ('the dynamic symbol table read',    'symbol_tables = [s for s in sections if s[0] == SHT_SYMTAB]',
                                         'symbol_tables = [s for s in sections if s[0] in (SHT_SYMTAB, 11)][-1:]'),
    ('the largest string table read',    'return sections[link][1]', 'return max(s[1] for s in sections if s[0] == SHT_STRTAB)'),
    ('a truncated read accepted',        'if len(data) != size:', 'if False:'),
    ('a zero-sized command loops',       'if cmdsize == 0:\n            sys.exit', 'if False:\n            sys.exit'),
    ('the last LC_SYMTAB of several',    'if len(sizes) != 1:', 'if not sizes:'),
    ('a symbol table entry for its strings',
                                         'sizes.append(struct.unpack_from("<IIII", commands, offset + 8)[3])',
                                         'sizes.append(struct.unpack_from("<IIII", commands, offset + 8)[1])'),
    ('any magic read as Mach-O',         'if magic == MACH_O_MAGIC:', 'if True:'),
    ('the nm version from the first line', 'if re.search(r"\\d", line)]', 'if line]'),
    ('no nm version accepted',           'if not lines:\n        sys.exit(f"{nm} --version', 'if False:\n        sys.exit(f"{nm} --version'),
    ('a dirty tree called clean',        '"clean": run(["git", "-C", root, "status", "--porcelain", "--untracked-files=all"]) == "",',
                                         '"clean": True,'),
    ('untracked files as git is configured',
                                         '"status", "--porcelain", "--untracked-files=all"]', '"status", "--porcelain"]'),
    ('the type\'s flags left out',       'if build_type:\n        flags +=', 'if False:\n        flags +='),
    ('the linker\'s flags left out',     'flags = ["CMAKE_CXX_FLAGS", "CMAKE_EXE_LINKER_FLAGS"]', 'flags = ["CMAKE_CXX_FLAGS"]'),
    ('the linker named as the compiler', '"linker":   {"path":    record["CMAKE_CXX_COMPILER_LINKER"],', '"linker":   {"path":    record["CMAKE_CXX_COMPILER"],'),
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
