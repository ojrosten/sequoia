#!/bin/bash
# Usage: snapshot_at_deadline.sh <seconds> <snapshot file> <executable name>
#                                -- <command>...
#
# Runs <command> and returns its exit status. <command> has the script's
# standard input, output and error. If <command> is still running after
# <seconds>, the script writes to <snapshot file> what the machine is doing:
#   - every process, with its parent and command line;
#   - the stack of every thread of each process named <executable name>.
# The script begins the snapshot within a second of the deadline, and finishes
# it before returning, so a caller never reads half of one. A command which
# ends more than a second before the deadline leaves no file.
#
# <seconds> is a positive whole number with no leading zero. <snapshot file>
# is a path at which the script can create a file, and at which nothing
# exists, not even a symbolic link. The script refuses arguments of any other
# form with status 2, before running <command>. If the script cannot create a
# temporary directory under TMPDIR, it exits with status 2 before running
# <command>, after mktemp's message. On Linux, a process's name is the first
# fifteen characters of its executable's name, so a longer <executable name>
# matches no process.
#
# The script exists for a suite which hangs. A step timeout kills such a suite
# without a word, and a suite which reports only at the end of a run leaves
# nothing in its log to say where it stopped. The process list tells a hung
# child process, such as a build or a git command, from a test stuck in the
# suite itself, and the stacks say which test.
#
# Where the script cannot take a part of the snapshot, the snapshot says why:
# in a line naming the tool which is absent, or in the complaint of the tool
# which failed. So an empty stacks section never reads as a process with no
# threads.

set -u

if [ $# -lt 5 ] || [ "$4" != "--" ]; then
  echo "Usage: $0 <seconds> <snapshot file> <executable name> -- <command>..." >&2
  exit 2
fi

seconds=$1 snapshot=$2 name=$3
shift 4

# A deadline must be a positive whole number with no leading zero. Any other
# would pass in silence: a deadline the arithmetic cannot read gives no
# snapshot, zero or less gives one at once, and a leading zero makes the
# arithmetic read the deadline as octal.
if ! [[ $seconds =~ ^[1-9][0-9]*$ ]]; then
  echo "$0: <seconds> must be a positive whole number with no leading zero, not '$seconds'" >&2
  exit 2
fi

# The watcher's errors go nowhere, so a snapshot it cannot write would be lost
# in silence. So the script creates the file here, and removes it. A file
# already there would read as this run's snapshot, and the creation would
# empty it.
if [ -e "$snapshot" ] || [ -L "$snapshot" ] || ! { : > "$snapshot"; } 2> /dev/null; then
  echo "$0: <snapshot file> must be a new file which the script can create, not '$snapshot'" >&2
  exit 2
fi
rm -f "$snapshot"

source "$(dirname "$0")/windows_debugger.sh"

cdb_frames_per_thread=50
sample_duration_seconds=1

case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) platform=windows ;;
  Darwin)               platform=macos   ;;
  *)                    platform=linux   ;;
esac

# On Windows, PowerShell both lists the processes and finds those with the
# name, since cdb takes Windows process ids, which Git Bash's ps does not show
# by default.
list_processes() {
  case $platform in
    windows) powershell -NoProfile -Command \
               "Get-CimInstance Win32_Process | Sort-Object ProcessId | Format-Table ProcessId, ParentProcessId, CreationDate, CommandLine -AutoSize -Wrap | Out-String -Width 400" ;;
    macos)   ps -axo pid,ppid,etime,command ;;
    linux)   ps -eo pid,ppid,etime,args --forest ;;
  esac
}

find_processes() {
  case $platform in
    windows) powershell -NoProfile -Command "(Get-Process -Name '$name' -ErrorAction SilentlyContinue).Id" | tr -d '\r' ;;
    *)       pgrep -x "$name" ;;
  esac
}

# cdb's -pv attaches without stopping the process for good, and `q` then
# detaches rather than killing the process. Ubuntu's kernel lets gdb attach
# only to gdb's own descendants unless gdb runs as root, so gdb runs through
# sudo where sudo needs no password.
dump_stacks() { # dump_stacks <pid>
  case $platform in
    windows)
      local cdb
      cdb=$(windows_debugger_path)
      if [ -n "$cdb" ]; then
        "$cdb" -pv -p "$1" -c "~*k $cdb_frames_per_thread; q"
      else
        echo "No stacks: cdb.exe is not installed where the Windows SDK puts it."
      fi
      ;;
    macos)
      sample "$1" "$sample_duration_seconds" -file /dev/stdout
      ;;
    linux)
      if ! command -v gdb > /dev/null; then
        echo "No stacks: gdb is not on PATH."
      elif sudo -n true 2> /dev/null; then
        sudo -n gdb -p "$1" -batch -ex "thread apply all bt"
      else
        gdb -p "$1" -batch -ex "thread apply all bt"
      fi
      ;;
  esac
}

take_snapshot() {
  echo "Snapshot taken at $(date -u +%Y-%m-%dT%H:%M:%SZ), ${seconds}s after the command began."
  echo
  echo "== Processes =="
  list_processes
  echo

  local pids pid
  pids=$(find_processes)
  if [ -z "$pids" ]; then
    echo "== No process named '$name' is running =="
  fi

  for pid in $pids; do
    echo "== Stacks of '$name', process $pid =="
    dump_stacks "$pid"
    echo
  done

  echo "Snapshot finished at $(date -u +%Y-%m-%dT%H:%M:%SZ)."
}

# The watcher stops when a file says the command has ended, or when the script
# has gone. A cancelled step kills the script, and the watcher would otherwise
# take a snapshot nobody waits for. The watcher looks for both once a second,
# rather than being stopped by a signal: a signal arriving before the
# watcher's trap is set is lost, and one arriving between a sleep's start and
# the watcher learning the sleep's id leaves the sleep running. So the script
# returns up to a second after the command ends. The watcher looks for the
# file once more immediately before the snapshot. $SECONDS counts whole
# seconds, so the snapshot begins within a second of the deadline, and a
# command which ends in the second before the deadline may still be
# snapshotted.
watch_for_deadline() {
  local deadline=$((SECONDS + seconds))
  while :; do
    if [ -e "$finished" ] || ! kill -0 $$ 2> /dev/null; then
      return
    fi
    [ "$SECONDS" -lt "$deadline" ] || break
    sleep 1
  done
  take_snapshot > "$snapshot" 2>&1
}

# The template puts the directory under TMPDIR, which macOS's `mktemp -d`
# ignores when given no template.
flag_dir=$(mktemp -d "${TMPDIR:-/tmp}/snapshot_at_deadline.XXXXXX") || exit 2
finished="$flag_dir/finished"

watch_for_deadline < /dev/null > /dev/null 2>&1 &
watcher=$!

"$@"
status=$?

touch "$finished"
wait "$watcher"
rm -rf "$flag_dir"
exit "$status"
