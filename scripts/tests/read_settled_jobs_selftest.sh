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
# The selftest shrinks the wait to 4 s, with a pause of 1 s between reads. Each
# control is a claim the script exists to keep:
#
#   - a read on which the filter awaits nothing ends the wait at once, and is
#     the jobs file;
#   - the read asks for every page of the jobs of the run named, in $GH_REPO;
#   - a read on which the filter awaits something is followed by a log line
#     naming what the filter awaits, a pause of the interval, and another read;
#   - the options after the filter reach jq;
#   - a read that fails, or gives output that is not JSON, is retried, and does
#     not replace the jobs file;
#   - the reads end once the time runs out, within a pause and a read, with no
#     pause after the last;
#   - when the time runs out, a warning names what the filter still awaits, or
#     says that the last read failed. The jobs file holds the last successful
#     read, and the script succeeds, leaving the judgement to the caller;
#   - if no read succeeds, the script fails with an error;
#   - a filter that jq cannot run fails the script, as does a jobs file that
#     cannot be written;
#   - fewer than three arguments, and an unset or empty $GH_REPO, are refused
#     before any read, with the usage on standard error;
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
# Runs the script in a directory of its own, $dir, serving the reads in order.
# Sets $status, and $calls to the number of reads.
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

# The time in tenths of a second. The script's deadline is in whole seconds,
# and the time a read takes varies, so the selftest times the wait finer.
tenths() { perl -MTime::HiRes=time -e 'printf "%d\n", time * 10'; }

same_json() { [ "$(jq -c . "$1" 2> /dev/null)" = "$(jq -c . <<<"$2")" ]; }

# refused <description> <env argument>...
# Runs `env <env argument>...` in a directory of its own, $dir, with a settled
# read to serve. Checks that the script refused: exit status 2, the usage on
# standard error alone, and no read.
refused() {
  local description=$1
  shift
  dir=$cases/refused
  rm -rf "$dir"
  mkdir -p "$dir/reads"
  echo "$settled" > "$dir/reads/1"
  (cd "$dir" && export FAKE=$dir PATH="$tmp/bin:$PATH" && env "$@" > out.txt 2> err.txt)
  status=$?
  calls=$(cat "$dir/calls" 2> /dev/null || echo 0)
  [ "$status" -eq 2 ] && [ "$calls" -eq 0 ] || fail "$description: status $status after $calls reads"
  [ ! -s "$dir/out.txt" ] && grep -q '^Usage: ' "$dir/err.txt" \
    || fail "$description: the usage was not on standard error alone"
}

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
  local asked
  asked=$(cat "$dir/arguments" 2> /dev/null)
  [ "$asked" = "api repos/owner/repo/actions/runs/1/jobs?per_page=100 --paginate --jq .jobs[]" ] \
    || fail "the read asked for: $asked"

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

  # The script's clock counts whole seconds, so its deadline falls 3 s to 4 s
  # after it starts. The last read can end a pause and a read after that. Each
  # read here takes far less than 1 s, and the bound tolerates 1 s.
  local began
  began=$(tenths)
  run never "$pending"
  local elapsed=$(( $(tenths) - began ))
  [ "$status" -eq 0 ] || fail "a read that never settles: status $status"
  [ "$elapsed" -ge 30 ] && [ "$elapsed" -lt 70 ] \
    || fail "a read that never settles was read for $elapsed tenths of a second, against a limit of 4 s"
  [ "$calls" -ge 2 ] || fail "a read that never settles was read $calls times"
  paused $((calls - 1)) || fail "$calls reads were not separated by one pause of the interval each"
  check "a read that never settles ends in a warning naming what is awaited" yes \
        "^::warning::.*awaited for t / B, and the wait's 4 s have run out" "$dir/out.txt"
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
  check "no successful read gives no warning" no "^::warning::" "$dir/out.txt"

  filter='.[] | no_such_function'
  run bad_filter "$settled"
  [ "$status" -ne 0 ] || fail "a filter jq cannot run did not fail the script"
  unset filter

  jobs=missing/jobs.json
  run unwritable "$settled"
  [ "$status" -ne 0 ] || fail "a jobs file that cannot be written did not fail the script"
  unset jobs

  refused "two arguments" GH_REPO=owner/repo bash "$cases/read_settled_jobs.sh" 1 jobs.json
  refused "an unset GH_REPO" -u GH_REPO bash "$cases/read_settled_jobs.sh" 1 jobs.json "$awaiting"
  refused "an empty GH_REPO" GH_REPO= bash "$cases/read_settled_jobs.sh" 1 jobs.json "$awaiting"

  local left
  left=$(cd "$cases" && find . -path '*/scratch/*')
  [ -z "$left" ] || fail "runs left temporary files behind: $left"
}

# Each mutant breaks one behaviour that the controls claim. Its entry holds a
# description, the text it replaces, and the replacement. Two mutants are left
# out:
#   - A deadline one second later: `-gt` for `-ge`, or a second added to the
#     limit. The script reads the clock in whole seconds, so its deadline
#     already falls anywhere within a second, and no control can see the
#     difference.
#   - `set -e` for `set -eu`, which is equivalent: no variable the script reads
#     can be unset once the arguments and $GH_REPO have been checked.
mutations=(
  'two arguments accepted'          'if [ $# -lt 3 ] ||'               'if [ $# -lt 2 ] ||'
  'three arguments refused'         'if [ $# -lt 3 ] ||'               'if [ $# -lt 4 ] ||'
  'an unset GH_REPO accepted'       '[ -z "${GH_REPO:-}" ]'            '[ -z "${GH_REPO-set}" ]'
  'an empty GH_REPO accepted'       '[ -z "${GH_REPO:-}" ]'            '[ -z "${GH_REPO+set}" ]'
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
  'a pause after the last read'     '  if [ "$SECONDS" -ge "$deadline" ]; then'
                                    '  sleep "$interval"; if [ "$SECONDS" -ge "$deadline" ]; then'
  'twice the time'                  '$((SECONDS + wait_limit))'        '$((SECONDS + 2 * wait_limit))'
  'the warning not an annotation'   'echo "::warning::'                'echo "warning: '
  'running out of time fails'       $'read."\n    break'               $'read."\n    exit 1'
  'a successful read forgotten'     '    read_succeeded=true'          '    read_succeeded=false'
  'no successful read needed'       $'read_succeeded=false\nwhile'     $'read_succeeded=true\nwhile'
  'no successful read succeeds'     '      exit 1'                     '      exit 0'
  'a warning without a read'        '    if ! $read_succeeded; then'
                                    '    echo "::warning::"; if ! $read_succeeded; then'
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
