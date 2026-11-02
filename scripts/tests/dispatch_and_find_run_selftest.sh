#!/bin/bash
# Controls for dispatch_and_find_run.sh, against a fake gh which records the
# dispatch and serves canned lists of runs, and a mutation check of the
# controls.
#
#   dispatch_and_find_run_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the script and against each mutant of the script.
# The script must fail no control, and each mutant at least one.
#
# The fake sleep takes no time. Each control is a claim the script exists to
# keep:
#   - the dispatch names the workflow and the ref, and passes the options
#     after them on whole, in order;
#   - each list asks for the dispatched runs of that workflow on that ref,
#     created no earlier than the dispatch;
#   - of the runs listed, the oldest is written to standard output, and
#     nothing else is;
#   - an empty list is followed by a pause of 10 s and another list, and the
#     first list naming a run ends the search;
#   - twelve empty lists fail the script with an error, and nothing on
#     standard output;
#   - a dispatch which fails fails the script, with no list;
#   - fewer than two arguments, an empty one, and an unset or empty $GH_REPO
#     are refused before the dispatch, with the usage on standard error.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script=$here/../dispatch_and_find_run.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

case "$*" in
  ''|--mutations) ;;
  *) echo "Usage: $0 [--mutations]" >&2; exit 2;;
esac

