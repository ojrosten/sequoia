#!/bin/bash
# Usage: snapshot_at_deadline.sh <seconds> <snapshot file> <executable name> -- <command>...
#
# Runs <command> and returns its exit status. If it is still running after
# <seconds>, writes to <snapshot file> what the machine is doing: every process,
# with its parent and command line, followed by the stack of every thread of
# each process running <executable name>. A snapshot begun is finished before
# this returns, so a caller never reads half of one; a command which ends
# before the deadline leaves no file.
#
# For a suite that hangs. A step timeout kills the suite without a word, and a
# suite which reports only at the end of a run leaves nothing in its log to say
# where it stopped. The process list tells a hung child process (a build, a git
# command) from a test stuck in the suite itself, and the stacks say which test.
#
# Every part of the snapshot which could not be taken says so, and why, rather
# than being absent: an empty stack section would otherwise read as a process
# with no threads.

set -u

if [ $# -lt 5 ] || [ "$4" != "--" ]; then
  echo "Usage: $0 <seconds> <snapshot file> <executable name> -- <command>..." >&2
  exit 2
fi

seconds=$1 snapshot=$2 name=$3
shift 4

case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) platform=windows ;;
  Darwin)               platform=macos   ;;
  *)                    platform=linux   ;;
esac

# Git Bash's ps lists only the processes it started itself, so on Windows both
# the listing and the search go through PowerShell, whose process ids are the
# ones a debugger takes.
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

# The Windows debugger comes with the SDK's Debugging Tools, when they were
# installed; -pv attaches without stopping the process for good, and `q` then
# detaches rather than killing it. Linux restricts attaching to a process which
# is not a child, hence sudo where it can be had without a password.
dump_stacks() { # dump_stacks <pid>
  case $platform in
    windows)
      local cdb
      for cdb in "/c/Program Files (x86)/Windows Kits/10/Debuggers/x64/cdb.exe" \
                 "/c/Program Files/Windows Kits/10/Debuggers/x64/cdb.exe"; do
        if [ -x "$cdb" ]; then
          "$cdb" -pv -p "$1" -c "~*k 50; q"
          return
        fi
      done
      echo "No stacks: cdb.exe is not installed where the Windows SDK puts it."
      ;;
    macos)
      sample "$1" 1 -file /dev/stdout
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

# The watcher is told the command has ended by a file, and looks for it once a
# second, rather than being killed: a signal can arrive before the watcher's
# trap is set, or between its sleep starting and that sleep's id being known,
# and an edition which killed it left its sleep running in one run and hung in
# another. It also stops if this script has gone, so that a
# cancelled step does not leave it to take a snapshot of nothing. The cost is
# that the deadline is kept to within a second, and that a command ending in
# that second may still be snapshotted.
watch() {
  local deadline=$((SECONDS + seconds))
  while [ "$SECONDS" -lt "$deadline" ]; do
    if [ -e "$finished" ] || ! kill -0 $$ 2> /dev/null; then
      return
    fi
    sleep 1
  done
  take_snapshot > "$snapshot" 2>&1
}

signals=$(mktemp -d)
finished="$signals/finished"

watch < /dev/null > /dev/null 2>&1 &
watcher=$!

"$@"
status=$?

touch "$finished"
wait "$watcher"
rm -rf "$signals"
exit "$status"
