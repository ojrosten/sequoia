#!/bin/bash
# Usage: cron_line_at.sh <workflow file> <epoch second>
#
# Prints the schedule that <workflow file>, a path from the top of the
# repository, carried on the checked-out branch at <epoch second>: the
# expression of its `cron:` line, or nothing if the branch then held no such
# file, or held it with no `cron:` line. GitHub fires a schedule only from a
# file on the default branch, so the watchdog asks this whether a schedule was
# in force at its tick.
#
# The branch's state at a time is that of the newest commit on its first-parent
# chain whose committer time is not after it. Commits reach main and staging
# only by the merges GitHub makes for pull requests, and GitHub stamps each
# merge when it makes it. A commit's own time says when it was written, not
# when it arrived: a cron line committed before a tick, and merged after it,
# was not in force at the tick.
#
# The history must reach back to the time. A checkout too shallow to say is an
# error, never an absent file, as is a file with more than one `cron:` line,
# or one that cannot be read. An error exits 1, with an ::error::. Arguments of
# any other form are refused with status 2, before anything is read.

set -u

usage() {
  echo "Usage: $0 <workflow file> <epoch second>" >&2
  exit 2
}

[ $# -eq 2 ] && [ -n "$1" ] && [[ $2 =~ ^(0|[1-9][0-9]*)$ ]] || usage
workflow=$1 time=$2

error() {
  echo "::error::$workflow at $(date -u -d "@$time" +%Y-%m-%dT%H:%M:%S+00:00 2> /dev/null || echo "$time"): $1"
  exit 1
}

top=$(git rev-parse --show-toplevel) || error "not in a git repository"
cd "$top" || error "cannot enter $top"

commit=$(git rev-list -1 --first-parent --before="@$time" HEAD) || error "the history of HEAD cannot be read"
[ -n "$commit" ] || error "the history of HEAD does not reach back that far"

listed=$(git ls-tree --name-only "$commit" -- "$workflow") || error "the tree of $commit cannot be read"
[ -n "$listed" ] || exit 0

text=$(git show "$commit:$workflow") || error "the file cannot be read from $commit"
cron=$(sed -n "s/^ *- cron: '\(.*\)'$/\1/p" <<< "$text")
lines=$(printf '%s' "$cron" | grep -c .)
[ "$lines" -le 1 ] || error "$commit carries $lines cron lines, not one"
[ -z "$cron" ] || echo "$cron"
