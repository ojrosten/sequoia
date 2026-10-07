#!/bin/bash
# Controls for read_settled_jobs.sh, against a fake gh which serves one canned
# read of the jobs API per call, and a mutation check of the controls.
#
#   read_settled_jobs_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the script and against each mutant of the script.
# The script must fail no control, and each mutant at least one. One run of the
# controls takes about 20 s, since four of them wait for the time to run out.
#
# The wait is shrunk to 4 s, reading every 1 s. Each control is a claim the
# script exists to keep:
#
#   - a read on which the filter awaits nothing ends the wait at once, and is
#     the jobs file;
#   - the read asks for every page of the jobs of the run named, in $GH_REPO;
#   - a read on which the filter awaits something is logged, naming what the
#     filter awaits, and is followed by a pause of the interval and another
#     read;
#   - the options after the filter reach jq;
#   - a read that fails, or gives output that is not JSON, is retried, and does
#     not replace the jobs file;
#   - the reads end once the time runs out, with no pause after the last;
#   - when the time runs out, a warning names what the filter still awaits, or
#     says that the last read failed. The jobs file holds the last successful
#     read, and the script succeeds, leaving the judgement to the caller;
#   - if no read succeeds, the script fails with an error;
#   - a filter that jq cannot run fails the script, as does a jobs file that
#     cannot be written;
#   - fewer than three arguments are refused, with the usage on standard error;
#   - no run leaves a temporary directory behind.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script=$here/../read_settled_jobs.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

case "$*" in
  ''|--mutations) ;;
  *) echo "Usage: $0 [--mutations]" >&2; exit 2;;
esac

# The n-th call of the fake gh serves $FAKE/reads/<n>, or the last of them past
# the end. A read holding `fail` makes the call fail as gh does on an HTTP
# error: the error body on stdout, exit 1. A read holding `garbled` succeeds
# with output that is not JSON. The fake sleep records each pause, then takes
# it. The fake mktemp makes its directory within $FAKE/scratch, so that a
# control can see what a run leaves behind.
mkdir "$tmp/bin"
cat > "$tmp/bin/gh" <<'GH'
#!/bin/bash
n=$(( $(cat "$FAKE/calls" 2> /dev/null || echo 0) + 1 ))
echo "$n" > "$FAKE/calls"
echo "$*" > "$FAKE/arguments"
read=$FAKE/reads/$n
[ -f "$read" ] || read=$FAKE/reads/$(ls "$FAKE/reads" | sort -n | tail -1)
case "$(cat "$read")" in
  fail)    echo '{"message":"Server Error","status":"502"}'; exit 1;;
  garbled) echo '{"name": "t / A", "concl'; exit 0;;
esac
jq -c '.[]' "$read"
GH
cat > "$tmp/bin/sleep" <<SLEEP
#!/bin/bash
echo "\$*" >> "\$FAKE/pauses"
exec '$(command -v sleep)' "\$@"
SLEEP
cat > "$tmp/bin/mktemp" <<'MKTEMP'
#!/bin/bash
mkdir -p "$FAKE/scratch/reads" && echo "$FAKE/scratch/reads"
MKTEMP
chmod +x "$tmp/bin/gh" "$tmp/bin/sleep" "$tmp/bin/mktemp"

pending='[{"name": "t / A", "conclusion": "success"}, {"name": "t / B", "conclusion": null}]'
settled='[{"name": "t / A", "conclusion": "success"}, {"name": "t / B", "conclusion": "failure"}]'
awaiting='[.[] | select(.conclusion == null) | .name] | join(", ")'

fail() { echo "FAIL: $1"; fails=$((fails+1)); }

# check <name> <yes|no> <pattern> <file>
check() {
  if grep -qE "$3" "$4" 2> /dev/null; then got=yes; else got=no; fi
  [ "$got" = "$2" ] || fail "$1 (expected $2, got $got, pattern: $3)"
}

# run <case> <read>...
# Runs the script in the directory $dir, against the reads given, served in
# order. Sets $status and $calls, the number of reads.
run() {
  dir=$cases/$1
  shift
  mkdir -p "$dir/reads"
  local n=1
  for read in "$@"; do echo "$read" > "$dir/reads/$n"; n=$((n+1)); done
  (cd "$dir" && FAKE=$dir PATH="$tmp/bin:$PATH" GH_REPO=owner/repo \
     bash "$cases/read_settled_jobs.sh" 1 "${jobs:-jobs.json}" "${filter:-$awaiting}" \
          "${options[@]+"${options[@]}"}" > out.txt 2>&1)
  status=$?
  calls=$(cat "$dir/calls" 2> /dev/null || echo 0)
}

