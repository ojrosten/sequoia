#!/bin/bash
# Usage:
#   GH_REPO=<owner>/<repo> \
#     dispatch_and_find_run.sh <workflow file> <ref> [<gh workflow run option>...]
#
# Dispatches <workflow file> on the branch <ref>, with the options after
# <ref>, and writes the id of the run the dispatch made to standard output.
# The options are those of `gh workflow run`, such as `-f <input>=<value>`.
#
# The dispatch returns nothing, and the run appears a few seconds later. The
# script takes the oldest dispatched run of <workflow file> on <ref> created
# no earlier than the second before the dispatch, so that a second dispatch
# made later is not taken in place of this one. A dispatch made in the same
# second, by another caller, can still be taken. The script looks every 10 s,
# twelve times, and fails with an error if no run has appeared.
#
# The caller confirms that the run found is the one it dispatched, by what
# the run did.

set -eu

if [ $# -lt 2 ] || [ -z "$1" ] || [ -z "$2" ] || [ -z "${GH_REPO:-}" ]; then
  echo "Usage: GH_REPO=<owner>/<repo> $0 <workflow file> <ref> [<gh workflow run option>...]" >&2
  exit 2
fi

workflow=$1 ref=$2
shift 2

before=$(date -u +%Y-%m-%dT%H:%M:%S+00:00)
gh workflow run "$workflow" --ref "$ref" "$@" >&2

id=""
for _ in $(seq 1 12); do
  sleep 10
  id=$(gh run list --workflow "$workflow" --branch "$ref" --event workflow_dispatch --created ">=$before" \
         --json databaseId --jq 'last.databaseId // empty')
  [ -n "$id" ] && break
done
[ -n "$id" ] || { echo "::error::the dispatched run of $workflow on $ref never appeared" >&2; exit 1; }
echo "dispatched: https://github.com/$GH_REPO/actions/runs/$id" >&2
echo "$id"
