#!/bin/bash
# Controls for runner_setup_record.sh, against a callgrind output and execution
# records built here, and a mutation check of the controls.
#
#   runner_setup_record_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the script and against each mutant of the script.
# The script must fail no control, and each mutant at least one.
#
# The fixture is a callgrind output whose events are Ir, sysCount and sysTime,
# with 1000 instructions and 7 system calls in its summary and its totals, and
# three arcs into test_to_run::execute: one from the runner's loop, and two
# from a nested runner, as the runner's own tests make. The records hold three
# files of tests which ran, one empty file of a test which did not, and the
# run's stamp. Each control is a claim the script exists to keep:
#   - the field holds the instructions and the system calls, read by the names
#     of the events, whatever their order, and the exit status and boundary
#     given;
#   - the tests are the records of the tests which ran: the three arcs into
#     execute do not make three more, so a nested call does not inflate them;
#   - the exit status of a pass is 0, and a nonzero status is recorded, not
#     refused, since the dashboard leaves its point out;
#   - a count which is zero or missing, a summary which differs from its totals,
#     no output, no records and an exit status which is no number are each
#     refused, with a ::warning:: on standard error, nothing on standard
#     output, and success;
#   - arguments of any other number are refused with status 2.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script=$here/../runner_setup_record.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

case "$*" in
  ''|--mutations) ;;
  *) echo "Usage: $0 [--mutations]" >&2; exit 2;;
esac

boundary='B'

# callgrind <events> <summary> <totals>: an output file's text, with three arcs
# into execute (a top-level call, then two of a nested runner), one into
# execute_and_record, and the totals as given.
callgrind() {
  printf '%s\n' "events: $1" "summary: $2" 'fn=(1) main' '1 5' \
    'cfn=(2) sequoia::testing::test_to_run::execute(std::optional<unsigned long>, sequoia::testing::discarded_materials_remover&)' \
    'calls=1 0x1' '1 9' \
    'cfn=(2)' 'calls=2 0x1' '1 9' \
    'cfn=(3) sequoia::testing::test_to_run::execute_and_record(a)' 'calls=9 0x1' '1 9' \
    "totals: $3"
}

records=$tmp/records
mkdir -p "$records/Tests/A" "$records/Tests/B"
printf '%s\n' 'started x' 'finished yes' > "$records/Tests/A/one.txt"
printf '%s\n' 'started x' 'finished yes' > "$records/Tests/A/two.txt"
printf '%s\n' 'started x' 'finished yes' > "$records/Tests/B/three.txt"
: > "$records/Tests/B/skipped.txt"
echo stamp > "$records/run.stamp"
mkdir "$tmp/none"

callgrind 'Ir sysCount sysTime' '1000 7 55' '1000 7 55' > "$tmp/ok.out"
callgrind 'sysTime sysCount Ir' '55 7 1000' '55 7 1000' > "$tmp/reordered.out"
callgrind 'Ir sysCount sysTime' '1000 7 55' '1000 8 55' > "$tmp/differs.out"
callgrind 'Ir sysCount sysTime' '1000 7 55' '' | grep -v '^totals:' > "$tmp/no_totals.out"
callgrind 'Ir sysCount sysTime' '0 7 0' '0 7 0' > "$tmp/no_instructions.out"
callgrind 'Ir sysCount sysTime' '1000 0 0' '1000 0 0' > "$tmp/no_syscalls.out"
callgrind 'Ir sysTime' '1000 55' '1000 55' > "$tmp/no_syscall_event.out"
callgrind 'Ir sysCount sysTime' '' '1000 7 55' | grep -v '^summary:' > "$tmp/no_summary.out"
: > "$tmp/empty.out"

fail() { echo "FAIL: $1"; fails=$((fails+1)); }

# run <argument>...: sets $status, $out and $err.
run() {
  bash "$under_test" "$@" > "$tmp/out" 2> "$tmp/err"
  status=$?
  out=$(cat "$tmp/out")
  err=$(cat "$tmp/err")
}

# field <description> <arguments>... <expected JSON>
field() {
  local description=$1 expected=${*: -1}
  set -- "${@:2:$#-2}"
  run "$@"
  [ "$status" -eq 0 ] && [ "$(jq -c . <<< "$out")" = "$expected" ] \
    || fail "$description: status $status, printed '$out'"
}

# refused <description> <argument>...
refused() {
  local description=$1
  shift
  run "$@"
  [ "$status" -eq 0 ] && [ -z "$out" ] && [[ $err == *'::warning::'*'the record has no runner_setup'* ]] \
    || fail "$description: status $status, printed '$out', warned '$err'"
}

