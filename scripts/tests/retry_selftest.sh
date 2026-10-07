#!/bin/bash
# Controls for retry.sh, and a mutation check of the controls.
#
#   retry_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls against the script. With
# it, the selftest runs them against the script and against each mutant of the
# script: the script must fail no control, and each mutant at least one.
#
# The command retried is a stand-in. It follows a plan of one behaviour per
# attempt, and records each run, its arguments and its standard input. Pauses
# are 1 s, or none, rather than the script's 15 s. Each control is a claim the
# script exists to keep:
#   - arguments of any other form are refused with status 2 and the usage,
#     and nothing runs;
#   - the first success ends the retries, with status 0 and nothing printed;
#   - a failed attempt is named with the whole command, its status and its
#     number, and followed by a pause of its number times the unit, except
#     after the last;
#   - when every attempt fails, an ::error:: names the whole command, and the
#     status is 1;
#   - a hung attempt is stopped at the limit, named as such, and retried,
#     with nothing printed on standard error;
#   - what the attempt started is stopped with it: a grandchild, a child
#     which ignores SIGTERM (killed 10 s later, even when its parent has
#     died), and, run through sudo, a grandchild running as root;
#   - the command's arguments arrive intact, and its standard input is empty;
#   - nothing outlives the script: no stand-in, no watcher, no flag directory;
#   - the pause unit is 15 s.
#
# The sudo control needs sudo without a password. On Linux, where CI runs the
# selftest and has it, its absence fails the selftest; elsewhere the control is
# reported as not run.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
original="$here/../retry.sh"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/retry_selftest.XXXXXX")
# The stand-ins hang in sleeps of 61 s, a length nothing else here uses, so
# that whatever a mutant leaves behind can be found and stopped.
clean_up() {
  pkill -f "$tmp/" 2> /dev/null
  pkill -f '^/bin/sleep 61$' 2> /dev/null
  pkill -f '^sleep 37$' 2> /dev/null
  sudo -n pkill -f '^/bin/sleep 61$' 2> /dev/null
}
trap 'clean_up; sudo -n rm -rf "$tmp" 2> /dev/null; rm -rf "$tmp"' EXIT

# Reads the clock in tenths of a second.
tenths() { perl -MTime::HiRes=time -e 'printf "%d\n", time * 10'; }

# The stand-in reads its plan from $tmp/plan, one behaviour per line, takes the
# line for this run, and records the run, its child or grandchild, and its
# standard input.
cat > "$tmp/standin" <<'STANDIN'
#!/bin/bash
dir=$(dirname "$0")
echo "$$ $*" >> "$dir/runs"
cat > "$dir/stdin.$$"
run=$(wc -l < "$dir/runs")
behaviour=$(sed -n "${run}p" "$dir/plan")
case "$behaviour" in
  succeed)      exit 0 ;;
  fail-*)       exit "${behaviour#fail-}" ;;
  hang)         exec /bin/sleep 61 ;;
  ignore-term)  trap '' TERM; /bin/sleep 61 & echo $! > "$dir/descendant"; wait $!; /bin/sleep 61 ;;
  child-ignores-term)
                bash -c 'trap "" TERM; exec /bin/sleep 61' & echo $! > "$dir/descendant"; wait $! ;;
  grandchild)   bash -c '/bin/sleep 61 & echo $! > "$1/descendant"; wait $!' _ "$dir" & wait $! ;;
esac
exit 99
STANDIN
chmod +x "$tmp/standin"

fails=0
fail() { echo "FAIL: $1"; fails=$((fails + 1)); }

# Runs the script under test with a plan, and records its output, status and
# duration: retry <plan> <arguments>...
retry() {
  local plan=$1
  shift
  printf '%s\n' $plan > "$tmp/plan"
  rm -f "$tmp/runs" "$tmp"/stdin.* "$tmp/descendant"
  local began
  began=$(tenths)
  TMPDIR="$tmp/temporary" RETRY_PAUSE_SECONDS=${pause-0} ${as_root-} bash "$script" "$@" \
    < /dev/null > "$tmp/out" 2> "$tmp/err"
  status=$?
  elapsed=$(( $(tenths) - began ))
  runs=0
  [ -f "$tmp/runs" ] && runs=$(wc -l < "$tmp/runs" | tr -d ' ')
}