same_json() { [ "$(jq -c . "$1" 2> /dev/null)" = "$(jq -c . <<<"$2")" ]; }

# paused <count>: whether the last run paused <count> times, each for the
# interval.
paused() {
  local expected= n=0
  while [ "$n" -lt "$1" ]; do expected=$expected$'1\n'; n=$((n+1)); done
  [ "$(cat "$dir/pauses" 2> /dev/null)" = "${expected%$'\n'}" ]
}

# controls <script>
# Runs the controls against <script>, and counts in $fails those that fail.
controls() {
  fails=0
  cases=$tmp/cases
  rm -rf "$cases"
  mkdir "$cases"
  grep -q '^wait_limit=420 interval=10$' "$1" \
    || { fail "the wait's constants are not where this expects them"; return; }
  sed 's/^wait_limit=420 interval=10$/wait_limit=4 interval=1/' "$1" > "$cases/read_settled_jobs.sh"

  options=()

  run settled "$settled"
  [ "$status" -eq 0 ] && [ "$calls" -eq 1 ] || fail "a settled read: status $status after $calls reads"
  same_json "$dir/jobs.json" "$settled" || fail "a settled read is not the jobs file"
  check "a settled read logs no wait" no "reading again" "$dir/out.txt"
  paused 0 || fail "a settled read was followed by a pause"
  [ "$(cat "$dir/arguments")" = "api repos/owner/repo/actions/runs/1/jobs?per_page=100 --paginate --jq .jobs[]" ] \
    || fail "the read asked for: $(cat "$dir/arguments")"

  run pending_then_settled "$pending" "$settled"
  [ "$status" -eq 0 ] && [ "$calls" -eq 2 ] \
    || fail "a pending read, then a settled: status $status after $calls reads"
  check "a pending read is logged, naming what is awaited" yes "awaited for t / B; reading again" "$dir/out.txt"
  paused 1 || fail "a pending read was not followed by one pause of the interval"
  same_json "$dir/jobs.json" "$settled" || fail "the settled read is not the jobs file"

  filter='[.[] | select(.name == $job and .conclusion == null) | .name] | join(", ")'
  options=(--arg job "t / A")
  run options_reach_jq "$pending"
  [ "$status" -eq 0 ] && [ "$calls" -eq 1 ] \
    || fail "the options after the filter did not reach jq: status $status after $calls reads"
  unset filter
  options=()

  run fail_then_settled fail "$settled"
  [ "$status" -eq 0 ] && [ "$calls" -eq 2 ] \
    || fail "a failed read, then a settled: status $status after $calls reads"
  check "a failed read is logged" yes "could not be read; reading again" "$dir/out.txt"
  paused 1 || fail "a failed read was not followed by one pause of the interval"
  same_json "$dir/jobs.json" "$settled" || fail "the settled read after a failed one is not the jobs file"

  run garbled_then_settled garbled "$settled"
  [ "$status" -eq 0 ] && [ "$calls" -eq 2 ] \
    || fail "a garbled read, then a settled: status $status after $calls reads"
  same_json "$dir/jobs.json" "$settled" || fail "the settled read after a garbled one is not the jobs file"

  run never "$pending"
  [ "$status" -eq 0 ] || fail "a read that never settles: status $status"
  [ "$calls" -ge 3 ] && [ "$calls" -le 6 ] \
    || fail "a read that never settles was read $calls times in 4 s, a second apart"
  paused $((calls - 1)) || fail "$calls reads were not separated by one pause of the interval each"
  check "a read that never settles ends in a warning naming what is awaited" yes \
        "^::warning::.*awaited for t / B, 4s after the wait began" "$dir/out.txt"
  same_json "$dir/jobs.json" "$pending" || fail "the last read is not the jobs file"

  run pending_then_failing "$pending" fail
  [ "$status" -eq 0 ] || fail "a pending read, then failing reads: status $status"
  check "failing reads end in a warning" yes "^::warning::The jobs API could not be read" "$dir/out.txt"
  same_json "$dir/jobs.json" "$pending" || fail "the last successful read is not the jobs file"

  run pending_then_garbled "$pending" garbled
  [ "$status" -eq 0 ] || fail "a pending read, then garbled reads: status $status"
  check "garbled reads end in a warning" yes "^::warning::The jobs API could not be read" "$dir/out.txt"
  same_json "$dir/jobs.json" "$pending" || fail "a garbled read replaced the last successful read"

  run always_failing fail
  [ "$status" -ne 0 ] || fail "no read succeeded, yet the script succeeded"
  check "no successful read is an error" yes "^::error::No read of the jobs API succeeded" "$dir/out.txt"

  filter='.[] | no_such_function'
  run bad_filter "$settled"
  [ "$status" -ne 0 ] || fail "a filter jq cannot run did not fail the script"
  unset filter

  jobs=missing/jobs.json
  run unwritable "$settled"
  [ "$status" -ne 0 ] || fail "a jobs file that cannot be written did not fail the script"
  unset jobs

  (cd "$cases" && bash read_settled_jobs.sh 1 jobs.json > refused.out 2> refused.err)
  [ $? -eq 2 ] || fail "two arguments were not refused"
  [ ! -s "$cases/refused.out" ] && grep -q '^Usage: ' "$cases/refused.err" \
    || fail "the refusal of two arguments did not give the usage on standard error alone"

  local left
  left=$(cd "$cases" && find . -path '*/scratch/*')
  [ -z "$left" ] || fail "runs left temporary files behind: $left"
}

