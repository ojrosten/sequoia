#!/bin/bash
# Controls for report_crashes.sh, run on this machine's platform.
#
# The stand-ins for a crashing suite are compiled here. One aborts in a function whose name the
# report must show. The other is optimised, with line tables, and aborts in a function inlined into
# its caller. The optimised stand-in is compiled and linked in separate steps, as a CMake build is,
# so the system reads the stand-in's debug information through the object file. A dSYM, which CI
# links, gives the report the same fields. Each control is a claim the script exists to keep:
#
#   - a crash since the since file is reported, with the function it crashed in, and counted;
#   - an optimised crash is reported with the inlined function and its source line, marked as
#     inlined, and the function it is inlined into is not marked;
#   - a crash before the since file is not, so a job reports only its own suite's crashes;
#   - with no crash, the count is zero, rather than the last line being absent;
#   - a crash reported in any of the directories is reported, with its path, and counted;
#   - a directory that cannot be read is named, and one that does not exist is not;
#   - the directories listed are the user's and the system's DiagnosticReports;
#   - arguments which do not name a directory and a file are refused.
#
# Only macOS's branch runs here: the system writes its crash reports itself, a few seconds after
# the crash. The Windows and Linux branches read dumps the job arranges for, and are exercised in
# CI, where the workflow's control crash checks the arrangement on every run.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script="$here/../report_crashes.sh"
tmp=$(mktemp -d)
source "$here/../macos_crash_reports.sh"
reports=()
while IFS= read -r directory; do reports+=("$directory"); done < <(macos_crash_report_directories)
trap 'rm -rf "$tmp"; for directory in "${reports[@]}"; do rm -f "$directory"/CrashingStandIn* "$directory"/InliningStandIn*; done' EXIT
fails=0

fail() { echo "FAIL: $1"; fails=$((fails+1)); }

check() { # check <name> <yes|no> <pattern> <file>
  if grep -qE "$3" "$4" 2> /dev/null; then got=yes; else got=no; fi
  [ "$got" = "$2" ] || fail "$1 (expected $2, got $got, pattern: $3)"
}

if [ "$(uname -s)" != Darwin ]; then
  echo "report_crashes: the local controls are macOS's; nothing run here"
  exit 0
fi

name=CrashingStandIn
cat > "$tmp/standin.c" <<'STANDIN'
#include <stdlib.h>
void crashes_in_the_stand_in(void) { abort(); }
int main(void) { crashes_in_the_stand_in(); }
STANDIN
cc -g -O0 -o "$tmp/$name" "$tmp/standin.c" || { echo "FAIL: cannot compile the stand-in"; exit 1; }

inlining=InliningStandIn
cat > "$tmp/inlining.c" <<'STANDIN'
#include <stdlib.h>
static inline __attribute__((always_inline)) void aborts_when_inlined(int code) { if(code) abort(); }
__attribute__((noinline)) void calls_the_inlined_function(int code) { aborts_when_inlined(code); }
int main(int argc, char** argv) { (void)argv; calls_the_inlined_function(argc); }
STANDIN
{ cc -g1 -O2 -c -o "$tmp/inlining.o" "$tmp/inlining.c" && cc -o "$tmp/$inlining" "$tmp/inlining.o"; } \
  || { echo "FAIL: cannot compile the inlining stand-in"; exit 1; }

mkdir "$tmp/dumps"
touch "$tmp/before"
sleep 1

"$tmp/$name" 2> /dev/null
"$tmp/$inlining" 2> /dev/null

# The system writes each report a few seconds after the crash.
for stand_in in "$name" "$inlining"; do
  for attempt in $(seq 1 60); do
    find "${reports[@]}" -type f -newer "$tmp/before" -name "$stand_in*" 2> /dev/null | grep -q . && break
    sleep 1
  done
done
sleep 1
touch "$tmp/after"

bash "$script" "$tmp/dumps" "$tmp/before" > "$tmp/crashed.txt" 2>&1
check "a crash since the since file is reported"          yes "^== Crash: $name" "$tmp/crashed.txt"
check "the report shows the function the crash was in"    yes "crashes_in_the_stand_in" "$tmp/crashed.txt"
check "a crash since the since file is counted"           yes "^Crashes found: [1-9]" "$tmp/crashed.txt"
check "an optimised crash shows the inlined function"     yes "^  $inlining: aborts_when_inlined \(inlining\.c:2\) \[inlined\]$" "$tmp/crashed.txt"
check "the inlining caller is not marked inlined"         yes "^  $inlining: calls_the_inlined_function \(inlining\.c:3\)$" "$tmp/crashed.txt"

bash "$script" "$tmp/dumps" "$tmp/after" > "$tmp/earlier.txt" 2>&1
check "a crash before the since file is not reported"     no  "^== Crash: $name" "$tmp/earlier.txt"
check "with no crash since, the count is zero"            yes "^Crashes found: 0$" "$tmp/earlier.txt"

# A temporary directory stands in for the system's. The first stand-in's report is copied into it,
# and it is listed after an empty directory, an unreadable one and an absent one.
mkdir "$tmp/user-directory" "$tmp/system-directory" "$tmp/locked-directory"
cp "$(find "${reports[@]}" -type f -newer "$tmp/before" -name "$name*" 2> /dev/null | head -1)" "$tmp/system-directory/"
chmod 000 "$tmp/locked-directory"
REPORT_CRASHES_MACOS_DIRECTORIES="$tmp/user-directory:$tmp/system-directory:$tmp/locked-directory:$tmp/absent-directory" \
  bash "$script" "$tmp/dumps" "$tmp/before" > "$tmp/elsewhere.txt" 2>&1
chmod 700 "$tmp/locked-directory"
check "a crash in a second directory is reported"         yes "^Report: $tmp/system-directory/$name" "$tmp/elsewhere.txt"
check "a crash in a second directory is counted"          yes "^Crashes found: 1$" "$tmp/elsewhere.txt"
check "an unreadable directory is named"                  yes "^Not read: $tmp/locked-directory " "$tmp/elsewhere.txt"
check "an absent directory is not named"                  no  "absent-directory" "$tmp/elsewhere.txt"

( unset REPORT_CRASHES_MACOS_DIRECTORIES; source "$here/../macos_crash_reports.sh"; macos_crash_report_directories ) > "$tmp/directories.txt"
check "the user's directory is listed"                    yes "^$HOME/Library/Logs/DiagnosticReports$" "$tmp/directories.txt"
check "the system's directory is listed"                  yes "^/Library/Logs/DiagnosticReports$" "$tmp/directories.txt"

for arguments in "" "$tmp/dumps" "$tmp/nowhere $tmp/before" "$tmp/dumps $tmp/nothing"; do
  # Word splitting is the point: each case is a list of arguments.
  # shellcheck disable=SC2086
  bash "$script" $arguments > /dev/null 2>&1
  refusal=$?
  [ "$refusal" -eq 2 ] || fail "the arguments '$arguments' were not refused (status $refusal)"
done

if [ "$fails" -eq 0 ]; then echo "report_crashes: all controls pass"; else exit 1; fi