expect() { # expect <claim> <actual> <expected>
  [ "$2" = "$3" ] || fail "$1 (expected '$3', got '$2')"
}

has() { # has <claim> <yes|no> <pattern> <file>
  local got=no
  grep -qE -- "$3" "$4" && got=yes
  [ "$got" = "$2" ] || fail "$1 (expected $2, pattern: $3)"
}

# Whether process <pid> has ended, allowing 3 s for it to be reaped: a killed
# process may linger as a zombie.
gone() {
  local polls=30
  while [ "$polls" -gt 0 ]; do
    case "$(ps -o stat= -p "$1" 2> /dev/null)" in
      ""|Z*) return 0 ;;
    esac
    sleep 0.1
    polls=$((polls - 1))
  done
  return 1
}

descendant_gone() { # descendant_gone <claim>
  if [ ! -s "$tmp/descendant" ]; then
    fail "$1 (the stand-in recorded no descendant)"
  elif ! gone "$(cat "$tmp/descendant")"; then
    fail "$1"
  fi
}

run_controls() {
  fails=0
  mkdir -p "$tmp/temporary"

  local arguments
  for arguments in "" "2 1 --" "2 1 x $tmp/standin" "0 1 -- $tmp/standin" "08 1 -- $tmp/standin" \
                   "abc 1 -- $tmp/standin" "-1 1 -- $tmp/standin" "2 0 -- $tmp/standin" "2 1.5 -- $tmp/standin"; do
    retry "succeed" $arguments
    expect "'$arguments' is refused with status 2" "$status" 2
    expect "'$arguments' runs nothing" "$runs" 0
    has "'$arguments' prints the usage on standard error" yes '^Usage: ' "$tmp/err"
    has "'$arguments' prints nothing on standard output" no '.' "$tmp/out"
  done

  retry "succeed" 3 5 -- "$tmp/standin"
  expect "a first success exits 0" "$status" 0
  expect "a first success runs once" "$runs" 1
  has "a first success prints nothing" no '.' "$tmp/out"

  pause=1 retry "fail-3 fail-3 succeed" 4 5 -- "$tmp/standin"
  expect "a success on the third attempt exits 0" "$status" 0
  expect "a success on the third attempt runs three times" "$runs" 3
  has "a failed attempt is named with its status and number" yes \
    "^$tmp/standin failed with status 3 \\(attempt 1 of 4\\)$" "$tmp/out"
  has "the second attempt is named" yes '\(attempt 2 of 4\)$' "$tmp/out"
  has "the first pause is one unit" yes '^Retrying in 1 s$' "$tmp/out"
  has "the second pause is two units" yes '^Retrying in 2 s$' "$tmp/out"
  [ "$elapsed" -ge 30 ] || fail "the pauses are taken (3 s expected, took $elapsed tenths)"
  has "no error is raised on a success" no '::error::' "$tmp/out"

  retry "fail-3 fail-3 fail-3 succeed" 3 5 -- "$tmp/standin"
  expect "every attempt failing exits 1" "$status" 1
  expect "every attempt is made, and no more" "$runs" 3
  expect "the last line is an error naming the command" "$(tail -n 1 "$tmp/out")" \
    "::error::$tmp/standin failed on all 3 attempts"
  expect "no pause follows the last attempt" "$(grep -c '^Retrying' "$tmp/out")" 2

  retry "fail-4" 1 5 -- "$tmp/standin" "two words" "" last
  expect "the command's arguments arrive intact" "$(cut -d' ' -f2- "$tmp/runs")" "two words  last"
  has "a failure names the whole command" yes \
    "^$tmp/standin two words  last failed with status 4 \\(attempt 1 of 1\\)$" "$tmp/out"
  expect "the error names the whole command" "$(tail -n 1 "$tmp/out")" \
    "::error::$tmp/standin two words  last failed on all 1 attempts"

  retry "hang succeed" 2 1 -- "$tmp/standin"
  expect "a hung attempt is retried, and the retry's success exits 0" "$status" 0
  expect "a hung attempt is retried once" "$runs" 2
  has "a hung attempt is named as such" yes \
    "^$tmp/standin did not finish within 1 s \\(attempt 1 of 2\\)$" "$tmp/out"
  [ "$elapsed" -lt 40 ] || fail "a hung attempt is stopped at its limit (took $elapsed tenths)"
  has "a stopped attempt prints nothing on standard error" no '.' "$tmp/err"
  gone "$(head -n 1 "$tmp/runs" | cut -d' ' -f1)" || fail "a hung attempt is stopped, not left running"

  retry "grandchild" 1 1 -- "$tmp/standin"
  expect "a stopped attempt with a grandchild fails the script" "$status" 1
  descendant_gone "the attempt's grandchild is stopped with it"

  retry "ignore-term" 1 1 -- "$tmp/standin"
  expect "an attempt ignoring SIGTERM fails the script" "$status" 1
  [ "$elapsed" -ge 100 ] && [ "$elapsed" -lt 160 ] \
    || fail "an attempt ignoring SIGTERM is killed 10 s after its limit (took $elapsed tenths)"
  gone "$(head -n 1 "$tmp/runs" | cut -d' ' -f1)" || fail "an attempt ignoring SIGTERM is killed, not left running"
  descendant_gone "the child of an attempt ignoring SIGTERM is killed with it"

  retry "child-ignores-term" 1 1 -- "$tmp/standin"
  [ "$elapsed" -ge 100 ] && [ "$elapsed" -lt 160 ] \
    || fail "a child ignoring SIGTERM is killed 10 s after the limit, its parent dead (took $elapsed tenths)"
  descendant_gone "a child ignoring SIGTERM is killed once its parent has died"

  rm -f "$tmp"/stdin.*
  printf '%s\n' succeed > "$tmp/plan"
  rm -f "$tmp/runs"
  echo something | TMPDIR="$tmp/temporary" RETRY_PAUSE_SECONDS=0 bash "$script" 1 5 -- "$tmp/standin" > /dev/null 2>&1
  local inputs
  inputs=$(ls "$tmp"/stdin.* 2> /dev/null)
  if [ "$(echo "$inputs" | grep -c .)" -ne 1 ]; then
    fail "the stand-in recorded its standard input once"
  elif [ -s "$inputs" ]; then
    fail "the command's standard input is empty"
  fi

  retry "succeed" 1 37 -- "$tmp/standin"
  pgrep -f '^sleep 37$' > /dev/null && fail "the watcher's sleep outlives the script"
  [ -z "$(ls -A "$tmp/temporary")" ] || fail "the flag directory outlives the script"

  grep -qF 'pause=${RETRY_PAUSE_SECONDS:-15}' "$script" || fail "the pause unit is 15 s"

  if sudo -n true 2> /dev/null; then
    as_root="sudo -n env" retry "grandchild" 1 1 -- "$tmp/standin"
    expect "a stopped attempt run as root fails the script" "$status" 1
    descendant_gone "run as root, the attempt's root grandchild is stopped with it"
  elif [ "$(uname -s)" = Linux ]; then
    fail "the sudo control needs sudo without a password, which CI's Linux runners have"
  else
    echo "Not run here: the sudo control, which needs sudo without a password"
  fi
  return "$fails"
}

