#!/bin/bash
# Store a record of symbol_table_report.py on a data branch, one file per
# commit.
#
#   store_symbol_table_record.sh <remote> <branch> <directory> <record>
#
# The record is stored as <directory>/<commit>.json on <branch> of <remote>,
# where <commit> is the record's own. Each record is a file of its own, so
# two runs storing records for different commits never touch the same file,
# and a record made later for an earlier commit - by a backfill - is stored
# beside the others. The script runs in a clone of the remote, whose objects
# it adds to, and leaves the clone's branches, index and working tree as
# they were.
#
# A record for a commit which the branch already holds is not stored again.
# If the two are the same but for their runs - the field "run" - the script
# says so and succeeds: a run repeated at one commit has nothing new to say.
# If they differ elsewhere, it prints the difference and fails, since the
# branch holds a record made first, and a second record which disagrees is
# either a change of toolchain or a build which is not deterministic, which
# a person should judge. Nothing is ever overwritten.
#
# A branch which does not exist is created, with no history. The push is
# never forced. When it fails - as it does when another run pushed first -
# the script fetches the branch again and builds its commit afresh on the
# new tip, a bounded number of times.
#
# The script refuses a record which cannot be read, or which names no commit
# of the clone by its full name. It also refuses a record whose tree was not
# clean, since the record's counts are then not the commit's.

set -euo pipefail

die() { echo "$*" >&2; exit 1; }

if [ $# -ne 4 ]; then
  echo "usage: $0 <remote> <branch> <directory> <record>" >&2; exit 2
fi
remote=$1 branch=$2 directory=$3 record=$4
attempts=5

commit=$(jq -r '.commit' "$record") || die "$record cannot be read as JSON"
# The script stores the record under its commit as given, so the commit must
# be the full name of a commit in the clone. rev-parse fails unless the name
# resolves to a commit, and otherwise prints the commit's full name, so the
# check refuses a branch's name or an abbreviated one.
full_name=$(git rev-parse --verify --quiet "$commit^{commit}") && [ "$full_name" = "$commit" ] \
  || die "$record names no commit of this clone by its full name: its commit is '$commit'"
[ "$(jq '.clean' "$record")" = true ] \
  || die "$record was made from a tree which was not clean, so its counts are not $commit's"
path="$directory/$commit.json"

work=$(mktemp -d); trap 'rm -rf "$work"' EXIT
index="$work/index" held_file="$work/held"

# without_run <file>: the record in <file>, its keys sorted, without its run
without_run() { jq -S 'del(.run)' "$1" || die "$1 cannot be read as JSON"; }

# store_once: 0 if the record is stored or already held, 1 if the push
# failed; any other failure exits. The function is called as a condition, so
# set -e does not reach into it, and each command's failure is handled here.
store_once() {
  local status=0 tip='' held offered blob tree new_tip
  git ls-remote --exit-code --heads "$remote" "refs/heads/$branch" > /dev/null || status=$?
  case $status in
    0) git fetch --quiet --no-tags "$remote" "refs/heads/$branch" || die "git fetch $remote $branch failed"
       tip=$(git rev-parse --verify FETCH_HEAD) || die "git fetch $remote $branch left no FETCH_HEAD";;
    2) echo "$remote has no branch $branch: it is created";;
    *) die "git ls-remote $remote failed with exit status $status";;
  esac

  # The tree alone says whether the record is held: a clone without blobs
  # must fetch the record's blob to show it, and a failure to fetch it is
  # not a record absent.
  if [ -n "$tip" ] && git rev-parse --verify --quiet "$tip:$path" > /dev/null; then
    git show "$tip:$path" > "$held_file" || die "git show $tip:$path failed"
    held=$(without_run "$held_file") || exit 1
    offered=$(without_run "$record") || exit 1
    if [ "$held" = "$offered" ]; then
      echo "$branch already holds $path, the same but for its run: $(jq -r '.run.url' "$held_file")"
      return 0
    fi
    echo "$branch already holds $path, and it differs from $record other than in its run:" >&2
    diff <(echo "$held") <(echo "$offered") >&2
    exit 1
  fi

  rm -f "$index"
  if [ -n "$tip" ]; then GIT_INDEX_FILE=$index git read-tree "$tip" || die "git read-tree $tip failed"; fi
  blob=$(git hash-object -w -- "$record") || die "git hash-object $record failed"
  GIT_INDEX_FILE=$index git update-index --add --cacheinfo "100644,$blob,$path" || die "git update-index failed"
  tree=$(GIT_INDEX_FILE=$index git write-tree) || die "git write-tree failed"
  new_tip=$(git -c user.name="sequoia CI" -c user.email=ci@example.invalid commit-tree "$tree" ${tip:+-p "$tip"} \
          -m "$path" -m "Recorded by $(jq -r '.run.url // "no run"' "$record")") || die "git commit-tree failed"
  if git push --quiet "$remote" "$new_tip:refs/heads/$branch"; then
    echo "Stored $path on $branch as $new_tip"
    return 0
  fi
  return 1
}

for ((attempt = 1; attempt <= attempts; ++attempt)); do
  if store_once; then exit 0; fi
  echo "Attempt $attempt of $attempts: the push to $branch failed; fetching it again"
done
die "The push to $branch failed $attempts times"