# The fake gh appends each call's arguments, one per line, then a line `--`,
# to $FAKE/calls. `gh workflow run` fails if $FAKE/dispatch_fails exists, and
# otherwise writes a line to standard output, as gh does. The
# n-th `gh run list` prints $FAKE/lists/<n>, or nothing past the last, after
# applying its --jq filter to it. The fake sleep records each pause.
mkdir "$tmp/bin"
cat > "$tmp/bin/gh" <<'GH'
#!/bin/bash
printf '%s\n' "$@" -- >> "$FAKE/calls"
case "$1 $2" in
  "workflow run")
    [ ! -e "$FAKE/dispatch_fails" ] || { echo 'HTTP 422' >&2; exit 1; }
    echo "Created workflow_dispatch event for $3 at $5";;
  "run list")
    n=$(( $(cat "$FAKE/lists_made" 2> /dev/null || echo 0) + 1 ))
    echo "$n" > "$FAKE/lists_made"
    list=$FAKE/lists/$n
    [ -f "$list" ] || list=$FAKE/empty
    while [ $# -gt 0 ] && [ "$1" != --jq ]; do shift; done
    jq -r "$2" "$list";;
esac
GH
cat > "$tmp/bin/sleep" <<'SLEEP'
#!/bin/bash
echo "$*" >> "$FAKE/pauses"
SLEEP
chmod +x "$tmp/bin/gh" "$tmp/bin/sleep"

fail() { echo "FAIL: $1"; fails=$((fails+1)); }

# run <case> <list>... -- <argument>...
# Runs the script in a directory of its own, $dir, serving the lists in order.
# Sets $status, $out and $err.
run() {
  dir=$tmp/cases/$1
  shift
  rm -rf "$dir"
  mkdir -p "$dir/lists"
  echo '[]' > "$dir/empty"
  local n=1
  while [ "$1" != -- ]; do echo "$1" > "$dir/lists/$n"; n=$((n+1)); shift; done
  shift
  out=$(cd "$dir" && FAKE=$dir PATH="$tmp/bin:$PATH" GH_REPO=${repo-owner/repo} bash "$subject" "$@" 2> "$dir/err")
  status=$?
  err=$(cat "$dir/err")
}

# calls <n>: the arguments of the n-th call, one per line.
calls() { awk -v n="$1" '$0 == "--" {i++; next} i == n - 1' "$dir/calls"; }
count_calls() { grep -cx -- -- "$dir/calls" 2> /dev/null || echo 0; }

controls() {
  subject=$1 fails=0

  run found '[{"databaseId": 3}, {"databaseId": 2}]' -- flow.yml topic -f a=b -f 'c=d e'
  [ "$status" -eq 0 ] && [ "$out" = 2 ] || fail "the oldest run listed: status $status, output '$out'"
  [ "$(calls 1)" = "$(printf '%s\n' workflow run flow.yml --ref topic -f a=b -f 'c=d e')" ] \
    || fail "the dispatch: '$(calls 1)'"
  listed=$(calls 2)
  [ "$(head -8 <<<"$listed" | tr '\n' ' ')" = 'run list --workflow flow.yml --branch topic --event workflow_dispatch ' ] \
    || fail "the list asked for: '$(tr '\n' ' ' <<<"$listed")'"
  grep -qxE -- '>=[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9:]{8}\+00:00' <<<"$listed" || fail "the list's --created: '$listed'"
  [ "$(cat "$dir/pauses")" = 10 ] || fail "the pauses before a run is found: '$(cat "$dir/pauses")'"

  run later '[]' '[]' '[{"databaseId": 7}]' '[{"databaseId": 8}]' -- flow.yml topic
  [ "$status" -eq 0 ] && [ "$out" = 7 ] || fail "the first list naming a run: status $status, output '$out'"
  [ "$(count_calls)" -eq 4 ] || fail "the search went on after a run was found: $(count_calls) calls"
  [ "$(cat "$dir/pauses")" = $'10\n10\n10' ] || fail "a pause of 10 s before each list: '$(cat "$dir/pauses")'"

  run never -- flow.yml topic
  [ "$status" -ne 0 ] && [ -z "$out" ] || fail "no run: status $status, output '$out'"
  [ "$(count_calls)" -eq 13 ] || fail "no run: $(count_calls) calls, not a dispatch and twelve lists"
  [[ $err == *'::error::'* ]] || fail "no run: no error: '$err'"

  dir=$tmp/cases/dispatch_fails; rm -rf "$dir"; mkdir -p "$dir/lists"; touch "$dir/dispatch_fails"; echo '[]' > "$dir/empty"
  echo '[{"databaseId": 1}]' > "$dir/lists/1"
  out=$(cd "$dir" && FAKE=$dir PATH="$tmp/bin:$PATH" GH_REPO=owner/repo bash "$subject" flow.yml topic 2> /dev/null)
  status=$?
  [ "$status" -ne 0 ] && [ -z "$out" ] && [ "$(count_calls)" -eq 1 ] \
    || fail "a failed dispatch: status $status, output '$out', $(count_calls) calls"

  for arguments in "flow.yml" "'' topic" "flow.yml ''"; do
    eval "run refused -- $arguments"
    [ "$status" -eq 2 ] && [ -z "$out" ] && [[ $err == Usage:* ]] && [ ! -e "$dir/calls" ] \
      || fail "arguments $arguments: status $status, err '$err'"
  done
  repo='' run refused -- flow.yml topic
  [ "$status" -eq 2 ] && [ ! -e "$dir/calls" ] || fail "an empty GH_REPO: status $status"
  unset repo
}

# Each mutant breaks one behaviour that the controls claim. Its entry holds a
# description, the text it replaces, and the replacement.
mutations=(
  'the newest run taken'         "--jq 'last.databaseId // empty')" "--jq 'first.databaseId // empty')"
  'the options dropped'          'gh workflow run "$workflow" --ref "$ref" "$@" >&2' 'gh workflow run "$workflow" --ref "$ref" >&2'
  'the options split'            'gh workflow run "$workflow" --ref "$ref" "$@" >&2' 'gh workflow run "$workflow" --ref "$ref" $@ >&2'
  'any event listed'             ' --event workflow_dispatch' ''
  'any branch listed'            ' --branch "$ref"' ''
  'runs before the dispatch'     ' --created ">=$before"' ''
  'the search goes on'           '  [ -n "$id" ] && break' '  :'
  'no pause'                     '  sleep 10' '  :'
  'six lists'                    'for _ in $(seq 1 12); do' 'for _ in $(seq 1 6); do'
  'no run passes'                '[ -n "$id" ] || { echo "::error::the dispatched run of $workflow on $ref never appeared" >&2; exit 1; }' ':'
  'the dispatch on stdout'       'gh workflow run "$workflow" --ref "$ref" "$@" >&2' 'gh workflow run "$workflow" --ref "$ref" "$@"'
  'an empty argument taken'      '[ -z "$1" ] || [ -z "$2" ] ||' ''
  'GH_REPO not needed'           ' || [ -z "${GH_REPO:-}" ]; then' '; then'
)

if [ "$*" != --mutations ]; then
  controls "$script"
  if [ "$fails" -eq 0 ]; then echo "dispatch_and_find_run: all controls pass"; else exit 1; fi
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
