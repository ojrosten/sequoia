#!/bin/bash
# Controls for leftover_materials_setup_record.sh, against files of counts
# built here, and a mutation check of the controls.
#
#   leftover_materials_setup_record_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the script and against each mutant of the script.
# The script must fail no control, and each mutant at least one.
#
# The fixtures are the output of runner_setup_record.sh for two runs, whose
# instructions differ so that the two cannot be swapped unseen, and whose
# system calls differ from every other count. Each control is a claim the
# script exists to keep:
#   - the field holds the boundary, the entries, and for each run its
#     instructions, tests and exit status, the fresh run's under "fresh" and
#     the seeded run's under "seeded";
#   - the system calls are left out;
#   - a run which failed is recorded with its exit status, since the dashboard
#     plots a point whatever its status;
#   - 0 previous entries is recorded, not refused;
#   - each of these is refused, and the script then succeeds, warns on
#     standard error, and prints nothing: a counts file which is absent,
#     empty or not JSON, or lacks the boundary, the exit status, the tests or
#     the instructions, whichever run it is; two boundaries which differ; and
#     a number of entries which is no number;
#   - arguments of any other number are refused with status 2.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script=$here/../leftover_materials_setup_record.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

case "$*" in
  ''|--mutations) ;;
  *) echo "Usage: $0 [--mutations]" >&2; exit 2;;
esac

# counts <file> <instructions> <tests> <exit status> <boundary>
counts() {
  printf '{"boundary":"%s","exit_status":%s,"tests":%s,"instructions":%s,"syscalls":631592}\n' "$5" "$4" "$3" "$2" > "$1"
}

counts "$tmp/fresh.json"  50083024 97 0  B
counts "$tmp/seeded.json" 54377059 97 0  B
counts "$tmp/seeded-failed.json" 54377059 97 82 B
counts "$tmp/fresh-failed.json"  50083024 97 82 B
counts "$tmp/other-boundary.json" 54377059 97 0 C
: > "$tmp/empty.json"
echo 'not json' > "$tmp/garbage.json"
for lacking in boundary exit_status tests instructions; do
  jq "del(.$lacking)" "$tmp/seeded.json" > "$tmp/seeded-without-$lacking.json"
  jq "del(.$lacking)" "$tmp/fresh.json"  > "$tmp/fresh-without-$lacking.json"
done
counts "$tmp/seeded-no-instructions.json" 0 97 0 B
counts "$tmp/seeded-no-tests.json" 54377059 0 0 B
jq '.boundary = 7' "$tmp/seeded.json" > "$tmp/seeded-numeric-boundary.json"
jq '.boundary = 7' "$tmp/fresh.json" > "$tmp/fresh-numeric-boundary.json"
jq '.exit_status = "0"' "$tmp/seeded.json" > "$tmp/seeded-text-status.json"

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
  [ "$status" -eq 0 ] && [ "$(jq -cS . <<< "$out")" = "$expected" ] \
    || fail "$description: status $status, printed '$out'"
}

# refused <description> <argument>...
refused() {
  local description=$1
  shift
  run "$@"
  [ "$status" -eq 0 ] && [ -z "$out" ] && [[ $err == *'::warning::'*'the record has no leftover_materials_setup'* ]] \
    || fail "$description: status $status, printed '$out', warned '$err'"
}

