#!/bin/bash
# Controls for rerun_unacquired_job.sh, against a fake gh which serves one jobs
# list and records each re-run asked for, and a mutation check of the controls.
#
#   rerun_unacquired_job_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the script and against each mutant of the script.
# The script must fail no control, and each mutant at least one.
#
# The jobs lists take the shapes of runs that lost jobs to the runners:
# 38425439632 lost a column; 37364856636 lost scope and every job after it;
# 37365728782 lost scope, while the columns ran and failed without its answer;
# 36712184199 lost an audit. Each control is a claim the script exists to keep:
#
#   - in each of those shapes, exactly one re-run is asked for, of the lost job
#     with the lowest id, whatever order the jobs come in;
#   - a job is lost only if it concluded `cancelled` or `failure` with no steps
#     and no runner; a job that ran and failed, a job with steps or a runner,
#     and a skipped job are never re-run;
#   - with no lost job, nothing is re-run, and the log says so;
#   - at the last attempt allowed, nothing is re-run, and a warning says why;
#     at the attempt before it, the re-run is still asked for;
#   - the read asks for every page of the latest attempt of each job of the run
#     named, in $GH_REPO;
#   - a read or a re-run that fails fails the script;
#   - arguments other than two positive whole numbers, and an unset or empty
#     $GH_REPO, are refused before any read, with the usage on standard error.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script=$here/../rerun_unacquired_job.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

case "$*" in
  ''|--mutations) ;;
  *) echo "Usage: $0 [--mutations]" >&2; exit 2;;
esac

# The fake gh serves $FAKE/jobs.json, one job per line as `--jq '.jobs[]'`
# gives them, to a read, and appends the path of each POST to $FAKE/reruns.
# Each call appends its arguments to $FAKE/calls. $FAKE/read_fails or
# $FAKE/rerun_fails makes that call fail as gh does on an HTTP error.
mkdir "$tmp/bin"
cat > "$tmp/bin/gh" <<'GH'
#!/bin/bash
echo "$*" >> "$FAKE/calls"
if [ "$2" = -X ]; then
  [ ! -f "$FAKE/rerun_fails" ] || { echo '{"message":"Forbidden","status":"403"}'; exit 1; }
  echo "$4" >> "$FAKE/reruns"
  exit 0
fi
[ ! -f "$FAKE/read_fails" ] || { echo '{"message":"Server Error","status":"502"}'; exit 1; }
jq -c '.[]' "$FAKE/jobs.json"
GH
chmod +x "$tmp/bin/gh"

# job <id> <conclusion> <steps> <runner> <name>: one job of a jobs list.
job() {
  local steps
  steps=$(seq "$3" | jq -R . | jq -s .)
  jq -n --argjson id "$1" --arg conclusion "$2" --argjson steps "$steps" --arg runner "$4" --arg name "$5" \
     '{id: $id, name: $name, conclusion: $conclusion, steps: $steps, runner_name: (if $runner == "" then null else $runner end)}'
}

lost_column=$({ job 10 success 3 r1 scope
                job 20 cancelled 0 '' 'coarse / macOS'
                job 21 success 32 r2 'coarse / Linux'
                job 30 failure 5 r3 'coarse / audit'
                job 40 failure 5 r4 'Every job ran'; } | jq -s .)
lost_all=$({ job 10 cancelled 0 '' scope
             job 20 cancelled 0 '' 'coarse / macOS'
             job 21 cancelled 0 '' 'coarse / Linux'
             job 30 cancelled 0 '' 'coarse / audit'
             job 40 failure 5 r1 'Every job ran'; } | jq -s .)
lost_scope_columns_ran=$({ job 10 cancelled 0 '' scope
                           job 20 failure 31 r1 'coarse / macOS'
                           job 21 failure 31 r2 'coarse / Linux'
                           job 30 cancelled 0 '' 'coarse / audit'
                           job 40 cancelled 0 '' 'Every job ran'; } | jq -s .)
lost_audit=$({ job 10 success 3 r1 scope
               job 20 success 28 r2 'coarse / macOS'
               job 30 failure 0 '' 'coarse / audit'
               job 40 failure 5 r3 'Every job ran'; } | jq -s .)
real_failure=$({ job 10 success 3 r1 scope
                 job 20 failure 32 r2 'fine / Windows'
                 job 40 failure 5 r3 'Every job ran'; } | jq -s .)
not_lost=$({ job 10 success 3 r1 scope
             job 20 cancelled 12 r2 'cancelled with steps'
             job 21 failure 0 r3 'a runner, no steps'
             job 22 cancelled 3 '' 'steps, no runner'
             job 23 skipped 0 '' 'skipped'
             job 40 failure 5 r4 'Every job ran'; } | jq -s .)
