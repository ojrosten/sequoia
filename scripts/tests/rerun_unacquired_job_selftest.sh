#!/bin/bash
# Controls for rerun_unacquired_job.sh, against a fake gh which serves one run,
# its jobs and their annotations, and records each re-run asked for; and a
# mutation check of the controls.
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
# 36712184199 lost an audit, annotated in GitHub's other wording; and the
# second attempt of 38425439632, in which the job re-run has a lower id than
# the scope it needs. Each control is a claim the script exists to keep:
#
#   - in each of those shapes, exactly one re-run is asked for, of the lost job
#     with the lowest id, whatever order the jobs come in;
#   - a job is lost only if it concluded `cancelled` or `failure` with no steps
#     and no runner, and an annotation says it was not acquired; a job lacking
#     any one of those is never re-run;
#   - with no lost job, nothing is re-run, and the log says so;
#   - a run superseded by a newer run of its workflow, branch and event is not
#     re-run, and its jobs are not read;
#   - at the last attempt allowed, or later, nothing is re-run, and a warning
#     says why; at the attempt before it, the re-run is still asked for;
#   - the jobs are read through read_settled_jobs.sh, every page of the run
#     named, in $GH_REPO;
#   - a job's name is logged on one line, whatever it holds;
#   - a read of the run, the jobs or an annotation that fails, and a re-run
#     that fails, fail the script;
#   - arguments other than two positive whole numbers, and an unset or empty
#     $GH_REPO, are refused before anything is read, with the usage on
#     standard error.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script=$here/../rerun_unacquired_job.sh
settled_jobs_reader=$here/../read_settled_jobs.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

case "$*" in
  ''|--mutations) ;;
  *) echo "Usage: $0 [--mutations]" >&2; exit 2;;
esac

# The fake gh serves $FAKE/run.json for the run, $FAKE/newest as the newest run
# of its workflow, $FAKE/jobs.json for the jobs, one per line, and each job's
# `annotation` field as that job's annotations. It appends the path of each
# POST to $FAKE/reruns, and every call's arguments to $FAKE/calls. A file
# $FAKE/<run|jobs|annotations|rerun>_fails makes that call fail as gh does on
# an HTTP error.
mkdir "$tmp/bin"
cat > "$tmp/bin/gh" <<'GH'
#!/bin/bash
echo "$*" >> "$FAKE/calls"
shift
method=GET
if [ "$1" = -X ]; then method=$2; shift 2; fi
path=$1
failing() { [ -f "$FAKE/$1_fails" ] && { echo '{"message":"Server Error","status":"502"}'; exit 1; }; }
case "$method $path" in
  'POST '*/rerun)                  failing rerun; echo "$path" >> "$FAKE/reruns";;
  'GET '*/actions/workflows/*/runs) cat "$FAKE/newest";;
  'GET '*/check-runs/*/annotations)
    failing annotations
    id=${path#*check-runs/}
    jq -r --argjson id "${id%/annotations}" '.[] | select(.id == $id) | .annotation // empty' "$FAKE/jobs.json";;
  'GET '*/runs/7/jobs*)            failing jobs; jq -c '.[]' "$FAKE/jobs.json";;
  'GET '*/runs/7)                  failing run; cat "$FAKE/run.json";;
  *)                               echo "the fake gh has no answer for: $method $path" >&2; exit 1;;
esac
GH
chmod +x "$tmp/bin/gh"

not_acquired='The job was not acquired by Runner of type hosted even after multiple attempts'
failed_to_be_acquired='The job was not started because it repeatedly failed to be acquired (5 attempts).'

# job <id> <conclusion> <step count> <runner> <name> [<annotation>]: one job of
# a jobs list.
job() {
  jq -n --argjson id "$1" --arg conclusion "$2" --argjson step_count "$3" --arg runner "$4" --arg name "$5" \
        --arg annotation "${6:-}" \
     '{id: $id, name: $name, conclusion: $conclusion,
       steps: [range(1; $step_count + 1) | tostring],
       runner_name: (if $runner == "" then null else $runner end),
       annotation: (if $annotation == "" then null else $annotation end)}'
}

