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
# A job is lost if it concluded `cancelled` or `failure` with no steps and no
# runner, and GitHub's annotation says so. A job that ran and failed is never
# re-run, so a real failure stays red, unless it needs a lost job.
#
# GitHub re-runs a job together with every job that needs it. The script asks
# for the lost job with the lowest id. In every run read, that job needed no
# other lost job: within an attempt, GitHub creates a job only after the jobs
# it needs, and a re-run attempt creates the job re-run before the jobs it
# copies. One request covers every loss seen so far.
#
# The script re-runs nothing if a newer run of the same workflow, branch and
# event exists. A re-run joins its workflow's concurrency group, and so could
# cancel that newer run.
#
# Once <attempt> reaches max_attempts, the script re-runs nothing, and warns.
# Whether GitHub starts this script's workflow again when an attempt it asked
# for finishes is not documented. If it does not, a person re-runs any loss the
# first request did not reach.
#
# The jobs are read through read_settled_jobs.sh, beside this script, which
# waits for every job to have a conclusion, since the jobs API can lag.
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

jobs=$(mktemp)
trap 'rm -f "$jobs"' EXIT
"$(dirname "$0")/read_settled_jobs.sh" "$run_id" "$jobs" '[.[] | select(.conclusion == null) | .name] | join(", ")'

candidates=$(jq -r '
  [ .[]
    | select((.conclusion == "cancelled" or .conclusion == "failure")
             and (.steps | length) == 0
             and (.runner_name // "") == "") ]
  | sort_by(.id)
  | .[]
  | "\(.id)\t\(.name | gsub("[\r\n]"; " "))"' "$jobs")

lost=
while IFS=$'\t' read -r job_id job_name; do
  [ -n "$job_id" ] || continue
  messages=$(gh api "repos/$GH_REPO/check-runs/$job_id/annotations" --jq '.[].message')
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
  echo "::warning::Run $run_id has had $attempt attempts, and lost a job to the runners in the last." \
       "No more re-runs are made."
  exit 0
fi

IFS=$'\t' read -r job_id job_name <<< "$lost"
gh api -X POST "repos/$GH_REPO/actions/jobs/$job_id/rerun" > /dev/null
echo "Re-running $job_name, and the jobs that need it."
