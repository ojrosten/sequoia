#!/bin/bash
# Controls for fetch_first_parent_record.sh, against a remote built here and
# a stand-in for gh, and a mutation check of the controls.
#
#   fetch_first_parent_record_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the script and against each mutant of the script.
# The script must fail no control, and each mutant at least one.
#
# The remote holds a branch `staging`, whose first-parent history is
# root - s1 - tip, with `tip` merging a branch `side` from `s1`, and the data
# branch `metrics`, to which each control gives the records it needs, each
# record naming its commit. Beside them are merges as GitHub makes them for a
# pull request, each merging a branch `topic` into a commit: into `tip`; into
# `s1`, before staging moved on; and into `side`. The stand-in for gh answers
# `gh run list` from a file which each control writes, logs its arguments,
# and fails once called more often than a control allows, so that no mutant
# loops for ever. Each control is a claim the script exists to keep:
#   - the record printed is that of the commit's first parent, not of the
#     commit, of its second parent or of the branch's tip;
#   - while a run of the workflow for the first parent, started by a push, is
#     unfinished, the script fetches the data branch again, and prints the
#     record once the branch holds it; it asks gh for exactly those runs;
#   - each of these fails with status 1, saying why and printing nothing: a
#     commit the clone lacks; a commit with no parent; a first parent off the
#     branch's first-parent history; no record, and no run unfinished,
#     whether there are no runs or only finished ones, which the failure
#     names; a run still unfinished when the time allowed is spent, which the
#     failure names; no data branch and no run; a branch which cannot be
#     fetched; a gh which fails, or lists the runs as other than JSON;
#   - arguments of any other form are refused with status 2.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script=$here/../fetch_first_parent_record.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

case "$*" in
  ''|--mutations) ;;
  *) echo "Usage: $0 [--mutations]" >&2; exit 2;;
esac

export GIT_AUTHOR_NAME=selftest GIT_AUTHOR_EMAIL=selftest@example.invalid
export GIT_COMMITTER_NAME=selftest GIT_COMMITTER_EMAIL=selftest@example.invalid
export GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_NOSYSTEM=1

g() { git -C "$tmp/work" "$@"; }
git init -q --bare -b staging "$tmp/remote.git"
git init -q -b staging "$tmp/work"
g commit -q --allow-empty -m root; root=$(g rev-parse HEAD)
g commit -q --allow-empty -m s1;   s1=$(g rev-parse HEAD)
g checkout -q -b side
g commit -q --allow-empty -m side; side=$(g rev-parse HEAD)
g checkout -q -b topic "$root"
g commit -q --allow-empty -m topic; topic=$(g rev-parse HEAD)
g checkout -q staging
g merge -q --no-ff --no-edit side; tip=$(g rev-parse HEAD)
# pull_request_merge <name> <commit>: a merge of topic into the commit, with
# the commit as its first parent, in $<name>
pull_request_merge() {
  g checkout -q --detach "$2"
  g merge -q --no-ff --no-edit -m "$1" "$topic"
  printf -v "$1" '%s' "$(g rev-parse HEAD)"
}
pull_request_merge into_tip  "$tip"
pull_request_merge into_s1   "$s1"
pull_request_merge into_side "$side"
g checkout -q staging
g push -q "$tmp/remote.git" staging

# store <commit>...: a metrics branch on the remote holding a record of each
# commit
store() {
  local data=$tmp/data commit
  rm -rf "$data"; git init -q -b metrics "$data"
  mkdir -p "$data/records"
  for commit in "$@"; do echo "{\"commit\": \"$commit\"}" > "$data/records/$commit.json"; done
  git -C "$data" add -A; git -C "$data" commit -q --allow-empty -m records
  git -C "$data" push -q -f "$tmp/remote.git" metrics
}
unstore() { git -C "$tmp/work" push -q "$tmp/remote.git" :metrics 2> /dev/null || true; }

# The clone the script runs in, with the remote as origin.
git clone -q "$tmp/remote.git" "$tmp/clone"
git -C "$tmp/clone" fetch -q "$tmp/work" "$into_tip" "$into_s1" "$into_side"