# Each mutant is a description, the text to replace, and its replacement.
mutants=(
  'arguments not checked'          '[ $# -ge 4 ] && [ "$3" = -- ] || usage'  ':'
  'counts not checked'             'positive "$attempts" && positive "$limit" || usage'  ':'
  'a leading zero accepted'        '^[1-9][0-9]*$'  '^[0-9]+$'
  'usage on standard output'       'echo "Usage: $0 <attempts> <seconds> -- <command>..." >&2'  'echo "Usage: $0 <attempts> <seconds> -- <command>..."'
  'one attempt too few'            'attempt <= attempts'  'attempt < attempts'
  'a success not the end'          '[ "$status" -ne 0 ] || exit 0'  ':'
  'no watcher'                     '    kill -TERM -- "-$command" 2> /dev/null'  '    :'
  'no SIGKILL'                     '    kill -KILL -- "-$command" 2> /dev/null'  '    :'
  'the command alone killed'       'kill -KILL -- "-$command"'  'kill -KILL "$command"'
  'the leader alone stopped'       'kill -TERM -- "-$command"'  'kill -TERM "$command"'
  'the grace skipped'              '    wait "$watcher" 2> /dev/null
    echo "$* did not finish'  '    kill "$watcher" 2> /dev/null
    echo "$* did not finish'
  'the whole grace always waited'  '|| exit 0'  '|| :'
  'a stop not named'               'if [ -e "$flag/stopped" ]; then'  'if false; then'
  'a pause after the last attempt' 'if [ "$attempt" -lt "$attempts" ]; then'  'if true; then'
  'the pause not lengthened'       'sleep $((attempt * pause))'  'sleep $pause'
  'no error annotation'            'echo "::error::$* failed on all $attempts attempts"'  'echo "$* failed on all $attempts attempts"'
  'only the first word named'      '"::error::$* failed'  '"::error::$1 failed'
  'success when all fail'          'exit 1'  'exit 0'
  'the watcher outlives'           "trap 'kill \"\$sleeper\" 2> /dev/null; exit 0' TERM"  "trap 'exit 0' TERM"
  'the flag directory kept'        "trap 'rm -rf \"\$flag\"' EXIT"  ':'
  'arguments joined'               '  "$@" < /dev/null &'  '  $* < /dev/null &'
  'standard input inherited'       '  "$@" < /dev/null &'  '  "$@" &'
  'job control left on'            '  set +m
'  '  :
'
  'no process group'               '  set -m
  "$@"'  '  :
  "$@"'
  'another pause unit'             'pause=${RETRY_PAUSE_SECONDS:-15}'  'pause=${RETRY_PAUSE_SECONDS:-5}'
  'another grace'                  'grace=10'  'grace=3'
)