# controls <script>
# Runs the controls against <script>, and counts in $fails those that fail.
controls() {
  fails=0
  under_test=$1

  field "each run's counts, under its own name" "$tmp/fresh.json" "$tmp/seeded.json" 2713 \
        '{"boundary":"B","fresh":{"exit_status":0,"instructions":50083024,"tests":97},"previous_entries":2713,"seeded":{"exit_status":0,"instructions":54377059,"tests":97}}'
  field "a failed seeded run, recorded as failed" "$tmp/fresh.json" "$tmp/seeded-failed.json" 2708 \
        '{"boundary":"B","fresh":{"exit_status":0,"instructions":50083024,"tests":97},"previous_entries":2708,"seeded":{"exit_status":82,"instructions":54377059,"tests":97}}'
  field "a failed fresh run, recorded as failed" "$tmp/fresh-failed.json" "$tmp/seeded.json" 2708 \
        '{"boundary":"B","fresh":{"exit_status":82,"instructions":50083024,"tests":97},"previous_entries":2708,"seeded":{"exit_status":0,"instructions":54377059,"tests":97}}'
  field "no previous entries, recorded" "$tmp/fresh.json" "$tmp/seeded.json" 0 \
        '{"boundary":"B","fresh":{"exit_status":0,"instructions":50083024,"tests":97},"previous_entries":0,"seeded":{"exit_status":0,"instructions":54377059,"tests":97}}'

  refused "a number of entries which is no number"   "$tmp/fresh.json" "$tmp/seeded.json" many
  refused "a negative number of entries"             "$tmp/fresh.json" "$tmp/seeded.json" -1
  refused "a fractional number of entries"           "$tmp/fresh.json" "$tmp/seeded.json" 1.5
  refused "an empty number of entries"               "$tmp/fresh.json" "$tmp/seeded.json" ''
  refused "a number of entries with a leading zero"  "$tmp/fresh.json" "$tmp/seeded.json" 007
  refused "two boundaries which differ"              "$tmp/fresh.json" "$tmp/other-boundary.json" 2713
  refused "an empty fresh file"                      "$tmp/empty.json" "$tmp/seeded.json" 2713
  refused "an empty seeded file"                     "$tmp/fresh.json" "$tmp/empty.json" 2713
  refused "an absent fresh file"                     "$tmp/absent.json" "$tmp/seeded.json" 2713
  refused "an absent seeded file"                    "$tmp/fresh.json" "$tmp/absent.json" 2713
  refused "a fresh file which is no JSON"            "$tmp/garbage.json" "$tmp/seeded.json" 2713
  refused "a seeded file which is no JSON"           "$tmp/fresh.json" "$tmp/garbage.json" 2713
  for lacking in boundary exit_status tests instructions; do
    refused "a fresh file without $lacking"          "$tmp/fresh-without-$lacking.json" "$tmp/seeded.json" 2713
    refused "a seeded file without $lacking"         "$tmp/fresh.json" "$tmp/seeded-without-$lacking.json" 2713
  done
  refused "a seeded file with no instructions"       "$tmp/fresh.json" "$tmp/seeded-no-instructions.json" 2713
  refused "a seeded file with no tests"              "$tmp/fresh.json" "$tmp/seeded-no-tests.json" 2713
  refused "boundaries which are no text, and equal" "$tmp/fresh-numeric-boundary.json" "$tmp/seeded-numeric-boundary.json" 2713
  refused "a seeded file with an exit status which is no number" "$tmp/fresh.json" "$tmp/seeded-text-status.json" 2713

  run "$tmp/fresh.json" "$tmp/seeded.json"
  [ "$status" -eq 2 ] && [ -z "$out" ] && [[ $err == Usage:* ]] || fail "two arguments: status $status"
  run "$tmp/fresh.json" "$tmp/seeded.json" 2713 extra
  [ "$status" -eq 2 ] && [ -z "$out" ] && [[ $err == Usage:* ]] || fail "four arguments: status $status"
}

# Each mutant breaks one behaviour that the controls claim. Its entry holds a
# description, the text it replaces, and the replacement.
mutations=(
  'the runs swapped'                       'fresh: ($fresh[0] | run), seeded: ($seeded[0] | run)'
                                           'fresh: ($seeded[0] | run), seeded: ($fresh[0] | run)'
  'the system calls kept'                  'def run: {instructions, tests, exit_status};' 'def run: {instructions, tests, exit_status, syscalls};'
  'the instructions left out'              'def run: {instructions, tests, exit_status};' 'def run: {tests, exit_status};'
  'the tests left out'                     'def run: {instructions, tests, exit_status};' 'def run: {instructions, exit_status};'
  'the exit status left out'               'def run: {instructions, tests, exit_status};' 'def run: {instructions, tests};'
  'the entries not recorded'               'previous_entries: $previous,' 'previous_entries: 0,'
  'the boundary not recorded'              'boundary: $fresh[0].boundary,' 'boundary: "x",'
  'no entries refused'                     '^(0|[1-9][0-9]*)$' '^[1-9][0-9]*$'
  'any number of entries accepted'         '[[ $previous =~ ^(0|[1-9][0-9]*)$ ]] || refuse "the number of previous entries '"'"'$previous'"'"' is not a number"' ':'
  'a leading zero accepted'                '^(0|[1-9][0-9]*)$' '^[0-9]+$'
  'the fresh counts not checked'           'counted "$fresh" ||' ':'
  'the seeded counts not checked'          'counted "$seeded" ||' ':'
  'the boundaries not compared'            '[ "$(jq -r '"'"'.boundary'"'"' "$fresh")" = "$(jq -r '"'"'.boundary'"'"' "$seeded")" ] \
  ||' ':'
  'a boundary of no text accepted'         '(.boundary | type == "string")' '(.boundary != null)'
  'an exit status of no number accepted'   '(.exit_status | type == "number" and . >= 0 and floor == .)' '(.exit_status != null)'
  'zero tests accepted'                     '[.tests, .instructions] | all(type == "number" and . > 0 and floor == .)' '[.tests, .instructions] | all(type == "number" and . >= 0 and floor == .)'
  'zero instructions accepted'             '[.tests, .instructions] | all(type == "number" and . > 0 and floor == .)' '[.tests] | all(type == "number" and . > 0 and floor == .)'
  'a refusal an error'                     '  exit 0
}'                                         '  exit 1
}'
  'a refusal not warned'                   'echo "::warning::$1, so' 'echo "$1, so'
  'the warning on standard output'         'leftover_materials_setup" >&2' 'leftover_materials_setup"'
  'an argument fewer accepted'             '[ $# -ne 3 ]'                       '[ $# -lt 2 ]'
  'an argument more accepted'              '[ $# -ne 3 ]'                       '[ $# -lt 3 ]'
  'a refused call succeeds'                '  exit 2'                           '  exit 0'
)

if [ "$*" != --mutations ]; then
  controls "$script"
  if [ "$fails" -eq 0 ]; then echo "leftover_materials_setup_record: all controls pass"; else exit 1; fi
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
