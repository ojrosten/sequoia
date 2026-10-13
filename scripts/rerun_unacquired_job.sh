#!/bin/bash
# Usage:
#   GH_REPO=<owner>/<repo> rerun_unacquired_job.sh <run id> <attempt>
#
# Re-runs a job of run <run id> that never got a runner, together with the
# jobs that need it. <attempt> is the run's attempt that has just finished.
#
# A hosted job can wait for a runner that never comes. GitHub gives up after
# about 15 minutes, annotating the job "The job was not acquired by Runner of
# type hosted even after multiple attempts". The jobs that need it then fail
# on its absence: a column without scope's answer fails at "Nothing to test",
# and an audit missing a column reports it absent. So the run's failure says
# nothing about the change. Run 38425439632 lost one macOS column this way, and
# run 37364856636 lost scope and every job after it.
#
# The script recognises such a job by what it lacks: it concluded `cancelled`
# or `failure`, with no steps and no runner. A job that ran and failed is never
# re-run, so a real failure stays red.
#
# GitHub re-runs a job together with every job that needs it. GitHub creates a
# job once the jobs it needs have finished, so in every run read, a job's id is
# higher than the ids of the jobs it needs. The script therefore re-runs the
# lost job with the lowest id, which no other lost job is needed by. One
# request covers every loss seen so far. A lost job the request does not reach
# is re-run when the new attempt finishes.
#
# Once <attempt> reaches max_attempts, the script re-runs nothing, and warns
# instead. So a shortage of runners that outlasts the retries ends red.
#
# <run id> and <attempt> are positive whole numbers with no leading zero.
# Arguments of any other form, and an unset or empty $GH_REPO, are refused with
# status 2, before anything is read.

set -euo pipefail

max_attempts=4

usage() {
  echo "Usage: GH_REPO=<owner>/<repo> $0 <run id> <attempt>" >&2
  exit 2
}

positive() { [[ $1 =~ ^[1-9][0-9]*$ ]]; }

[ $# -eq 2 ] && [ -n "${GH_REPO:-}" ] || usage
run_id=$1 attempt=$2
positive "$run_id" && positive "$attempt" || usage

jobs=$(gh api "repos/$GH_REPO/actions/runs/$run_id/jobs?filter=latest&per_page=100" --paginate --jq '.jobs[]' | jq -s .)

lost=$(jq -r '
  [ .[]
    | select((.conclusion == "cancelled" or .conclusion == "failure")
             and (.steps | length) == 0
             and (.runner_name // "") == "") ]
  | sort_by(.id)
  | .[]
  | "\(.id)\t\(.name)"' <<< "$jobs")

if [ -z "$lost" ]; then
  echo "No job of run $run_id lacked a runner."
  exit 0
fi

echo "Jobs of run $run_id that never got a runner:"
printf '%s\n' "$lost" | cut -f2 | sed 's/^/  /'

if [ "$attempt" -ge "$max_attempts" ]; then
  echo "::warning::Run $run_id has had $attempt attempts, and lost a job to the runners in the last. No more re-runs are made."
  exit 0
fi

IFS=$'\t' read -r job_id job_name <<< "$lost"
gh api -X POST "repos/$GH_REPO/actions/jobs/$job_id/rerun" > /dev/null
echo "Re-running $job_name, and the jobs that need it."