# The stand-in for gh. It logs its arguments to $tmp/gh.log, one call to a
# line. Its answer is $tmp/runs.json, or, after $GH_SWITCH_AFTER calls,
# $tmp/runs_later.json, when it also runs $tmp/on_switch.sh. It fails on call
# number $GH_CALLS_ALLOWED + 1, and at once if $tmp/gh_fails exists.
mkdir "$tmp/bin"
cat > "$tmp/bin/gh" <<EOF
#!/bin/bash
echo "\$*" >> "$tmp/gh.log"
calls=\$(wc -l < "$tmp/gh.log")
[ ! -e "$tmp/gh_fails" ] || { echo "gh: HTTP 502" >&2; exit 1; }
[ "\$calls" -le "\${GH_CALLS_ALLOWED:-5}" ] || { echo "gh: called too often" >&2; exit 1; }
if [ "\$calls" -gt "\${GH_SWITCH_AFTER:-1000}" ]; then
  [ ! -e "$tmp/on_switch.sh" ] || bash "$tmp/on_switch.sh"
  cat "$tmp/runs_later.json"
else
  cat "$tmp/runs.json"
fi
EOF
chmod +x "$tmp/bin/gh"
export PATH="$tmp/bin:$PATH" FETCH_POLL_SECONDS=0 GH_REPO=owner/repo

running='[{"databaseId": 7, "status": "in_progress", "conclusion": "", "url": "https://example.invalid/runs/7"}]'
failed='[{"databaseId": 6, "status": "completed", "conclusion": "failure", "url": "https://example.invalid/runs/6"}]'

fail() { echo "FAIL: $1"; fails=$((fails+1)); }

# invoke <argument>...: the script, run in the clone with the arguments;
# sets $status, $out and $err
invoke() {
  (cd "$tmp/clone" && bash "$under_test" "$@") > "$tmp/out" 2> "$tmp/err"
  status=$?
  out=$(cat "$tmp/out")
  err=$(cat "$tmp/err")
}

# run <commit> [<seconds>]: the script for the commit, with $runs as gh's
# answer
run() {
  rm -f "$tmp/gh_fails" "$tmp/on_switch.sh"
  : > "$tmp/gh.log"
  echo "$runs" > "$tmp/runs.json"
  invoke origin staging metrics records workflow.yml "$1" "${2:-0}"
}

# prints <description> <record's commit>
prints() {
  [ "$status" -eq 0 ] && [ "$(jq -r .commit <<< "$out")" = "$2" ] \
    || fail "$1: status $status, printed '$out', said '$err'"
}

# fails_with <description> <expected text>: status 1, the text on standard
# error, and nothing printed
fails_with() {
  [ "$status" -eq 1 ] && [ -z "$out" ] && [[ $err == *"$2"* ]] \
    || fail "$1: status $status, printed '$out', said '$err'"
}

# controls <script>
# Runs the controls against <script>, and counts in $fails those that fail.
controls() {
  fails=0
  under_test=$1

  store "$root" "$s1" "$tip" "$topic"
  runs='[]'
  run "$into_tip"; prints "a merge into the branch's tip" "$tip"
  run "$into_s1";  prints "a merge into a commit the branch has moved on from" "$s1"

  run "$into_side"
  fails_with "a first parent off the first-parent history" "is not on the first-parent history of staging"
  run "$root"
  fails_with "a commit with no parent" "is not a commit of this clone, or has no parent"
  run 0123456789abcdef0123456789abcdef01234567
  fails_with "a commit the clone lacks" "is not a commit of this clone, or has no parent"

  store "$root" "$s1" "$topic"
  run "$into_tip"
  fails_with "no record, and no runs" "no run of workflow.yml for it is unfinished. Its runs: none"
  grep -qxF "run list --workflow workflow.yml --commit $tip --event push --limit 100 --json databaseId,status,conclusion,url" \
    "$tmp/gh.log" || fail "gh is not asked for the runs which a push started for the first parent: $(cat "$tmp/gh.log")"
  runs=$failed run "$into_tip"
  fails_with "no record, and a finished run" "https://example.invalid/runs/6: failure"

  # The record is stored when gh is first asked after two answers of a run
  # unfinished, and that answer says the run has finished.
  : > "$tmp/gh.log"; rm -f "$tmp/gh_fails"
  echo "$running" > "$tmp/runs.json"; echo "$failed" > "$tmp/runs_later.json"
  cat > "$tmp/on_switch.sh" <<EOF
$(declare -f store)
tmp=$(printf '%q' "$tmp")
store $(printf '%q ' "$root" "$s1" "$tip" "$topic") > /dev/null 2>&1
EOF
  GH_SWITCH_AFTER=2 invoke origin staging metrics records workflow.yml "$into_tip" 60
  prints "a record stored while waiting" "$tip"
  [ "$(wc -l < "$tmp/gh.log")" -eq 3 ] || fail "a record stored while waiting: gh was asked $(wc -l < "$tmp/gh.log") times, not 3"

  store "$root" "$s1" "$topic"
  runs=$running run "$into_tip" 0
  fails_with "a run unfinished when the time is spent" \
             "after 0 seconds, and a run which may store it is unfinished: https://example.invalid/runs/7"

  unstore
  runs='[]' run "$into_tip"
  fails_with "no data branch, and no runs" "no run of workflow.yml for it is unfinished"

  store "$root" "$s1" "$tip"
  runs='not json' run "$into_tip"
  fails_with "gh listing the runs as other than JSON" "as other than JSON"
  : > "$tmp/gh.log"; touch "$tmp/gh_fails"
  invoke origin staging metrics records workflow.yml "$into_tip" 0
  fails_with "a gh which fails" "gh could not list the runs of workflow.yml for $tip"
  rm -f "$tmp/gh_fails"

  invoke origin no-such-branch metrics records workflow.yml "$into_tip" 0
  fails_with "a branch which cannot be fetched" "git fetch origin no-such-branch failed"

  local arguments
  for arguments in "origin staging metrics records workflow.yml $into_tip" \
                   "origin staging metrics records workflow.yml $into_tip 0 extra" \
                   "origin staging metrics records workflow.yml $into_tip soon" \
                   "origin staging metrics records workflow.yml $into_tip 060"; do
    # shellcheck disable=SC2086
    invoke $arguments
    [ "$status" -eq 2 ] && [ -z "$out" ] && [[ $err == Usage:* ]] || fail "the arguments '$arguments': status $status"
  done
}

