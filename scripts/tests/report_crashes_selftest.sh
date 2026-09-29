#!/bin/bash
# Controls for report_crashes.sh, run on this machine's platform.
#
# The stand-in for a crashing suite is compiled here and aborts in a function whose name the
# report must show. Each control is a claim the script exists to keep:
#
#   - a crash since the since file is reported, with the function it crashed in, and counted;
#   - a crash before the since file is not, so a job reports only its own suite's crashes;
#   - with no crash, the count is zero, rather than the last line being absent;
#   - arguments which do not name a directory and a file are refused.
#
# Only macOS's branch runs here: the system writes its crash reports itself, a few seconds after
# the crash. The Windows and Linux branches read dumps the job arranges for, and are exercised in
# CI, where the workflow's control crash checks the arrangement on every run.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script="$here/../report_crashes.sh"
tmp=$(mktemp -d)
reports="$HOME/Library/Logs/DiagnosticReports"
trap 'rm -rf "$tmp"; rm -f "$reports"/CrashingStandIn*' EXIT
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

mkdir "$tmp/dumps"
touch "$tmp/before"
sleep 1

"$tmp/$name" 2> /dev/null

# The system writes the report a few seconds after the crash.
for attempt in $(seq 1 60); do
  find "$reports" -type f -newer "$tmp/before" -name "$name*" 2> /dev/null | grep -q . && break
  sleep 1
done
sleep 1
touch "$tmp/after"

bash "$script" "$tmp/dumps" "$tmp/before" > "$tmp/crashed.txt" 2>&1
check "a crash since the since file is reported"          yes "^== Crash: $name" "$tmp/crashed.txt"
check "the report shows the function the crash was in"    yes "crashes_in_the_stand_in" "$tmp/crashed.txt"
check "a crash since the since file is counted"           yes "^Crashes found: [1-9]" "$tmp/crashed.txt"

bash "$script" "$tmp/dumps" "$tmp/after" > "$tmp/earlier.txt" 2>&1
check "a crash before the since file is not reported"     no  "^== Crash: $name" "$tmp/earlier.txt"
check "with no crash since, the count is zero"            yes "^Crashes found: 0$" "$tmp/earlier.txt"

for arguments in "" "$tmp/dumps" "$tmp/nowhere $tmp/before" "$tmp/dumps $tmp/nothing"; do
  # Word splitting is the point: each case is a list of arguments.
  # shellcheck disable=SC2086
  bash "$script" $arguments > /dev/null 2>&1
  refusal=$?
  [ "$refusal" -eq 2 ] || fail "the arguments '$arguments' were not refused (status $refusal)"
done

if [ "$fails" -eq 0 ]; then echo "report_crashes: all controls pass"; else exit 1; fi
