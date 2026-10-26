#!/bin/bash
# Prints the leftover_materials_setup field of a symbol-table record, as JSON
# on standard output, from the counts of two runs of one test executable and
# the number of entries which the first run left:
#
#   leftover_materials_setup_record.sh <fresh counts> <seeded counts> <previous entries>
#
# The field holds the cost to the runner's setup of the materials which an
# earlier run left behind. The fresh run is made with no such materials. The
# seeded run is made next, so that it finds the materials which the fresh run
# left. Both runs are counted by runner_setup_record.sh, and the two files of
# counts are its output. The field is the instructions of the two runs, so that
# a reader subtracts the fresh run's from the seeded run's, and the number of
# entries which the seeded run found. The system calls are left out: callgrind
# counts them over the whole run and every thread, and the runner removes
# leftover materials on a thread of its own.
#
# The field holds:
#   - boundary, which both files of counts must give, since a pair counted
#     under two boundaries has no difference to read;
#   - previous_entries, the entries which the seeded run found;
#   - fresh and seeded, each with its instructions, its tests and the exit
#     status of its run.
#
# A record whose previous_entries is 0 is printed, not refused: it says that
# the seeded run found nothing to remove, so that its difference from the
# fresh run measures nothing. The dashboard's chart of the field is to show
# such a record as blind; no dashboard code reads the field yet.
#
# A file of counts which is empty or absent, as runner_setup_record.sh leaves
# it when it refuses a run, or which lacks a count, is refused: the script
# succeeds, says why in a ::warning:: on standard error, and prints nothing.
# So is a boundary on which the files differ, and a number of entries which is
# no number.

set -u

if [ $# -ne 3 ]; then
  echo "Usage: $0 <fresh counts> <seeded counts> <previous entries>" >&2
  exit 2
fi
fresh=$1 seeded=$2 previous=$3

refuse() {
  echo "::warning::$1, so the record has no leftover_materials_setup" >&2
  exit 0
}

# counted <file>: succeeds if the file holds one run's counts
counted() {
  jq -e '(.boundary | type == "string")
         and (.exit_status | type == "number" and . >= 0 and floor == .)
         and ([.tests, .instructions] | all(type == "number" and . > 0 and floor == .))' "$1" > /dev/null 2>&1
}

[[ $previous =~ ^(0|[1-9][0-9]*)$ ]] || refuse "the number of previous entries '$previous' is not a number"
counted "$fresh" || refuse "the fresh run's counts in '$fresh' are absent, or lack a boundary, an exit status, tests or instructions"
counted "$seeded" || refuse "the seeded run's counts in '$seeded' are absent, or lack a boundary, an exit status, tests or instructions"
[ "$(jq -r '.boundary' "$fresh")" = "$(jq -r '.boundary' "$seeded")" ] \
  || refuse "the two runs were counted under different boundaries"

jq -n --slurpfile fresh "$fresh" --slurpfile seeded "$seeded" --argjson previous "$previous" \
  'def run: {instructions, tests, exit_status};
   {boundary: $fresh[0].boundary, previous_entries: $previous, fresh: ($fresh[0] | run), seeded: ($seeded[0] | run)}'