mutations() {
  local i survivors=0 content stripped occurrences failed
  script=$original
  run_controls > "$tmp/controls.log" 2>&1
  echo "unmutated: $(grep -c '^FAIL' "$tmp/controls.log") controls fail"
  grep -q '^FAIL' "$tmp/controls.log" && { cat "$tmp/controls.log"; survivors=1; }
  for ((i = 0; i < ${#mutants[@]}; i += 3)); do
    content=$(cat "$original"; echo x)
    content=${content%x}
    stripped=${content//"${mutants[i+1]}"/}
    occurrences=$(( (${#content} - ${#stripped}) / ${#mutants[i+1]} ))
    if [ "$occurrences" -ne 1 ]; then
      echo "${mutants[i]}: the text to mutate occurs $occurrences times  <-- must be once"
      survivors=1
      continue
    fi
    printf '%s' "${content%%"${mutants[i+1]}"*}${mutants[i+2]}${content#*"${mutants[i+1]}"}" > "$tmp/mutant.sh"
    script="$tmp/mutant.sh"
    run_controls > "$tmp/controls.log" 2>&1
    failed=$(grep -c '^FAIL' "$tmp/controls.log")
    if [ "$failed" -gt 0 ]; then
      echo "${mutants[i]}: $failed controls fail"
    else
      echo "${mutants[i]}: SURVIVED"
      survivors=1
    fi
    clean_up
  done
  return "$survivors"
}

case "${1-}" in
  --mutations) mutations; exit ;;
  "")          ;;
  *)           echo "Usage: $0 [--mutations]" >&2; exit 2 ;;
esac

script=$original
run_controls
if [ "$fails" -eq 0 ]; then echo "retry: all controls pass"; else exit 1; fi