all_success=$({ job 10 success 3 r1 scope
                job 20 success 32 r2 'coarse / macOS'; } | jq -s .)

fail() { echo "FAIL: $1"; fails=$((fails+1)); }

# check <name> <yes|no> <pattern> <file>
check() {
  if grep -qE "$3" "$4" 2> /dev/null; then got=yes; else got=no; fi
  [ "$got" = "$2" ] || fail "$1 (expected $2, got $got, pattern: $3)"
}

# run <case> <jobs> [<attempt>]
# Runs the script in a directory of its own, $dir, serving <jobs>, as attempt
# <attempt> of run 7, by default the first. Sets $status, and $reruns to the
# paths re-run, one per line.
run() {
  dir=$cases/$1
  mkdir -p "$dir"
  echo "$2" > "$dir/jobs.json"
  (cd "$dir" && FAKE=$dir PATH="$tmp/bin:$PATH" GH_REPO=owner/repo \
     bash "$subject" 7 "${3:-1}" > out.txt 2>&1)
  status=$?
  reruns=$(cat "$dir/reruns" 2> /dev/null)
}

# reran <description> <job id>: whether the last run asked for exactly one
# re-run, of <job id>, and succeeded.
reran() {
  [ "$status" -eq 0 ] && [ "$reruns" = "repos/owner/repo/actions/jobs/$2/rerun" ] \
    || fail "$1: status $status, re-runs: ${reruns:-none}"
}

# reran_none <description>: whether the last run asked for no re-run, and
# succeeded.
reran_none() {
  [ "$status" -eq 0 ] && [ -z "$reruns" ] || fail "$1: status $status, re-runs: ${reruns:-none}"
}

# refused <description> <env argument>...
# Runs `env <env argument>...` in a directory of its own, $dir, with a lost
# column to serve. Checks that the script refused: exit status 2, the usage on
# standard error alone, and no call of gh.
refused() {
  local description=$1
  shift
  dir=$cases/refused
  rm -rf "$dir"
  mkdir -p "$dir"
  echo "$lost_column" > "$dir/jobs.json"
  (cd "$dir" && export FAKE=$dir PATH="$tmp/bin:$PATH" && env "$@" > out.txt 2> err.txt)
  status=$?
  [ "$status" -eq 2 ] && [ ! -f "$dir/calls" ] || fail "$description: status $status, calls: $(cat "$dir/calls" 2> /dev/null)"
  [ ! -s "$dir/out.txt" ] && grep -q '^Usage: ' "$dir/err.txt" \
    || fail "$description: the usage was not on standard error alone"
}

# controls <script>
# Runs the controls against <script>, and counts in $fails those that fail.
controls() {
  fails=0
  cases=$tmp/cases
  rm -rf "$cases"
  mkdir "$cases"
  subject=$1

  run lost_column "$lost_column"
  reran "a lost column" 20
  check "a lost column is named" yes "^  coarse / macOS$" "$dir/out.txt"
  check "the re-run is logged" yes "^Re-running coarse / macOS, and the jobs that need it\.$" "$dir/out.txt"
  local asked
  asked=$(head -1 "$dir/calls" 2> /dev/null)
  [ "$asked" = "api repos/owner/repo/actions/runs/7/jobs?filter=latest&per_page=100 --paginate --jq .jobs[]" ] \
    || fail "the read asked for: $asked"

  run lost_all "$lost_all"
  reran "scope and every job after it lost" 10

  run lost_scope_columns_ran "$lost_scope_columns_ran"
  reran "scope lost, the columns run and failed" 10

  run lost_audit "$lost_audit"
  reran "a lost audit" 30

  run lost_all_reversed "$(jq 'reverse' <<< "$lost_all")"
  reran "scope and every job after it lost, in reverse order" 10

  run real_failure "$real_failure"
  reran_none "a column that ran and failed"
  check "no lost job is logged" yes "^No job of run 7 lacked a runner\.$" "$dir/out.txt"

  run not_lost "$not_lost"
  reran_none "jobs with steps, a runner, or skipped"

  run all_success "$all_success"
  reran_none "every job succeeded"

  run last_attempt "$lost_column" 4
  reran_none "a lost column at the last attempt allowed"
  check "the last attempt warns" yes "^::warning::Run 7 has had 4 attempts" "$dir/out.txt"

  run attempt_before_last "$lost_column" 3
  reran "a lost column at the attempt before the last" 20
  check "an attempt before the last does not warn" no "::warning::" "$dir/out.txt"

  dir=$cases/read_fails
  mkdir -p "$dir"
  touch "$dir/read_fails"
  run read_fails "$lost_column"
  [ "$status" -ne 0 ] && [ -z "$reruns" ] || fail "a failed read: status $status, re-runs: ${reruns:-none}"

  dir=$cases/rerun_fails
  mkdir -p "$dir"
  touch "$dir/rerun_fails"
  run rerun_fails "$lost_column"
  [ "$status" -ne 0 ] || fail "a failed re-run did not fail the script"
  check "a failed re-run is not logged as made" no "^Re-running" "$dir/out.txt"

  refused "one argument"            GH_REPO=owner/repo bash "$subject" 7
  refused "three arguments"         GH_REPO=owner/repo bash "$subject" 7 1 1
  refused "a run id not a number"   GH_REPO=owner/repo bash "$subject" seven 1
  refused "an attempt of 0"         GH_REPO=owner/repo bash "$subject" 7 0
  refused "a leading zero"          GH_REPO=owner/repo bash "$subject" 7 01
  refused "an unset GH_REPO"        -u GH_REPO bash "$subject" 7 1
  refused "an empty GH_REPO"        GH_REPO= bash "$subject" 7 1
}

