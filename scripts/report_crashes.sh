#!/bin/bash
# Usage: report_crashes.sh <dump directory> <since file>
#
# Prints the stacks of every crash since <since file> was written, then a last line,
# "Crashes found: <n>". Where the crashes are found depends on the platform:
#   - Windows: the minidumps Windows Error Reporting wrote to <dump directory>, read with cdb;
#   - Linux: the cores the kernel wrote to <dump directory>, each named
#     core.<process name>.<pid>, read with gdb;
#   - macOS: the crash reports the system wrote to ~/Library/Logs/DiagnosticReports.
#
# A suite that crashes ends with an exit status and nothing else, and the stacks are what say
# where it was. A crash whose stacks cannot be read is still counted, and says why, rather than
# being absent.

set -u

if [ $# -ne 2 ]; then
  echo "Usage: $0 <dump directory> <since file>" >&2
  exit 2
fi

dumps=$1 since=$2

if [ ! -d "$dumps" ] || [ ! -f "$since" ]; then
  echo "$0: <dump directory> must be a directory and <since file> a file: '$dumps', '$since'" >&2
  exit 2
fi

frames_per_thread=50
found=0

source "$(dirname "$0")/windows_debugger.sh"

case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) platform=windows ;;
  Darwin)               platform=macos   ;;
  *)                    platform=linux   ;;
esac

# The images' symbols come from the paths the linker recorded in them; the system's, from
# Microsoft's symbol server, cached beside the dumps.
report_windows() {
  local cdb dump
  cdb=$(windows_debugger_path)
  while IFS= read -r dump; do
    found=$((found + 1))
    echo "== Crash: $(basename "$dump") =="
    if [ -n "$cdb" ]; then
      "$cdb" -z "$(cygpath -w "$dump")" \
             -y "srv*$(cygpath -w "$dumps")\\symbols*https://msdl.microsoft.com/download/symbols" \
             -c ".ecxr; kb $frames_per_thread; ~*k $frames_per_thread; q"
    else
      echo "No stacks: cdb.exe is not installed where the Windows SDK puts it."
    fi
    echo
  done < <(find "$dumps" -maxdepth 1 -type f -newer "$since" -name '*.dmp')
}

# A core's name carries only the process name the kernel keeps; the executable's path is in the
# core itself, which `file` reads.
report_linux() {
  local core executable
  while IFS= read -r core; do
    found=$((found + 1))
    executable=$(file -b "$core" | sed -n "s/.*execfn: '\([^']*\)'.*/\1/p")
    echo "== Crash: ${executable:-an unknown executable}, $(basename "$core") =="
    if ! command -v gdb > /dev/null; then
      echo "No stacks: gdb is not on PATH."
    elif [ -n "$executable" ] && [ -e "$executable" ]; then
      gdb -batch -ex "thread apply all bt $frames_per_thread" "$executable" "$core"
    else
      echo "The executable was not found, so frames in it are unnamed."
      gdb -batch -ex "thread apply all bt $frames_per_thread" -c "$core"
    fi
    echo
  done < <(find "$dumps" -maxdepth 1 -type f -newer "$since" -name 'core.*')
}

# An .ips report is JSON: a line of metadata, then the body. Only crashes are counted - bug
# type 309 - since the directory also collects reports of other kinds. What a reader wants of a
# crash is the exception and the faulting thread's frames, so those come first; the whole report
# follows, for everything else. A .crash report, the older form, is not JSON, and is printed whole.
is_crash_report() {
  case "$1" in
    *.crash) return 0 ;;
    *)       head -1 "$1" | grep -q '"bug_type":"309"' ;;
  esac
}

report_macos() {
  local report
  while IFS= read -r report; do
    is_crash_report "$report" || continue
    found=$((found + 1))
    echo "== Crash: $(basename "$report") =="
    python3 - "$report" "$frames_per_thread" <<'SUMMARY' || echo "The report could not be summarised; it follows whole."
import json, sys
with open(sys.argv[1]) as f:
    metadata, body = f.readline(), json.loads(f.read())
exception = body.get("exception", {})
print(f"{json.loads(metadata).get('name', '?')}: {exception.get('type', '?')} ({exception.get('signal', '?')})")
images = body.get("usedImages", [])
faulting = body["threads"][body["faultingThread"]]
for frame in faulting["frames"][:int(sys.argv[2])]:
    index = frame.get("imageIndex", -1)
    image = images[index].get("name", "?") if 0 <= index < len(images) else "?"
    print(f"  {image}: {frame.get('symbol', hex(frame.get('imageOffset', 0)))}")
SUMMARY
    echo "-- The whole report --"
    cat "$report"
    echo
  done < <(find "$HOME/Library/Logs/DiagnosticReports" -type f -newer "$since" \( -name '*.ips' -o -name '*.crash' \) 2> /dev/null)
}

report_$platform
echo "Crashes found: $found"
