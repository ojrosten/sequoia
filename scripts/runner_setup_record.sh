#!/bin/bash
# The runner_setup field of a symbol-table record, as JSON on standard output,
# from a callgrind run of a test executable and the run's execution records:
#
#   runner_setup_record.sh <callgrind output> <execution records> <exit status> <boundary>
#
# The callgrind run is of an executable whose collection was toggled on by
# test_to_run::execute and off by execution_timer::time_execution, so that its
# totals are the runner's work for each test, outside the test. The totals are
# the events Ir (instructions) and sysCount (system calls): the file's
# "events:" line names the columns of its "summary:" and "totals:" lines, which
# must both be present, and agree on Ir. The system calls are those of the
# "totals:" line.
#
# This reading of callgrind's two lines is inferred from runs 40186679215 and
# 40186689490 of symbol-table.yml, not taken from callgrind's documentation.
# With --collect-systime=yes the lines agreed on Ir (311650326) and differed on
# sysCount (summary 1719515, totals 833201) and on sysTime. The totals are
# taken to count the collected regions and the summary every system call of the
# run, so only Ir is compared.
#
# The tests are counted from the runner's own execution records: the
# executable's directory under output/ExecutionRecords holds a file for each
# test of the run, which the run fills, and none for a nested runner's tests
# (the runner's own tests run runners of their own, in a sandbox). Callgrind's
# call counts would count those nested calls of test_to_run::execute, whose
# setup its toggling does not collect, since it toggles only on the call at
# recursion depth 0 and on its return. A test which has not run has an empty
# file, so the tests are the non-empty files. The caller removes the directory
# before the run, so that no record is of an earlier run.
#
# The exit status is that of the run, as the executable's main gives it:
# 0 when the run passed, else 80 plus the flags of the run's return_code
# (versioned output moved, soft failures, a test threw, an incomplete run,
# failures after the run); to_exit_code in ReturnCodes.hpp. A pass is exactly
# status 0.
#
# A count which is zero or missing is refused: the script succeeds, says why
# in a ::warning:: on standard error, and prints nothing.

set -u

if [ $# -ne 4 ]; then
  echo "Usage: $0 <callgrind output> <execution records> <exit status> <boundary>" >&2
  exit 2
fi
out=$1 records=$2 status=$3 boundary=$4

refuse() {
  echo "::warning::$1, so the record has no runner_setup" >&2
  exit 0
}

[[ $status =~ ^(0|[1-9][0-9]*)$ ]] || refuse "the exit status '$status' is not a number"
[ -s "$out" ] || refuse "callgrind left no output"

instructions='' syscalls=''
read -r instructions syscalls < <(awk '
  $1 == "events:" { for (i = 2; i <= NF; ++i) col[$i] = i - 1 }
  $1 == "summary:" || $1 == "totals:" { line[$1] = $0; sub(/^[a-z]+:[ \t]*/, "", line[$1]) }
  END { if (!("totals:" in line) || !("summary:" in line) || !("Ir" in col) || !("sysCount" in col)) exit
        split(line["summary:"], s, " ")
        split(line["totals:"], f, " ")
        if (s[col["Ir"]] != f[col["Ir"]]) exit
        print f[col["Ir"]] + 0, f[col["sysCount"]] + 0 }' "$out")

tests=$(find "$records" -type f -name '*.txt' -size +0 2> /dev/null | wc -l | tr -d ' ')

for count in "$instructions" "$syscalls" "$tests"; do
  case "$count" in
    ''|*[!0-9]*|0)
      refuse "callgrind counted instructions '${instructions}' and system calls '${syscalls}' (summary and totals must agree on instructions), and the records give '${tests}' tests, and a zero or missing count is refused";;
  esac
done

jq -n --argjson instructions "$instructions" --argjson syscalls "$syscalls" --argjson tests "$tests" \
      --argjson exit_status "$status" --arg boundary "$boundary" \
  '{boundary: $boundary, exit_status: $exit_status, tests: $tests, instructions: $instructions, syscalls: $syscalls}'