lost_column=$({ job 10 success 3 r1 scope
                job 20 cancelled 0 '' 'coarse / macOS' "$not_acquired"
                job 21 success 32 r2 'coarse / Linux'
                job 30 failure 5 r3 'coarse / audit'
                job 40 failure 5 r4 'Every job ran'; } | jq -s .)
lost_all=$({ job 10 cancelled 0 '' scope "$not_acquired"
             job 20 cancelled 0 '' 'coarse / macOS' "$not_acquired"
             job 21 cancelled 0 '' 'coarse / Linux' "$not_acquired"
             job 30 cancelled 0 '' 'coarse / audit' "$not_acquired"
             job 40 failure 5 r1 'Every job ran'; } | jq -s .)
lost_scope_columns_ran=$({ job 10 cancelled 0 '' scope "$not_acquired"
                           job 20 failure 31 r1 'coarse / macOS'
                           job 21 failure 31 r2 'coarse / Linux'
                           job 30 cancelled 0 '' 'coarse / audit' "$not_acquired"
                           job 40 cancelled 0 '' 'Every job ran' "$not_acquired"; } | jq -s .)
lost_audit=$({ job 10 success 3 r1 scope
               job 20 success 28 r2 'coarse / macOS'
               job 30 failure 0 '' 'coarse / audit' "$failed_to_be_acquired"
               job 40 failure 5 r3 'Every job ran'; } | jq -s .)
second_attempt=$({ job 5 cancelled 0 '' 'coarse / macOS' "$not_acquired"
                   job 6 success 3 r1 scope
                   job 7 success 32 r2 'coarse / Linux'
                   job 30 cancelled 0 '' 'coarse / audit' "$not_acquired"; } | jq -s .)
real_failure=$({ job 10 success 3 r1 scope
                 job 20 failure 32 r2 'fine / Windows'
                 job 40 failure 5 r3 'Every job ran'; } | jq -s .)
not_lost=$({ job 10 success 3 r1 scope
             job 20 cancelled 12 r2 'cancelled with steps' "$not_acquired"
             job 21 failure 0 r3 'a runner, no steps' "$not_acquired"
             job 22 cancelled 3 '' 'steps, no runner' "$not_acquired"
             job 23 skipped 0 '' 'skipped' "$not_acquired"
             job 24 cancelled 0 '' 'no annotation'
             job 25 cancelled 0 '' 'another annotation' 'The operation was canceled.'
             job 40 failure 5 r4 'Every job ran'; } | jq -s .)
all_success=$({ job 10 success 3 r1 scope
                job 20 success 32 r2 'coarse / macOS'; } | jq -s .)
newline_name=$({ job 20 cancelled 0 '' $'coarse / macOS\n::error::forged' "$not_acquired"; } | jq -s .)

fail() { echo "FAIL: $1"; fails=$((fails+1)); }

# check <name> <yes|no> <pattern> <file>
check() {
  if grep -qE "$3" "$4" 2> /dev/null; then got=yes; else got=no; fi
  [ "$got" = "$2" ] || fail "$1 (expected $2, got $got, pattern: $3)"
}

# run <case> <jobs> [<attempt>]
# Runs the script in a directory of its own, $dir, serving <jobs> for attempt
# <attempt> of run 7, by default the first, a push to branch b, the newest of
# its workflow unless $newest says otherwise. Files placed in $dir beforehand
# stay. Sets $status, and $reruns to the paths re-run, one per line.
run() {
  dir=$cases/$1
  mkdir -p "$dir"
  echo "$2" > "$dir/jobs.json"
  echo '{"id": 7, "workflow_id": 3, "head_branch": "b", "event": "push"}' > "$dir/run.json"
  echo "${newest:-7}" > "$dir/newest"
  (cd "$dir" && FAKE=$dir PATH="$tmp/bin:$PATH" GH_REPO=owner/repo \
     bash "$cases/rerun_unacquired_job.sh" 7 "${3:-1}" > out.txt 2>&1)
  status=$?
  reruns=$(cat "$dir/reruns" 2> /dev/null)
}