# Each mutant breaks one behaviour the controls claim, and is a description,
# the text it replaces, and the replacement. One mutant is left out as beyond
# the controls' resolution: a deadline one interval later. The script reads the
# clock in whole seconds, so the reads that fit before the deadline already
# vary by one.
mutations=(
  'two arguments accepted'          'if [ $# -lt 3 ]; then'            'if [ $# -lt 2 ]; then'
  'three arguments refused'         'if [ $# -lt 3 ]; then'            'if [ $# -lt 4 ]; then'
  'a refusal fails as any error'    '  exit 2'                         '  exit 1'
  'the usage on standard output'    '<jq option>...]" >&2'             '<jq option>...]"'
  'errors ignored'                  'set -eu'                          'set -u'
  'one page only'                   ' --paginate'                      ''
  'a failed read accepted'          '     && jq -s .'                  '     ; jq -s .'
  'the pages not combined'          'jq -s . "$reads/jobs.ndjson"'     'jq . "$reads/jobs.ndjson"'
  'a garbled read written'          '> "$reads/jobs.json"; then'
                                    '> "$jobs_file" && cp "$jobs_file" "$reads/jobs.json"; then'
  'the options not passed'          'jq -r "$@" "$filter"'             'jq -r "$filter"'
  'the filter output quoted'        'jq -r "$@"'                       'jq "$@"'
  'a filter error ignored'          '"$filter" "$jobs_file")'          '"$filter" "$jobs_file" || :)'
  'reads continue once settled'     '[ -n "$awaited" ] || break'       '[ -n "$awaited" ] || :'
  'what is awaited not named'       'awaited for $awaited'             'awaited'
  'a failed read not named'         'lacking="The jobs API could not be read"'
                                    'lacking="The jobs API has yet to give a conclusion"'
  'no pause'                        '  sleep "$interval"'              '  :'
  'a longer pause'                  '  sleep "$interval"'              '  sleep "$((interval * 2))"'
  'a pause after the last read'     '  [ "$SECONDS" -lt "$deadline" ] ||'
                                    '  sleep "$interval"; [ "$SECONDS" -lt "$deadline" ] ||'
  'twice the time'                  '$((SECONDS + wait_limit))'        '$((SECONDS + 2 * wait_limit))'
  'the warning not an annotation'   'echo "::warning::'                'echo "warning: '
  'running out of time fails'       'last successful read."; break; }' 'last successful read."; exit 1; }'
  'a successful read forgotten'     '    read_succeeded=true'          '    read_succeeded=false'
  'no successful read needed'       $'read_succeeded=false\nwhile'     $'read_succeeded=true\nwhile'
  'no successful read succeeds'     'exit 1; }'                        'exit 0; }'
  'the error not an annotation'     'echo "::error::'                  'echo "error: '
  'the reads left behind'           $'trap \'rm -rf "$reads"\' EXIT'   ':'
)

if [ "$*" != --mutations ]; then
  controls "$script"
  if [ "$fails" -eq 0 ]; then echo "read_settled_jobs: all controls pass"; else exit 1; fi
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
