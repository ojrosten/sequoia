#!/bin/bash
# Usage: report_crashes.sh <dump directory> <since file>
#
# Reports every crash whose dump is newer than <since file>, with its stacks.
# A suite that crashes ends with an exit status and nothing else. The stacks
# say where it stopped.
#
# The dumps depend on the platform:
#   - Windows: the minidumps that Windows Error Reporting wrote to
#     <dump directory>, read with cdb;
#   - Linux: the cores that the kernel wrote to <dump directory>, each named
#     core.<process name>.<pid>, read with gdb;
#   - macOS: the crash reports that the system wrote to the directories
#     macos_crash_report_directories lists. <dump directory> is not read.
#
# The output, in order:
#   - on macOS, a line beginning "Not read:" for each listed directory that
#     cannot be read;
#   - for each crash, a line beginning "== Crash:", then its stacks. A crash
#     whose stacks cannot be read is still reported and counted, and the
#     report says why;
#   - "Crashes found: <n>".
# Errors from the tools the script runs go to standard error, and may fall
# between any of these lines. The status is 0 if the script reports, and 2 if it refuses its
# arguments.

set -u

if [ $# -ne 2 ]; then
  echo "Usage: $0 <dump directory> <since file>" >&2
  exit 2
fi

dumps=$1 since=$2

if [ ! -d "$dumps" ] || [ ! -f "$since" ]; then
  echo "$0: <dump directory> must be a directory, and <since file> must be a file: '$dumps', '$since'" >&2
  exit 2
fi

frames_per_thread=50
found=0

source "$(dirname "$0")/windows_debugger.sh"
source "$(dirname "$0")/macos_crash_reports.sh"

case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) platform=windows ;;
  Darwin)               platform=macos   ;;
  *)                    platform=linux   ;;
esac

# cdb reads an executable's symbols from the path the linker recorded in it.
# It downloads the system libraries' symbols from Microsoft's symbol server,
# and caches them in <dump directory>/symbols.
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

# A core's name carries the kernel's name for the process, not the executable's
# path. file reads that path from the core itself.
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

# Whether a report in the directories is of a crash. The directories also
# hold reports of other kinds. An .ips report begins with a line of JSON
# metadata, whose bug type is 309 for a crash. A .crash report is the older
# form of a crash report.
is_crash_report() {
  case "$1" in
    *.crash) return 0 ;;
    *)       head -1 "$1" | grep -q '"bug_type":"309"' ;;
  esac
}

# Prints the exception of an .ips crash report, then the frames of its
# faulting thread. The system gives a frame its source file and line, and
# lists the frames inlined into it, when it can read the executable's debug
# information.
summarise_ips_report() { # summarise_ips_report <report>
  python3 - "$1" "$frames_per_thread" <<'SUMMARY'
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
    location = f" ({frame['sourceFile']}:{frame['sourceLine']})" if {"sourceFile", "sourceLine"} <= frame.keys() else ""
    inlined = " [inlined]" if frame.get("inline") else ""
    print(f"  {image}: {frame.get('symbol', hex(frame.get('imageOffset', 0)))}{location}{inlined}")
SUMMARY
}

# Prints each crash report's path, then a summary of an .ips report, then the
# whole report. A .crash report is not JSON, and gets no summary. If an .ips
# report cannot be summarised, the output says so. find names any
# subdirectory it cannot read, in an error.
report_macos() {
  local directory report directories=()
  while IFS= read -r directory; do
    [ -d "$directory" ] || continue
    if [ -r "$directory" ] && [ -x "$directory" ]; then
      directories+=("$directory")
    else
      echo "Not read: $directory cannot be read as $(id -un), so a crash reported there is not counted"
    fi
  done < <(macos_crash_report_directories)
  [ ${#directories[@]} -gt 0 ] || return 0

  while IFS= read -r report; do
    is_crash_report "$report" || continue
    found=$((found + 1))
    echo "== Crash: $(basename "$report") =="
    echo "Report: $report"
    case "$report" in
      *.ips) summarise_ips_report "$report" || echo "The report could not be summarised; it follows whole." ;;
    esac
    echo "-- The whole report --"
    cat "$report"
    echo
  done < <(find "${directories[@]}" -type f -newer "$since" \( -name '*.ips' -o -name '*.crash' \))
}

report_$platform
echo "Crashes found: $found"
