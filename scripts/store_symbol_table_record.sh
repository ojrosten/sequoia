#!/bin/bash
# Stores a record of symbol_table_report.py on a data branch, one file per
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
# A record for a commit which the branch already holds is merged with the
# held one, at every depth, ignoring the field "run". The run holds what is
# particular to one measurement: the id, attempt and URL of the job, and the
# model name of its runner's CPU. Another job measuring the commit differs in
# each of these, so the merge compares none of them. The merge is the union of
# the rest:
#   - a field only the held record has is kept: a measurement since dropped
#     from the records does not lose its history;
#   - a field only the offered record has is added: a backfill measures a
#     commit which was recorded before the measurement existed. This holds
#     within an object both have, so a new sub-field of a held object is
#     added;
#   - a field both have must be equal, bar a measured count (below) within
#     its tolerance, where the held value stands;
#   - otherwise the script prints the fields which differ and fails, since a
#     field which changed is either a change of toolchain or a build which is
#     not deterministic, which a person should judge.
# A field is never changed, nor removed. If the offered record adds nothing, the
# script says so and succeeds, and the branch is left as it was: a run repeated
# at one commit has nothing new to say. If it adds fields, the union is stored
# as jq's output with the keys sorted. It carries the offered record's run,
# since that run made the added fields.
#
# A rerun of one commit moves the measured counts a little. So a measured
# count within 1 in `denominator` of the held one, as a fraction of the held
# count, is taken to be the held one: the held value stands, and the script
# prints the difference. Each count is named in measured_fields below with a
# denominator of its own: 1 in 1,000 for prune's, 1 in 50 for the setup's
# instructions and 1 in 200 for its system calls.
#
# The spread measured, with the performance tests excluded from the setup's
# runs: within one job, prune's analysis moved by 2 and by 49 parts in a
# million on two pairs of reruns; the setup's instructions by 0.092% and its
# system calls by 0.014%. Between separate jobs measuring one commit, from
# the 80th pull request on, prune's analysis moved by up to 722 parts in a
# million, and the setup's instructions per test by up to 1.04% between
# commits whose code is identical. The tolerances above are wider than those
# spreads, so that an honest re-measurement is not refused.
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

# The counts which a rerun of one commit may move: each one's path into a
# record, and the denominator of the relative difference below which a count is
# taken to be the held one
measured_fields='[
  {"path": ["prune_dependency_analysis", "instructions"], "denominator": 1000},
  {"path": ["runner_setup", "instructions"],              "denominator": 50},
  {"path": ["runner_setup", "syscalls"],                  "denominator": 200}
]'

# within($held; $offered; $denominator), in jq: both are numbers, they differ,
# and they differ by less than $held over the denominator
within_tolerance='
  def within($held; $offered; $denominator):
    ($held | type) == "number" and ($offered | type) == "number"
    and $held != $offered
    and ((($offered - $held) | fabs) * $denominator < $held);'

# merge <held> <offered>: the union of the two records, as JSON on standard
# output, by the rule in the header: {value, conflicts, tolerated, added, kept}.
# The paths in conflicts, added and kept are arrays of keys.
merge() {
  jq -n -S --argjson held "$1" --argjson offered "$2" --argjson fields "$measured_fields" \
     "$within_tolerance"'
     def tolerance($path): $fields | map(select(.path == $path)) | first | .denominator;
     def none: {conflicts: [], tolerated: [], added: [], kept: []};
     def merge($h; $o; $path):
       tolerance($path) as $denominator | if ($h | type) == "object" and ($o | type) == "object" then
         reduce (($h | keys) + ($o | keys) | unique)[] as $key ({value: {}} + none;
           if ($h | has($key)) and ($o | has($key)) then
             merge($h[$key]; $o[$key]; $path + [$key]) as $r
             | .value[$key] = $r.value | .conflicts += $r.conflicts | .tolerated += $r.tolerated
             | .added += $r.added | .kept += $r.kept
           elif $h | has($key) then .value[$key] = $h[$key] | .kept += [$path + [$key]]
           else .value[$key] = $o[$key] | .added += [$path + [$key]] end)
       elif $h == $o then {value: $h} + none
       elif $denominator != null and within($h; $o; $denominator) then
         {value: $h} + none + {tolerated: [{path: $path, held: $h, offered: $o, denominator: $denominator}]}
       else {value: $h} + none + {conflicts: [$path]} end;
     merge($held; $offered; [])'
}

# store_once: 0 if the record is stored or already held, 1 if the push
# failed; any other failure exits. The function is called as a condition, so
# set -e does not reach into it, and each command's failure is handled here.
store_once() {
  local status=0 tip='' held offered merged blob tree new_tip stored=$record
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
    merged=$(merge "$held" "$offered") || die "merging $record with $path failed"
    if [ "$(jq '.conflicts | length' <<< "$merged")" -ne 0 ]; then
      echo "$branch already holds $path, and it differs from $record other than in its run, in: $(jq -r '[.conflicts[] | join(".")] | join(", ")' <<< "$merged")" >&2
      diff <(echo "$held") <(echo "$offered") >&2
      exit 1
    fi
    if [ "$(jq '.tolerated | length' <<< "$merged")" -ne 0 ]; then
      echo "$record has measured counts within their tolerances of those of $path, which stand:"
      jq -r '.tolerated[] | "\(.path | join(".")): held \(.held), offered \(.offered), tolerance 1 in \(.denominator)"' <<< "$merged"
    fi
    if [ "$(jq '.added | length' <<< "$merged")" -eq 0 ]; then
      if [ "$(jq '.kept | length' <<< "$merged")" -eq 0 ]; then
        echo "$branch already holds $path, the same but for its run: $(jq -r '.run.url' "$held_file")"
      else
        echo "$branch already holds $path, with every field of $record and more: $(jq -r '.run.url' "$held_file")"
      fi
      return 0
    fi
    echo "$branch holds $path, which $record adds to: $(jq -r '[.added[] | join(".")] | join(", ")' <<< "$merged")"
    stored="$work/union.json"
    jq -S --argjson offered "$(cat "$record")" '.value + ($offered | if has("run") then {run} else {} end)' <<< "$merged" > "$stored"
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
