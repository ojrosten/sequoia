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
# completion time, but not its annotations. So the annotations are read from a
# job's original. A job of the first attempt is its own original. The original
# of a job of a later attempt is the one job of its name and completion time in
# the earliest attempt that has such a job: an earlier attempt for a copy, and
# the job's own for any other job.
#
# A job of a later attempt is a copy if it completed before its attempt began.
# A copy that has no original of an earlier attempt, and a job that has several
# originals, fail the script with an error naming the job, and nothing is
# re-run. Those two rules held for each of the 1036 completed jobs, 380 of them
# copies, of the 56 runs of several attempts read on 2026-11-24. A job's
# creation time tells no copy: a job cancelled before it starts can complete a
# second before its creation, as 7 of the 656 other jobs did.
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
# The script re-runs nothing while another run of the same workflow and branch
# is live, whatever its event. A re-run joins its workflow's concurrency group,
# and so could cancel that run. The script cannot tell whether the two share a
# group, so a lost job stays lost beside a live run of another group, as a
# scheduled run of rigorous beside another. The script assumes that no group
# holds runs of several workflows or branches, and no group of sequoia's does.
# A run created more than 35 days ago, the longest that GitHub lets a run last,
# is not live, whatever its status. If the list of the workflow's and branch's
# runs does not hold the run to re-run, the script fails.
#
# The script looks for a live run before it reads the jobs, so that a run
# superseded by a newer push is left alone without the wait below. The script
# looks again just before it asks for a re-run, since a run may have begun in
# between. A run that begins after the second look can still be cancelled.
#
# Once <attempt> reaches max_attempts, the script re-runs nothing, and warns.
# The bound counts the run's attempts, whichever lost job each re-ran.
#
# The latest attempt's jobs are read through read_settled_jobs.sh, beside this
# script, which waits for every job to have a conclusion, since the jobs API
# can lag. The jobs of every attempt are read too, for the originals, with no
# wait. The attempts before the latest finished before it began, so their
# records have settled. A job of the latest, where that is not the first, has
# no original until its record there settles, so fails the script.
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

# exit_if_another_run_is_live [<prefix>]: ends the script if another run of the
# workflow and branch of $run is live, with a message that <prefix> opens.
exit_if_another_run_is_live() {
  local workflow_id branch earliest live_id
  if ! { workflow_id=$(jq -r .workflow_id <<< "$run") \
         && branch=$(jq -r .head_branch <<< "$run") \
         && earliest=$(jq -nr 'now - 35 * 24 * 3600 | todate') \
         && gh api -X GET "repos/$GH_REPO/actions/workflows/$workflow_id/runs" \
              -f branch="$branch" -f created=">=$earliest" -F per_page=100 --paginate > "$reads/runs.json" \
         && live_id=$(jq -rs --argjson run_id "$run_id" '
              map(.workflow_runs[])
              | if any(.id == $run_id) then . else error("run \($run_id) is not listed") end
              | map(select(.id != $run_id and .status != "completed").id)
              | first // empty' "$reads/runs.json"); }; then
    echo "::error::No list of the runs of run $run_id's workflow and branch that holds run $run_id could be read." \
         "Nothing is re-run."
    exit 1
  fi
  if [ -n "$live_id" ]; then
    echo "${1:-}Run $live_id, of the same workflow and branch, is live. Run $run_id is not re-run."
    exit 0
  fi
}

[ $# -eq 2 ] && [ -n "${GH_REPO:-}" ] || usage
run_id=$1 attempt=$2
positive "$run_id" && positive "$attempt" || usage

run=$(gh api "repos/$GH_REPO/actions/runs/$run_id")
reads=$(mktemp -d)
trap 'rm -rf "$reads"' EXIT
exit_if_another_run_is_live

jobs=$reads/jobs.json every_attempt_jobs=$reads/every_attempt_jobs.ndjson
"$(dirname "$0")/read_settled_jobs.sh" "$run_id" "$jobs" '[.[] | select(.conclusion == null) | .name] | join(", ")'
gh api "repos/$GH_REPO/actions/runs/$run_id/jobs?filter=all&per_page=100" --paginate --jq '.jobs[]' \
  > "$every_attempt_jobs"

attempt_began=$(jq -r .run_started_at <<< "$run")
# A candidate's line holds the id of its original, or null where it has no
# single original.
candidates=$(jq -r --slurpfile every_attempt_jobs "$every_attempt_jobs" --arg attempt_began "$attempt_began" '
  [ .[]
    | select((.conclusion == "cancelled" or .conclusion == "failure")
             and (.steps | length) == 0
             and (.runner_name // "") == "")
    | . as $job
    | [ $every_attempt_jobs[]
        | select(.name == $job.name
                 and .completed_at == $job.completed_at) ]
    | (map(.run_attempt) | min) as $earliest
    | map(select(.run_attempt == $earliest)) as $originals
    | $job
    | .original_id = (if .run_attempt == 1
                      then .id
                      elif ($originals | length) == 1
                           and ($earliest < .run_attempt or .completed_at >= $attempt_began)
                      then $originals[0].id
                      else null end) ]
  | sort_by(.original_id)
  | .[]
  | "\(.original_id)\t\(.id)\t\(.name | gsub("[\r\n]"; " "))"' "$jobs")

lost=
while IFS=$'\t' read -r original_id job_id job_name; do
  [ -n "$job_id" ] || continue
  if [ "$original_id" = null ]; then
    echo "::error::Job $job_name of run $run_id has no single original among the jobs of the run's attempts." \
         "Nothing is re-run."
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
exit_if_another_run_is_live ::notice::
gh api -X POST "repos/$GH_REPO/actions/jobs/$job_id/rerun" > /dev/null
echo "Re-running $job_name, and the jobs that need it."
