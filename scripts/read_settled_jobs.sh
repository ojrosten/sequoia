#!/bin/bash
# Usage: read_settled_jobs.sh <run id> <jobs file> <jq filter> [<jq option>...]
#
# Reads the jobs of run <run id> until the jobs API has given every conclusion that
# <jq filter> awaits, and writes the last successful read to <jobs file>, as one JSON
# array. The filter reads that array and prints what it still awaits, or nothing once it
# awaits nothing. The options after the filter are passed to jq with it. $GH_REPO names
# the repository.
#
# The jobs are read with 10 s between reads, for up to 420 s. Each read that waits logs
# what the filter awaits. A read that fails is retried. If the time runs out, a warning
# names what the filter still awaits, or says that the last read failed. The script then
# still succeeds, leaving <jobs file> for the caller's checks to judge. If no read succeeds, the script fails with an error. A caller's job
# needs a timeout long enough for the wait.
#
# The wait exists because the jobs API can lag behind the run it describes. In run
# 36726395452, the API was read at 14:20:01:
#   - The Linux column's job had completed at 14:19:55. It was reported as `in_progress`.
#   - The Windows column's job had completed at 14:18:30, and its `Run the suite` step at
#     14:18:17. The job was reported as `success`, but the step had no conclusion.
# A read at 14:24:36 gave both `Run the suite` steps a conclusion. The same read still
# reported the Linux column's Build step as `in_progress`, 4 min 41 s after the job had
# completed. A read at 14:37:32 gave every step a conclusion. The bound of 420 s is longer
# than any lag measured. The filter should await only what the caller judges, since other
# steps can lag for longer. The wait is unnecessary if the logged lines stop appearing, or
# if the API comes to reflect a finished job at once.

set -eu

if [ $# -lt 3 ]; then
  echo "Usage: $0 <run id> <jobs file> <jq filter> [<jq option>...]" >&2
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
  [ "$SECONDS" -lt "$deadline" ] || { echo "::warning::$lacking, ${wait_limit}s after the wait began. The checks judge the last successful read."; break; }
  echo "$lacking; reading again in ${interval}s"
  sleep "$interval"
done
$read_succeeded || { echo "::error::No read of the jobs API succeeded"; exit 1; }
