#!/bin/bash
# Usage:
#   GH_REPO=<owner>/<repo> \
#     read_settled_jobs.sh <run id> <jobs file> <jq filter> [<jq option>...]
#
# Reads the jobs of run <run id> until the jobs API has given every conclusion
# that <jq filter> awaits. The script writes each successful read to
# <jobs file>, as one JSON array. The filter reads that array and prints what
# it still awaits, or nothing once it awaits nothing. The options after the
# filter are passed to jq with it.
#
# Between reads, the script logs what the filter awaits, or that the read
# failed, and pauses 10 s. Reading also stops after the first read to end at
# least 420 s after the script began reading. If the time runs out, a warning
# names what the filter still awaits, or says that the last read failed. The
# script then succeeds, and <jobs file> holds the last successful read, for the
# caller's checks to judge. If no read succeeds, the script fails with an error
# instead. A caller's job needs a timeout longer than the wait.
#
# The wait exists because the jobs API can lag behind the run it describes. In
# run 36726395452, the API was read at 14:20:01:
#   - The Linux column's job had completed at 14:19:55. It was reported as
#     `in_progress`.
#   - The Windows column's job had completed at 14:18:30, and its
#     `Run the suite` step at 14:18:17. The job was reported as `success`, but
#     the step had no conclusion.
# A read at 14:24:36 gave both `Run the suite` steps a conclusion. The same
# read still reported the Linux column's Build step as `in_progress`, 4 min
# 41 s after the job had completed. A read at 14:37:32 gave every step a
# conclusion. The bound of 420 s is longer than any lag measured. A filter
# should await only what its caller judges, since other steps can lag for
# longer. The wait is unnecessary if the API comes to reflect a finished job
# at once. The callers' logs would then show no `reading again` line.

set -eu

if [ $# -lt 3 ] || [ -z "${GH_REPO:-}" ]; then
  echo "Usage: GH_REPO=<owner>/<repo> $0 <run id> <jobs file> <jq filter> [<jq option>...]" >&2
  exit 2
fi

run_id=$1 jobs_file=$2 filter=$3
shift 3

reads=$(mktemp -d)
trap 'rm -rf "$reads"' EXIT

wait_limit=420 interval=10
deadline=$((SECONDS + wait_limit))
read_succeeded=false
while :; do
  if gh api "repos/$GH_REPO/actions/runs/$run_id/jobs?per_page=100" --paginate --jq '.jobs[]' > "$reads/jobs.ndjson" \
     && jq -s . "$reads/jobs.ndjson" > "$reads/jobs.json"; then
    mv "$reads/jobs.json" "$jobs_file"
    read_succeeded=true
    awaited=$(jq -r "$@" "$filter" "$jobs_file")
    [ -n "$awaited" ] || break
    lacking="The jobs API has yet to give a conclusion awaited for $awaited"
  else
    lacking="The jobs API could not be read"
  fi
  if [ "$SECONDS" -ge "$deadline" ]; then
    if ! $read_succeeded; then
      echo "::error::No read of the jobs API succeeded, and the wait's ${wait_limit} s have run out"
      exit 1
    fi
    echo "::warning::$lacking, and the wait's ${wait_limit} s have run out." \
         "$jobs_file holds the last successful read."
    break
  fi
  echo "$lacking; reading again in ${interval}s"
  sleep "$interval"
done
