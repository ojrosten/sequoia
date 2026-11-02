#!/bin/bash
# Usage: GH_TOKEN=<token> GH_REPO=<owner>/<repo> job_log.sh <job id>
#
# Writes the log of the job <job id> to standard output, each line without the
# time stamp which GitHub puts before it. A line which a step wrote reads as
# the step wrote it. A line of a step's script, which GitHub lists before the
# step runs, starts with an escape sequence setting its colour, so it does not
# read as a line the step wrote. The script fails if the log cannot be read.
#
# The log is read with curl rather than `gh api`: gh 2.97.0 refuses to write
# a response holding escape sequences, as a log does, without
# --allow-escape-sequences, an option which the image's gh may predate.

set -euo pipefail

if [ $# -ne 1 ] || [ -z "$1" ] || [ -z "${GH_TOKEN:-}" ] || [ -z "${GH_REPO:-}" ]; then
  echo "Usage: GH_TOKEN=<token> GH_REPO=<owner>/<repo> $0 <job id>" >&2
  exit 2
fi

curl -fsSL -H "Authorization: Bearer $GH_TOKEN" -H 'Accept: application/vnd.github+json' \
  "${GITHUB_API_URL:-https://api.github.com}/repos/$GH_REPO/actions/jobs/$1/logs" \
  | sed -E 's/^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9:.]+Z //'
