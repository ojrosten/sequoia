#!/bin/bash
# Usage: retry.sh <attempts> <seconds> -- <command>...
#
# Runs <command> until it succeeds, at most <attempts> times, and exits 0 at
# the first success. An attempt still running after <seconds> is stopped, and
# counts as a failure, so that a command which hangs is retried rather than
# holding its step until the job's timeout. Between attempts the script pauses,
# 15 s after the first, 30 s after the second, and so on. If every attempt
# fails, it prints an ::error:: naming the command and exits 1.
#
# An attempt runs in a process group of its own, and is stopped with SIGTERM to
# the group, then SIGKILL 10 s later if any of it is still running, so that
# what the command started stops with it. A command run through sudo is
# stopped too: sudo keeps its caller's real user id, so the caller may signal
# it, and it relays SIGTERM to its command. SIGKILL reaches sudo alone, since
# its command runs as root.
#
# The command's standard input is /dev/null.
#
# Arguments of any other form are refused with status 2, before anything runs:
# <attempts> and <seconds> are positive whole numbers with no leading zero.
#
# RETRY_PAUSE_SECONDS replaces the 15 s unit of the pauses, for the selftest.

set -u
# Job control gives each background command a process group of its own.
set -m

usage() {
  echo "Usage: $0 <attempts> <seconds> -- <command>..." >&2
  exit 2
}

positive() { [[ $1 =~ ^[1-9][0-9]*$ ]]; }

[ $# -ge 4 ] && [ "$3" = -- ] || usage
attempts=$1 limit=$2
positive "$attempts" && positive "$limit" || usage
shift 3
pause=${RETRY_PAUSE_SECONDS:-15}
grace=10

flag=$(mktemp -d "${TMPDIR:-/tmp}/retry.XXXXXX") || exit 2
trap 'rm -rf "$flag"' EXIT

for ((attempt = 1; attempt <= attempts; attempt++)); do
  rm -f "$flag/stopped"
  "$@" < /dev/null &
  command=$!
  # The watcher marks the attempt as stopped before signalling it, so that
  # the status below can be told from the command's own.
  (
    sleep "$limit"
    touch "$flag/stopped"
    kill -TERM -- "-$command" 2> /dev/null
    sleep "$grace"
    kill -KILL -- "-$command" 2> /dev/null
  ) &
  watcher=$!
  wait "$command"
  status=$?
  # The watcher's own sleep is ended with it, so that nothing outlives the
  # attempt.
  pkill -P "$watcher" 2> /dev/null
  kill "$watcher" 2> /dev/null
  wait "$watcher" 2> /dev/null

  if [ -e "$flag/stopped" ]; then
    echo "$1 did not finish within $limit s (attempt $attempt of $attempts)"
  elif [ "$status" -eq 0 ]; then
    exit 0
  else
    echo "$1 failed with status $status (attempt $attempt of $attempts)"
  fi
  if [ "$attempt" -lt "$attempts" ]; then
    echo "Retrying in $((attempt * pause)) s"
    sleep $((attempt * pause))
  fi
done

echo "::error::$1 failed on all $attempts attempts"
exit 1
