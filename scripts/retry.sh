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
# An attempt runs in a process group of its own. To stop it, the script sends
# SIGTERM to the group, and SIGKILL 10 s later if any of the group is still
# running, so that what the command started stops with it. A signal reaches
# only the processes the script's user may signal. So for a command that runs
# as root, run this script as root, with sudo, rather than the command: a
# signal from an ordinary user stops sudo, and sudo relays SIGTERM to its own
# child alone, never to what that child started.
#
# The command's standard input is /dev/null.
#
# <attempts> and <seconds> are positive whole numbers with no leading zero.
# Arguments of any other form are refused with status 2, before anything runs.
# A temporary directory the script cannot create ends it with status 2 too.
#
# RETRY_PAUSE_SECONDS replaces the 15 s unit of the pauses, for the selftest.

set -u

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
  # Job control gives the command a process group of its own. It is on only
  # while the command starts, since it also reports, in words that read like
  # errors, a job that ends on a signal.
  set -m
  "$@" < /dev/null &
  command=$!
  set +m
  # The watcher marks the attempt as stopped before signalling it, so that
  # the status below can be told from the command's own. After SIGTERM it
  # looks each second for what is left of the group, and kills that at the
  # end of the grace. Told to stop by SIGTERM itself, it ends its sleep and
  # exits 0, so that no job ends on a signal.
  (
    trap 'kill "$sleeper" 2> /dev/null; exit 0' TERM
    sleep "$limit" &
    sleeper=$!
    wait "$sleeper"
    touch "$flag/stopped"
    kill -TERM -- "-$command" 2> /dev/null
    for ((waited = 0; waited < grace; waited++)); do
      kill -0 -- "-$command" 2> /dev/null || exit 0
      sleep 1
    done
    kill -KILL -- "-$command" 2> /dev/null
  ) &
  watcher=$!
  wait "$command" 2> /dev/null
  status=$?

  if [ -e "$flag/stopped" ]; then
    # The group may outlive its leader, so the watcher finishes its grace.
    wait "$watcher" 2> /dev/null
    echo "$* did not finish within $limit s (attempt $attempt of $attempts)"
  else
    kill "$watcher" 2> /dev/null
    wait "$watcher" 2> /dev/null
    [ "$status" -ne 0 ] || exit 0
    echo "$* failed with status $status (attempt $attempt of $attempts)"
  fi
  if [ "$attempt" -lt "$attempts" ]; then
    echo "Retrying in $((attempt * pause)) s"
    sleep $((attempt * pause))
  fi
done

echo "::error::$* failed on all $attempts attempts"
exit 1
