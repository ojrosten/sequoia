#!/bin/bash
# Prints the leftover_materials_setup field of a symbol-table record, as JSON
# on standard output, from the counts of two runs of one test executable and
# the number of entries which the first run left:
#
#   leftover_materials_setup_record.sh <counts without leftovers> <counts with leftovers> <leftover entries>
#
# The field holds the cost to the runner's setup of the materials which an
# earlier run left behind. The run without leftovers is made with no such
# materials. The run with leftovers is made next, so that it finds the
# materials which the first run left. Both runs are counted by
# runner_setup_record.sh, and the two files of counts are its output. The field
# is the instructions of the two runs, so that a reader subtracts the first
# run's from the second's, and the number of entries which the second run
# found.
#
# The field holds:
#   - boundary, which both files of counts must give, since a pair counted
#     under two boundaries has no difference to read;
#   - leftover_entries, the entries which the run with leftovers found;
#   - without_leftovers and with_leftovers, each with its instructions, its
#     tests and the exit status of its run. The two runs must have executed
#     the same number of tests, since a run which stopped early has fewer
#     instructions for that reason alone. The exit statuses may differ, and
#     are recorded.
#
# The system calls are left out. In a run with the regression this field is
# to detect, the system calls of the counted region were the same as in a
# run without it. The total of the system calls of the field this replaces
# stayed flat while its tests grew from 81 to 97.
#
# Two offsets are constant for fixed code, so they belong to the baseline of
# the difference and are not noise:
#   - leftover_entries includes the parent directories above each test's tree;
#   - the run without leftovers takes the in-place path, since renaming a
#     tree fails when there is none, while the run with leftovers renames the
#     tree and enqueues its removal.
#
# A record whose leftover_entries is 0 is printed, not refused: it says that
# the run with leftovers found nothing to remove, so that the difference
# measures nothing.
#
# A file of counts which is empty or absent, as runner_setup_record.sh leaves
# it when it refuses a run, or which lacks a count, is refused: the script
# succeeds, says why in a ::warning:: on standard error, and prints nothing.
# So is a pair on which the files differ in their boundary or in their tests,
# and a number of entries which is no number.

set -u

if [ $# -ne 3 ]; then
  echo "Usage: $0 <counts without leftovers> <counts with leftovers> <leftover entries>" >&2
  exit 2
fi
without_leftovers=$1 with_leftovers=$2 leftover_entries=$3

refuse() {
  echo "::warning::$1, so the record has no leftover_materials_setup" >&2
  exit 0
}

# has_counts <file>: succeeds if the file holds one run's counts
has_counts() {
  jq -e '(.boundary | type == "string")
         and (.exit_status | type == "number" and . >= 0 and floor == .)
         and ([.tests, .instructions] | all(type == "number" and . > 0 and floor == .))' "$1" > /dev/null 2>&1
}

# same_in <field>: succeeds if the two files give the field equally
same_in() {
  [ "$(jq -r ".$1" "$without_leftovers")" = "$(jq -r ".$1" "$with_leftovers")" ]
}

[[ $leftover_entries =~ ^(0|[1-9][0-9]*)$ ]] || refuse "the number of leftover entries '$leftover_entries' is not a number"
has_counts "$without_leftovers" || refuse "the counts in '$without_leftovers' are absent, or lack a boundary, an exit status, tests or instructions"
has_counts "$with_leftovers" || refuse "the counts in '$with_leftovers' are absent, or lack a boundary, an exit status, tests or instructions"
same_in boundary || refuse "the two runs were counted under different boundaries"
same_in tests || refuse "the two runs executed different numbers of tests"

jq -n --slurpfile without "$without_leftovers" --slurpfile with "$with_leftovers" --argjson entries "$leftover_entries" \
  'def counts_of_one_run: {instructions, tests, exit_status};
   {boundary: $without[0].boundary, leftover_entries: $entries,
    without_leftovers: ($without[0] | counts_of_one_run), with_leftovers: ($with[0] | counts_of_one_run)}'