# Each mutant breaks one behaviour that the controls claim. Its entry holds a
# description, the text it replaces, and the replacement.
mutations=(
  'one argument accepted'           '[ $# -eq 2 ]'                     '[ $# -ge 1 ]'
  'three arguments accepted'        '[ $# -eq 2 ]'                     '[ $# -ge 2 ]'
  'an unset GH_REPO accepted'       '[ -n "${GH_REPO:-}" ]'            '[ -n "${GH_REPO-set}" ]'
  'an empty GH_REPO accepted'       '[ -n "${GH_REPO:-}" ]'            '[ -n "${GH_REPO+set}" ]'
  'a run id not checked'            'positive "$run_id" && '           ''
  'an attempt not checked'          ' && positive "$attempt" || usage' ' || usage'
  'a leading zero accepted'         '^[1-9][0-9]*$'                    '^[0-9]+$'
  'a refusal fails as any error'    '  exit 2'                         '  exit 1'
  'the usage on standard output'    '<attempt>" >&2'                   '<attempt>"'
  'errors ignored'                  'set -eu'                          'set -u'
  'a failed read read as empty'     'set -euo pipefail'                'set -eu'
  'one page only'                   ' --paginate'                      ''
  'every attempt read'              'filter=latest&'                   'filter=all&'
  'a failed read ignored'           "--jq '.jobs[]' | jq -s .)"        "--jq '.jobs[]' | jq -s . || echo '[]')"
  'a skipped job lost'              '(.conclusion == "cancelled" or .conclusion == "failure")'
                                    '(.conclusion != "success")'
  'a failed job never lost'         ' or .conclusion == "failure")'    ')'
  'a job with steps lost'           $'\n             and (.steps | length) == 0' ''
  'a job with a runner lost'        $'\n             and (.runner_name // "") == ""' ''
  'the highest id re-run'           '  | sort_by(.id)'                 '  | sort_by(.id) | reverse'
  'the jobs order kept'             '  | sort_by(.id)'                 '  | .'
  'no lost job reported as lost'    $'if [ -z "$lost" ]; then\n  echo "No job of run $run_id lacked a runner."\n  exit 0'
                                    $'if [ -z "$lost" ]; then\n  echo "No job of run $run_id lacked a runner."\n  exit 1'
  'the attempts unbounded'          'if [ "$attempt" -ge "$max_attempts" ]; then'
                                    'if false; then'
  'one attempt fewer'               'if [ "$attempt" -ge "$max_attempts" ]; then'
                                    'if [ "$attempt" -ge "$((max_attempts - 1))" ]; then'
  'the warning not an annotation'   'echo "::warning::'                'echo "warning: '
  'the last attempt fails'          $'re-runs are made."\n  exit 0'    $'re-runs are made."\n  exit 1'
  'a failed re-run ignored'         '/rerun" > /dev/null'              '/rerun" > /dev/null || :'
  'the run re-run whole'            'actions/jobs/$job_id/rerun'       'actions/runs/$run_id/rerun-failed-jobs'
  'the lost jobs not named'         "printf '%s\\n' \"\$lost\" | cut -f2 | sed 's/^/  /'"
                                    ':'
)

if [ "$*" != --mutations ]; then
  controls "$script"
  if [ "$fails" -eq 0 ]; then echo "rerun_unacquired_job: all controls pass"; else exit 1; fi
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