# Each mutant breaks one behaviour that the controls claim. Its entry holds a
# description, the text it replaces, and the replacement. The order of a
# pass, gh before the data branch, is witnessed by "a record stored while
# waiting": asking gh after the fetch fails it, as the controls of
# 2026-10-30 showed, but the reordering is two edits, which no entry here can
# make.
mutations=(
  'the commit taken for its parent'    'parent=$(git rev-parse --verify --quiet "$commit^1^{commit}")' 'parent=$(git rev-parse --verify --quiet "$commit^{commit}")'
  'the second parent taken'            'parent=$(git rev-parse --verify --quiet "$commit^1^{commit}")' 'parent=$(git rev-parse --verify --quiet "$commit^2^{commit}")'
  'the tip taken for the parent'       'path="$directory/$parent.json"' 'path="$directory/$tip.json"'
  'the first-parent history not checked' 'git rev-list --first-parent "$tip" | grep -xF "$parent" > /dev/null \' 'true \'
  'the whole history checked'          'git rev-list --first-parent "$tip"' 'git rev-list "$tip"'
  'runs asked for by the commit'       '--commit "$parent"' '--commit "$commit"'
  'runs of any event'                  ' --event push' ''
  'finished runs awaited'              'select(.status != "completed")' 'select(.status != "")'
  'unfinished runs not awaited'        'select(.status != "completed")' 'select(.status == "")'
  'no wait'                            'if [ "$SECONDS" -ge "$deadline" ]; then' 'if true; then'
  'no deadline'                        'if [ "$SECONDS" -ge "$deadline" ]; then' 'if false; then'
  'a missing data branch fails'        '    2) ;;' '    2) die "no data branch";;'
  'a failing gh read as no runs'       '|| die "gh could not list the runs of $workflow for $parent"' '|| runs="[]"'
  'runs which are not JSON read as none' '|| die "gh listed the runs of $workflow for $parent as other than JSON: $runs"' '|| unfinished=""'
  'a failed fetch of the branch passes' '|| die "git fetch $remote $branch failed"' '|| true'
  'the finished runs not named'        '"  \(.url): \(.conclusion)"' '""'
  'the record printed and the script goes on' '         exit 0
       fi;;' '       fi;;'
  'any number of arguments'            '[ $# -ne 7 ]' '[ $# -lt 7 ]'
  'any wait accepted'                  '[[ ! $7 =~ ^(0|[1-9][0-9]*)$ ]]' 'false'
  'a refusal exits 1'                  '  exit 2
fi' '  exit 1
fi'
)

if [ "$*" != --mutations ]; then
  controls "$script"
  if [ "$fails" -eq 0 ]; then echo "fetch_first_parent_record: all controls pass"; else exit 1; fi
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