# failing_run <case> <call> <jobs>: runs <case> with <call> failing.
failing_run() {
  mkdir -p "$cases/$1"
  touch "$cases/$1/$2_fails"
  run "$1" "$3"
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
  [ "$status" -eq 2 ] && [ ! -f "$dir/calls" ] \
    || fail "$description: status $status, calls: $(cat "$dir/calls" 2> /dev/null)"
  [ ! -s "$dir/out.txt" ] && grep -q '^Usage: ' "$dir/err.txt" \
    || fail "$description: the usage was not on standard error alone"
}

# controls <script>
# Runs the controls against <script>, beside a copy of read_settled_jobs.sh
# whose wait is shrunk to 4 s, and counts in $fails those that fail.
controls() {
  fails=0
  cases=$tmp/cases
  rm -rf "$cases"
  mkdir "$cases"
  cp "$1" "$cases/rerun_unacquired_job.sh"
  sed 's/^wait_limit=420 interval=10$/wait_limit=4 interval=1/' "$settled_jobs_reader" > "$cases/read_settled_jobs.sh"
  chmod +x "$cases/read_settled_jobs.sh"

  run lost_column "$lost_column"
  reran "a lost column" 20
  check "a lost column is named" yes "^  coarse / macOS$" "$dir/out.txt"
  check "the re-run is logged" yes "^Re-running coarse / macOS, and the jobs that need it\.$" "$dir/out.txt"
  check "the jobs are read through read_settled_jobs.sh" yes \
        "^api repos/owner/repo/actions/runs/7/jobs\?per_page=100 --paginate --jq \.jobs\[\]$" "$dir/calls"
  check "the newest run asked for is of the run's workflow, branch and event" yes \
        "^api -X GET repos/owner/repo/actions/workflows/3/runs -f branch=b -f event=push -F per_page=1 " "$dir/calls"

  run lost_all "$lost_all"
  reran "scope and every job after it lost" 10

  run lost_scope_columns_ran "$lost_scope_columns_ran"
  reran "scope lost, the columns run and failed" 10

  run lost_audit "$lost_audit"
  reran "a lost audit, in the other wording" 30

  run lost_all_reversed "$(jq 'reverse' <<< "$lost_all")"
  reran "scope and every job after it lost, in reverse order" 10

  run second_attempt "$second_attempt"
  reran "a second attempt's job re-run, lost again" 5

  run real_failure "$real_failure"
  reran_none "a column that ran and failed"
  check "no lost job is logged" yes "^No job of run 7 lacked a runner\.$" "$dir/out.txt"

  run not_lost "$not_lost"
  reran_none "jobs lacking one of the marks of a lost job"

  run all_success "$all_success"
  reran_none "every job succeeded"

  newest=8
  run superseded "$lost_column"
  unset newest
  reran_none "a superseded run"
  check "a superseded run is logged" yes "^Run 8 has superseded run 7, which is not re-run\.$" "$dir/out.txt"
  check "a superseded run's jobs are not read" no "/jobs" "$dir/calls"

  run newline_name "$newline_name"
  reran "a lost job whose name holds a newline" 20
  check "a name is logged on one line" yes "^  coarse / macOS ::error::forged$" "$dir/out.txt"
  check "a name forges no command" no "^::error::" "$dir/out.txt"

  run last_attempt "$lost_column" 4
  reran_none "a lost column at the last attempt allowed"
  check "the last attempt warns" yes "^::warning::Run 7 has had 4 attempts" "$dir/out.txt"

  run past_last_attempt "$lost_column" 5
  reran_none "a lost column past the last attempt allowed"

  run attempt_before_last "$lost_column" 3
  reran "a lost column at the attempt before the last" 20
  check "an attempt before the last does not warn" no "::warning::" "$dir/out.txt"

  failing_run run_fails run "$lost_column"
  [ "$status" -ne 0 ] && [ -z "$reruns" ] || fail "a failed read of the run: status $status, re-runs: ${reruns:-none}"

  failing_run jobs_fail jobs "$lost_column"
  [ "$status" -ne 0 ] && [ -z "$reruns" ] || fail "a failed read of the jobs: status $status, re-runs: ${reruns:-none}"

  failing_run annotations_fail annotations "$lost_column"
  [ "$status" -ne 0 ] && [ -z "$reruns" ] \
    || fail "a failed read of the annotations: status $status, re-runs: ${reruns:-none}"

  failing_run rerun_fails rerun "$lost_column"
  [ "$status" -ne 0 ] || fail "a failed re-run did not fail the script"
  check "a failed re-run is not logged as made" no "^Re-running" "$dir/out.txt"

  refused "one argument"            GH_REPO=owner/repo bash "$cases/rerun_unacquired_job.sh" 7
  refused "three arguments"         GH_REPO=owner/repo bash "$cases/rerun_unacquired_job.sh" 7 1 1
  refused "a run id not a number"   GH_REPO=owner/repo bash "$cases/rerun_unacquired_job.sh" seven 1
  refused "an attempt of 0"         GH_REPO=owner/repo bash "$cases/rerun_unacquired_job.sh" 7 0
  refused "a leading zero"          GH_REPO=owner/repo bash "$cases/rerun_unacquired_job.sh" 7 01
  refused "an unset GH_REPO"        -u GH_REPO bash "$cases/rerun_unacquired_job.sh" 7 1
  refused "an empty GH_REPO"        GH_REPO= bash "$cases/rerun_unacquired_job.sh" 7 1
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
  'a superseded run re-run'         'if [ "$newest" != "$run_id" ]; then'
                                    'if false; then'
  'the branch not asked'            ' -f branch="$branch"'             ''
  'the event not asked'             ' -f event="$event"'               ''
  'the jobs read once, unsettled'   '"$(dirname "$0")/read_settled_jobs.sh" "$run_id" "$jobs"'
                                    'gh api "repos/$GH_REPO/actions/runs/$run_id/jobs" --jq .jobs > "$jobs" #'
  'a skipped job lost'              '(.conclusion == "cancelled" or .conclusion == "failure")'
                                    '(.conclusion != "success")'
  'a failed job never lost'         ' or .conclusion == "failure")'    ')'
  'a job with steps lost'           $'\n             and (.steps | length) == 0' ''
  'a job with a runner lost'        $'\n             and (.runner_name // "") == ""' ''
  'the annotation ignored'          'if grep -qE'                      'if true || grep -qE'
  'the other wording missed'        '|failed to be acquired'           ''
  'a failed annotation read ignored' "--jq '.[].message')"             "--jq '.[].message' || :)"
  'the highest id re-run'           '  | sort_by(.id)'                 '  | sort_by(.id) | reverse'
  'the jobs order kept'             '  | sort_by(.id)'                 '  | .'
  'a newline kept in a name'        ' | gsub("[\r\n]"; " ")'           ''
  'no lost job reported as lost'    $'lacked a runner."\n  exit 0'     $'lacked a runner."\n  exit 1'
  'the attempts unbounded'          'if [ "$attempt" -ge "$max_attempts" ]; then'
                                    'if false; then'
  'one attempt fewer'               'if [ "$attempt" -ge "$max_attempts" ]; then'
                                    'if [ "$attempt" -ge "$((max_attempts - 1))" ]; then'
  'only the last attempt bounded'   'if [ "$attempt" -ge "$max_attempts" ]; then'
                                    'if [ "$attempt" -eq "$max_attempts" ]; then'
  'the warning not an annotation'   'echo "::warning::'                'echo "warning: '
  'the last attempt fails'          $'re-runs are made."\n  exit 0'    $'re-runs are made."\n  exit 1'
  'a failed re-run ignored'         '/rerun" > /dev/null'              '/rerun" > /dev/null || :'
  'the run re-run whole'            'actions/jobs/$job_id/rerun'       'actions/runs/$run_id/rerun-failed-jobs'
  'the lost jobs not named'         "printf '%s' \"\$lost\" | cut -f2 | sed 's/^/  /'"
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
