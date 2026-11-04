#!/bin/bash
# Usage:
#   GH_REPO=<owner>/<repo> rerun_unacquired_job.sh <run id> <attempt>
#
# Re-runs a job of run <run id> that never got a runner, together with the
# jobs that need it. <attempt> is the run's attempt that has just finished.
#
# A hosted job can wait for a runner that never comes. GitHub gives up after
# about 15 minutes, and annotates the job, either "The job was not acquired by
# Runner of type hosted even after multiple attempts" or "The job was not
# started because it repeatedly failed to be acquired (5 attempts)". The run
# then fails or is cancelled, though the change is not at fault: the jobs that
# need the lost job fail on its absence, or are lost too. Run 38425439632 lost
# one macOS column this way, and run 37364856636 lost scope and every job after
# it.
#
# A re-run attempt holds a copy of each job of the previous attempt that it
# does not re-run. A copy keeps the job's conclusion, steps, runner and
# completion time, but has no annotations. A copy's completion time precedes
# its creation time, and no other job's does. That held for each of the 107
# jobs read on 2026-11-04 from runs 37364856636, 37365728782, 38425439632 and
# 39478109862, and from the first three attempts of run 41874199305. A job's
# original is the one job of any attempt that is not a copy, and has the
# job's name and completion time: a copy's original ran in an earlier
# attempt, and any other job is its own original. If a job's name and
# completion time match no such job, or several, the script fails with an
# error naming the job, and re-runs nothing.
#
# A job is lost if it concluded `cancelled` or `failure` with no steps and no
# runner, and GitHub's annotation on its original says so. A job that ran and
# failed is never re-run, so a real failure stays red, unless it needs a lost
# job.
#
# GitHub's API re-runs a run's jobs in one of three ways: one job with the
# jobs that need it, every failed job with the jobs that need them, or every
# job. The second and third would re-run each job that ran and failed, and
# the jobs API does not say which jobs a job needs, so the script cannot tell
# whether such a job needs a lost one. Each request starts an attempt, and
# whether GitHub accepts a second request while that attempt runs is not
# known; so the script asks for one lost job per attempt. Each lost job that
# the new attempt does not re-run is copied into it, still lost. GitHub starts
# this script's workflow again when an attempt the script asked for finishes,
# and the script then asks for the next lost job. Run 41874199305 lost two
# jobs in attempt 1, neither of which needs the other; attempt 2 re-ran one,
# and copied the other. Run 39478109862 lost its macOS column in attempts 1
# and 2, and the script asked for attempts 2 and 3.
#
# The script asks for the lost job whose original has the lowest id. That job
# needs no other lost job, since a job's original has a higher id than the
# original of each job it needs:
#   - When GitHub re-runs a job, it re-runs each job that needs that job. So a
#     job's original is of the same attempt as the original of each job it
#     needs, or of a later one.
#   - Within an attempt, GitHub creates each job that it runs only after the
#     jobs it needs. Each attempt's ids exceed the previous attempt's.
#
# The script re-runs nothing if a newer run of the same workflow, branch and
# event exists. A re-run joins its workflow's concurrency group, and so could
# cancel that newer run.
#
# Once <attempt> reaches max_attempts, the script re-runs nothing, and warns.
# The bound counts the run's attempts, whichever lost job each re-ran.
#
# The latest attempt's jobs are read through read_settled_jobs.sh, beside this
# script, which waits for every job to have a conclusion, since the jobs API
# can lag. The jobs of every attempt are read too, for the originals, with no
# wait: the attempts before the latest finished before it began, and a job of
# the latest whose record has not settled matches no original, so fails the
# script.
#
# <run id> and <attempt> are positive whole numbers with no leading zero.
# Arguments of any other form, and an unset or empty $GH_REPO, are refused with
# status 2, before anything is read.

set -eu

max_attempts=4

usage() {
  echo "Usage: GH_REPO=<owner>/<repo> $0 <run id> <attempt>" >&2
  exit 2
}

positive() { [[ $1 =~ ^[1-9][0-9]*$ ]]; }

[ $# -eq 2 ] && [ -n "${GH_REPO:-}" ] || usage
run_id=$1 attempt=$2
positive "$run_id" && positive "$attempt" || usage

run=$(gh api "repos/$GH_REPO/actions/runs/$run_id")
workflow_id=$(jq -r .workflow_id <<< "$run")
branch=$(jq -r .head_branch <<< "$run")
event=$(jq -r .event <<< "$run")
newest=$(gh api -X GET "repos/$GH_REPO/actions/workflows/$workflow_id/runs" \
           -f branch="$branch" -f event="$event" -F per_page=1 --jq '.workflow_runs[0].id')
if [ "$newest" != "$run_id" ]; then
  echo "Run $newest has superseded run $run_id, which is not re-run."
  exit 0
fi

reads=$(mktemp -d)
trap 'rm -rf "$reads"' EXIT
jobs=$reads/jobs.json every_attempt_jobs=$reads/every_attempt_jobs.ndjson
"$(dirname "$0")/read_settled_jobs.sh" "$run_id" "$jobs" '[.[] | select(.conclusion == null) | .name] | join(", ")'
gh api "repos/$GH_REPO/actions/runs/$run_id/jobs?filter=all&per_page=100" --paginate --jq '.jobs[]' \
  > "$every_attempt_jobs"

# A candidate's line holds the id of its original, or null where it matches no
# original, or several.
candidates=$(jq -r --slurpfile every_attempt_jobs "$every_attempt_jobs" '
  def copied: .completed_at < .created_at;
  [ .[]
    | select((.conclusion == "cancelled" or .conclusion == "failure")
             and (.steps | length) == 0
             and (.runner_name // "") == "")
    | . as $job
    | .original_id = ([ $every_attempt_jobs[]
                        | select(.name == $job.name
                                 and .completed_at == $job.completed_at
                                 and (copied | not))
                        | .id ]
                      | if length == 1 then .[0] else null end) ]
  | sort_by(.original_id)
  | .[]
  | "\(.original_id)\t\(.id)\t\(.name | gsub("[\r\n]"; " "))"' "$jobs")

lost=
while IFS=$'\t' read -r original_id job_id job_name; do
  [ -n "$job_id" ] || continue
  if [ "$original_id" = null ]; then
    echo "::error::Job $job_name of run $run_id has no single original: its name and completion time match" \
         "no job that is not a copy, or several. Nothing is re-run."
    exit 1
  fi
  messages=$(gh api "repos/$GH_REPO/check-runs/$original_id/annotations" --jq '.[].message')
  if grep -qE 'not acquired by Runner|failed to be acquired' <<< "$messages"; then
    lost+=$job_id$'\t'$job_name$'\n'
  fi
done <<< "$candidates"

if [ -z "$lost" ]; then
  echo "No job of run $run_id lacked a runner."
  exit 0
fi

echo "Jobs of run $run_id that never got a runner:"
printf '%s' "$lost" | cut -f2 | sed 's/^/  /'

if [ "$attempt" -ge "$max_attempts" ]; then
  echo "::warning::Run $run_id has had $attempt attempts, and the last holds a job that never got a runner." \
       "No more re-runs are made."
  exit 0
fi

IFS=$'\t' read -r job_id job_name <<< "$lost"
gh api -X POST "repos/$GH_REPO/actions/jobs/$job_id/rerun" > /dev/null
echo "Re-running $job_name, and the jobs that need it."
