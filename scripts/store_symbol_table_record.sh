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
# A record for a commit which the branch already holds is stored again only
# to add fields. Ignoring the field "run":
#   - if the two are the same, the script says so and succeeds: a run
#     repeated at one commit has nothing new to say;
#   - if the new record keeps every field of the held one, unchanged, and has
#     others besides, it replaces the held one, which it adds to: a backfill
#     measures a commit which was recorded before the measurement existed.
#     The new record's run replaces the held one's, since it made the fields
#     added;
#   - if the held record keeps every field of the new one, unchanged, and has
#     others besides, the script says so and succeeds: a repeated
#     measurement which failed has nothing to add;
#   - otherwise it prints the difference and fails, since a field which
#     changed is either a change of toolchain or a build which is not
#     deterministic, which a person should judge.
# Bar run, a field is never changed, nor removed.
#
# The exception is a measured count, which a rerun of one commit moves a
# little: prune's instruction count by about 2 parts in a million, the runner
# setup's system calls by 0.2%. Each is named in measured_fields below with
# its own tolerance, as the denominator of a relative difference. A count which
# differs from the held one by less than that is taken to be the held one. The
# script prints the difference, and the record is then compared, and stored,
# with the held count in place of the new: the first measurement stands. The
# stored record is jq's output, keys sorted. A larger difference is refused as
# any other, and so is a field added beneath an object already held, since
# only a whole field is added to a held record.
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

# The counts which a rerun may move slightly: each one's path into a record,
# and the denominator of the relative difference below which a count is taken
# to be the held one
measured_fields='[
  {"path": ["prune_dependency_analysis", "instructions"], "denominator": 10000},
  {"path": ["runner_setup", "instructions"],              "denominator": 200},
  {"path": ["runner_setup", "syscalls"],                  "denominator": 200}
]'

# within($held; $offered; $denominator), in jq: both are numbers, they differ,
# and they differ by less than $held over the denominator
within_tolerance='
  def within($held; $offered; $denominator):
    ($held | type) == "number" and ($offered | type) == "number"
    and $held != $offered
    and ((($offered - $held) | fabs) * $denominator < $held);'

# tolerated <held> <record>: a line for each measured field of <record> which
# differs from <held>'s by less than the tolerance
tolerated() {
  jq -r --argjson held "$1" --argjson fields "$measured_fields" \
     "$within_tolerance"'
     . as $offered | $fields[] | .path as $path | .denominator as $denominator
     | ($held | getpath($path)) as $h | ($offered | getpath($path)) as $o
     | select(within($h; $o; $denominator))
     | "\($path | join(".")): held \($h), offered \($o), tolerance 1 in \($denominator)"' "$2"
}

# settled <held> <record>: <record>, its keys sorted, with the held count in
# each measured field which differs from it by less than the tolerance
settled() {
  jq -S --argjson held "$1" --argjson fields "$measured_fields" \
     "$within_tolerance"'
     . as $offered | reduce $fields[] as $field (.; $field.path as $path
       | if within($held | getpath($path); $offered | getpath($path); $field.denominator)
         then setpath($path; $held | getpath($path)) else . end)' "$2"
}

# keeps_every_field <whole> <part>: <whole> has every field of <part>, with
# the same value, and so <part> is no larger; both are records as JSON
keeps_every_field() {
  jq -e --argjson part "$2" '. as $whole | $part | to_entries
                             | all(.key as $key | ($whole | has($key)) and .value == $whole[$key])' \
     <<< "$1" > /dev/null
}

# added_fields <held> <offered>: the names of the fields of <offered> which <held> lacks
added_fields() { jq -rn --argjson held "$1" --argjson offered "$2" '$offered | keys - ($held | keys) | join(", ")'; }

# store_once: 0 if the record is stored or already held, 1 if the push
# failed; any other failure exits. The function is called as a condition, so
# set -e does not reach into it, and each command's failure is handled here.
store_once() {
  local status=0 tip='' held offered blob tree new_tip close stored=$record
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
    close=$(tolerated "$held" "$record") || die "$record cannot be read as JSON"
    if [ -n "$close" ]; then
      echo "$record has measured counts within their tolerances of those of $path, which stand:"
      echo "$close"
      stored="$work/settled.json"
      settled "$held" "$record" > "$stored"
    fi
    offered=$(without_run "$stored") || exit 1
    if [ "$held" = "$offered" ]; then
      echo "$branch already holds $path, the same but for its run: $(jq -r '.run.url' "$held_file")"
      return 0
    elif keeps_every_field "$offered" "$held"; then
      echo "$branch holds $path, which $record adds to: $(added_fields "$held" "$offered")"
    elif keeps_every_field "$held" "$offered"; then
      echo "$branch already holds $path, with every field of $record and more: $(jq -r '.run.url' "$held_file")"
      return 0
    else
      echo "$branch already holds $path, and it differs from $record other than in its run, or in fields only one has:" >&2
      diff <(echo "$held") <(echo "$offered") >&2
      exit 1
    fi
  fi

  rm -f "$index"
  if [ -n "$tip" ]; then GIT_INDEX_FILE=$index git read-tree "$tip" || die "git read-tree $tip failed"; fi
  blob=$(git hash-object -w -- "$stored") || die "git hash-object $record failed"
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
