#!/bin/bash
# Prints the record which a data branch holds for the first parent of a
# commit, waiting while a run of the recording workflow may still store it:
#
#   fetch_first_parent_record.sh <remote> <branch> <data branch> <directory>
#                                <workflow> <commit> <seconds>
#
# The first parent of <commit> must be on the first-parent history of
# <branch> of <remote>, as the remote holds the branch when the script
# starts. The record is the file <directory>/<first parent>.json on <data
# branch> of <remote>, as store_symbol_table_record.sh stores it. The script
# runs in a clone of the remote holding <commit>, whose objects it adds to.
# gh must be authenticated for the repository, and GH_REPO must name it.
#
# A push to <branch> starts a run of <workflow>, a file in
# .github/workflows, which stores the record of the commit pushed before the
# run finishes. So, while such a run for the first parent is unfinished, the
# script fetches the data branch again every 60 seconds, for up to <seconds>
# seconds in all. It prints the record as soon as the branch holds it, and
# exits with status 0. It waits for no run of another event, such as a
# dispatch recording the commit afresh.
#
# The script fails with status 1, saying why on standard error and printing
# nothing, if it finds no record to print:
#   - <commit> is not a commit of the clone, or has no parent;
#   - the first parent is not on the branch's first-parent history, so that
#     no push to the branch recorded it;
#   - no run of the workflow that a push started for the first parent is
#     unfinished, and the data branch holds no record of it;
#   - a run is still unfinished after <seconds> seconds.
# In each, there is no baseline, and the script fails rather than let a gate
# pass without one. It also fails with status 1 if git or gh fails, since a
# record or a run not found may then exist. Arguments of any other form are
# refused with status 2.
#
# FETCH_POLL_SECONDS replaces the 60 seconds between fetches, for the
# selftest.

set -u

die() { echo "$*" >&2; exit 1; }

if [ $# -ne 7 ] || [[ ! $7 =~ ^(0|[1-9][0-9]*)$ ]]; then
  echo "Usage: $0 <remote> <branch> <data branch> <directory> <workflow> <commit> <seconds>" >&2
  exit 2
fi
remote=$1 branch=$2 data_branch=$3 directory=$4 workflow=$5 commit=$6 seconds=$7
poll_seconds=${FETCH_POLL_SECONDS:-60}

parent=$(git rev-parse --verify --quiet "$commit^1^{commit}") \
  || die "$commit is not a commit of this clone, or has no parent"
git fetch --quiet --no-tags "$remote" "refs/heads/$branch" || die "git fetch $remote $branch failed"
tip=$(git rev-parse --verify FETCH_HEAD) || die "git fetch $remote $branch left no FETCH_HEAD"
# grep reads the whole list, so that rev-list never meets a closed pipe.
git rev-list --first-parent "$tip" | grep -xF "$parent" > /dev/null \
  || die "The first parent of $commit, $parent, is not on the first-parent history of $branch," \
         "so no push to $branch recorded it"
path="$directory/$parent.json"
echo "The baseline is the record of $parent, the first parent of $commit" >&2

# Each pass asks for the runs before it fetches the data branch. A run
# stores the record before it finishes, so a run which the answer gives as
# finished has stored whatever it was to store by the fetch.
deadline=$((SECONDS + seconds))
while :; do
  runs=$(gh run list --workflow "$workflow" --commit "$parent" --event push --limit 100 \
           --json databaseId,status,conclusion,url) \
    || die "gh could not list the runs of $workflow for $parent"
  unfinished=$(jq -r '.[] | select(.status != "completed") | .url' <<< "$runs") \
    || die "gh listed the runs of $workflow for $parent as other than JSON: $runs"
  status=0
  git ls-remote --exit-code --heads "$remote" "refs/heads/$data_branch" > /dev/null || status=$?
  case $status in
    0) git fetch --quiet --no-tags "$remote" "refs/heads/$data_branch" || die "git fetch $remote $data_branch failed"
       data_tip=$(git rev-parse --verify FETCH_HEAD) || die "git fetch $remote $data_branch left no FETCH_HEAD"
       # The tree alone says whether the record is held: a clone without
       # blobs must fetch the record's blob to show it, and a failure to
       # fetch it is not a record absent.
       if git rev-parse --verify --quiet "$data_tip:$path" > /dev/null; then
         git show "$data_tip:$path" || die "git show $data_tip:$path failed"
         exit 0
       fi;;
    2) ;;
    *) die "git ls-remote $remote failed with exit status $status";;
  esac
  if [ -z "$unfinished" ]; then
    finished=$(jq -r '.[] | "  \(.url): \(.conclusion)"' <<< "$runs")
    die "$data_branch holds no record of $parent at $path, and no run of $workflow for it is unfinished." \
        "Its runs: ${finished:-none}"
  fi
  if [ "$SECONDS" -ge "$deadline" ]; then
    die "$data_branch holds no record of $parent at $path after $seconds seconds, and a run which may" \
        "store it is unfinished: $unfinished. Re-run this job once it finishes."
  fi
  echo "Waiting for a record of $parent, which this run may store: $unfinished" >&2
  sleep "$poll_seconds"
done