# controls <script>
# Runs the controls against <script>, and counts in $fails those that fail.
controls() {
  fails=0
  under_test=$1

  field "the totals, and the tests of the records"  "$tmp/ok.out" "$records" 0 "$boundary" \
        '{"boundary":"B","exit_status":0,"tests":3,"instructions":1000,"syscalls":7}'
  field "events read by name, not by place"         "$tmp/reordered.out" "$records" 0 "$boundary" \
        '{"boundary":"B","exit_status":0,"tests":3,"instructions":1000,"syscalls":7}'
  field "a failed run recorded as failed"           "$tmp/ok.out" "$records" 82 "$boundary" \
        '{"boundary":"B","exit_status":82,"tests":3,"instructions":1000,"syscalls":7}'
  field "the boundary as given"                     "$tmp/ok.out" "$records" 0 'a b' \
        '{"boundary":"a b","exit_status":0,"tests":3,"instructions":1000,"syscalls":7}'

  refused "a summary which differs from its totals" "$tmp/differs.out" "$records" 0 "$boundary"
  refused "no summary"                              "$tmp/no_summary.out" "$records" 0 "$boundary"
  refused "no totals"                               "$tmp/no_totals.out" "$records" 0 "$boundary"
  refused "no instructions"                         "$tmp/no_instructions.out" "$records" 0 "$boundary"
  refused "no system calls"                         "$tmp/no_syscalls.out" "$records" 0 "$boundary"
  refused "no system call event"                    "$tmp/no_syscall_event.out" "$records" 0 "$boundary"
  refused "an empty output"                         "$tmp/empty.out" "$records" 0 "$boundary"
  refused "no output file"                          "$tmp/absent.out" "$records" 0 "$boundary"
  refused "no records"                              "$tmp/ok.out" "$tmp/none" 0 "$boundary"
  refused "a records directory which is absent"     "$tmp/ok.out" "$tmp/absent" 0 "$boundary"
  refused "an exit status which is no number"       "$tmp/ok.out" "$records" ok "$boundary"
  refused "an empty exit status"                    "$tmp/ok.out" "$records" '' "$boundary"

  run "$tmp/ok.out" "$records" 0
  [ "$status" -eq 2 ] && [ -z "$out" ] && [[ $err == Usage:* ]] || fail "three arguments: status $status"
  run "$tmp/ok.out" "$records" 0 "$boundary" extra
  [ "$status" -eq 2 ] && [ -z "$out" ] && [[ $err == Usage:* ]] || fail "five arguments: status $status"
}

# Each mutant breaks one behaviour that the controls claim. Its entry holds a
# description, the text it replaces, and the replacement.
mutations=(
  'the tests from the calls into execute'  'tests=$(find "$records" -type f -name '"'"'*.txt'"'"' -size +0 2> /dev/null | wc -l | tr -d '"'"' '"'"')'
                                           'tests=$(awk '"'"'/^calls=/ { split($1, f, "="); n += f[2] } END { print n + 0 }'"'"' "$out")'
  'tests from every file'                  ' -size +0'                          ''
  'tests from the directory alone'         '-type f -name '"'"'*.txt'"'"''      '-name '"'"'*'"'"''
  'a summary alone read'                   '|| line["summary:"] != line["totals:"] \' '|| 0 \'
  'the events read by place'               'print f[col["Ir"]] + 0, f[col["sysCount"]] + 0' 'print f[1] + 0, f[2] + 0'
  'the instructions not checked'           '"$instructions" "$syscalls" "$tests"' '"$syscalls" "$tests"'
  'the system calls not checked'           '"$instructions" "$syscalls" "$tests"' '"$instructions" "$tests"'
  'the tests not checked'                  '"$instructions" "$syscalls" "$tests"; do' '"$instructions" "$syscalls"; do'
  'zero accepted'                          "''|*[!0-9]*|0)"                     "''|*[!0-9]*)"
  'a refusal an error'                     '  exit 0
}'                                         '  exit 1
}'
  'a refusal not warned'                   'echo "::warning::$1, so' 'echo "$1, so'
  'the warning on standard output'         'the record has no runner_setup" >&2' 'the record has no runner_setup"'
  'a failed run refused'                   '[[ $status =~ ^(0|[1-9][0-9]*)$ ]] ||' '[ "$status" -eq 0 ] ||'
  'any exit status accepted'               '[[ $status =~ ^(0|[1-9][0-9]*)$ ]] || refuse "the exit status '"'"'$status'"'"' is not a number"' ':'
  'the boundary not recorded'              '--arg boundary "$boundary"'         '--arg boundary x'
  'the exit status not recorded'           '--argjson exit_status "$status"'    '--argjson exit_status 0'
  'an argument fewer accepted'             '[ $# -ne 4 ]'                       '[ $# -lt 3 ]'
  'an argument more accepted'              '[ $# -ne 4 ]'                       '[ $# -lt 4 ]'
  'a refused call succeeds'                '  exit 2'                           '  exit 0'
)

if [ "$*" != --mutations ]; then
  controls "$script"
  if [ "$fails" -eq 0 ]; then echo "runner_setup_record: all controls pass"; else exit 1; fi
  exit 0
fi

source_text=$(cat "$script")
controls "$script" > "$tmp/controls.txt" 2>&1
echo "unmutated: $fails controls fail$([ "$fails" -eq 0 ] || echo '  <-- must be none')"
survivors=$fails
i=0
while [ "$i" -lt ${#mutations[@]} ]; do
  description=${mutations[i]} old=${mutations[i+1]} new=${mutations[i+2]}
  i=$((i+3))
  after=${source_text#*"$old"}
  if [ "$after" = "$source_text" ] || [ "${after#*"$old"}" != "$after" ]; then
    echo "$description: the text to mutate does not occur exactly once"
    survivors=$((survivors+1))
    continue
  fi
  printf '%s\n' "${source_text%%"$old"*}$new$after" > "$tmp/mutant.sh"
  controls "$tmp/mutant.sh" > "$tmp/controls.txt" 2>&1
  echo "$description: $fails controls fail$([ "$fails" -ne 0 ] || echo '  <-- SURVIVED')"
  [ "$fails" -ne 0 ] || survivors=$((survivors+1))
done
[ "$survivors" -eq 0 ]
