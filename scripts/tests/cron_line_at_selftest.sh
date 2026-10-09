#!/bin/bash
# Controls for cron_line_at.sh, against repositories built here with the times
# of their commits set, and a mutation check of the controls.
#
#   cron_line_at_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the script and against each mutant of the script.
# The script must fail no control, and each mutant at least one.
#
# The history, in seconds after a base time B, on the first-parent chain of
# main:
#   0    the root, with no workflow;
#   100  wf.yml added, scheduled A;
#   200  wf.yml rescheduled B;
#   500  the merge of a branch whose one commit, at 300, rescheduled it C;
#   600  two.yml added with two cron lines, and none.yml with none.
# Each control is a claim the script exists to keep:
#   - the schedule at a time is that of the newest commit on the first-parent
#     chain not after it: the commit at 300 was not in force until the merge
#     at 500, which a rule reading commits' own times would miss;
#   - a commit at the time itself is in force, and one a second after is not;
#   - a file absent at the time, or present with no cron line, prints nothing,
#     and succeeds;
#   - a commented-out cron line is not a schedule;
#   - the path is read from the top of the repository, whatever the directory;
#   - a blobless clone reads the file it needs from its remote;
#   - each fact the script cannot establish is an error, with status 1 and an
#     ::error:: saying which, and nothing on standard output: a time before
#     the root, a clone too shallow to reach the time, a repository with no
#     commits, a directory outside any repository, a tree or a file the clone
#     cannot fetch, a file with two cron lines;
#   - arguments of any other form are refused with status 2 and the usage on
#     standard error alone.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script=$here/../cron_line_at.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

case "$*" in
  ''|--mutations) ;;
  *) echo "Usage: $0 [--mutations]" >&2; exit 2;;
esac

# Run from a git hook, git exports GIT_DIR, which would override the
# repositories built here.
unset GIT_DIR GIT_WORK_TREE GIT_INDEX_FILE GIT_COMMON_DIR

B=1790000000
A_cron='1 2 * * *' B_cron='5 6 * * *' C_cron='9 10 * * 0'

# workflow <cron expression>: a workflow file's text, with a commented-out
# cron line above the real one.
workflow() {
  printf '%s\n' 'on:' '  schedule:' "  # - cron: '0 0 * * *'" "    - cron: '$1'"
}

# commit_at <seconds after B> <message>, in the current directory.
commit_at() {
  GIT_AUTHOR_DATE="@$((B + $1)) +0000" GIT_COMMITTER_DATE="@$((B + $1)) +0000" \
    git commit -q --allow-empty -m "$2"
}

configure() {
  git config user.name selftest
  git config user.email selftest@example.com
  git config maintenance.auto false
  git config gc.auto 0
}

repo=$tmp/repo
mkdir -p "$repo/.github/workflows" "$repo/sub"
(
  cd "$repo" || exit 1
  git init -q -b main
  configure
  git config uploadpack.allowFilter true
  echo readme > README && git add README && commit_at 0 root
  workflow "$A_cron" > .github/workflows/wf.yml && git add .github && commit_at 100 'add wf, A'
  workflow "$B_cron" > .github/workflows/wf.yml && git add .github && commit_at 200 'wf to B'
  git checkout -q -b side
  workflow "$C_cron" > .github/workflows/wf.yml && git add .github && commit_at 300 'wf to C'
  git checkout -q main
  echo more >> README && git add README && commit_at 400 'unrelated'
  GIT_AUTHOR_DATE="@$((B + 500)) +0000" GIT_COMMITTER_DATE="@$((B + 500)) +0000" \
    git merge -q --no-ff -m 'merge side' side
  { workflow "$A_cron"; echo "    - cron: '$B_cron'"; } > .github/workflows/two.yml
  printf '%s\n' 'on:' '  push:' > .github/workflows/none.yml
  git add .github && commit_at 600 'add two and none'
  touch sub/.keep
) > "$tmp/setup.txt" 2>&1 || { cat "$tmp/setup.txt"; echo "FAIL: the fixture could not be built"; exit 1; }

git clone -q --depth 1 "file://$repo" "$tmp/shallow" 2> /dev/null
git clone -q --filter=blob:none --no-checkout "file://$repo" "$tmp/blobless" 2> /dev/null
git clone -q --filter=blob:none --no-checkout "file://$repo" "$tmp/blobless-cut-off" 2> /dev/null
git clone -q --filter=tree:0 --no-checkout "file://$repo" "$tmp/treeless-cut-off" 2> /dev/null
git -C "$tmp/blobless-cut-off" remote set-url origin "file://$tmp/nowhere"
git -C "$tmp/treeless-cut-off" remote set-url origin "file://$tmp/nowhere"
mkdir "$tmp/empty" "$tmp/outside"
git -C "$tmp/empty" init -q -b main

fail() { echo "FAIL: $1"; fails=$((fails+1)); }

# run <directory> <argument>...
# Runs the script under test in <directory>, setting $status, $out and $err.
# Above $tmp, git looks for no repository, so $tmp/outside is outside any.
run() {
  local dir=$1
  shift
  (cd "$dir" && GIT_CEILING_DIRECTORIES=$tmp bash "$under_test" "$@" > "$tmp/out" 2> "$tmp/err")
  status=$?
  out=$(cat "$tmp/out")
}

# reads <description> <directory> <file> <seconds after B> <expected>
# The script succeeds, printing <expected> and a newline, or nothing at all.
reads() {
  run "$2" "$3" "$((B + $4))"
  local expected=
  [ -z "$5" ] || expected=$5$'\n'
  if [ "$status" -ne 0 ] || [ "$(cat "$tmp/out"; echo .)" != "$expected." ] || [ -s "$tmp/err" ]; then
    fail "$1: status $status, printed '$out', expected '$5'"
  fi
}

# refused_as_error <description> <expected message> <directory> <argument>...
# The script fails with status 1 and an ::error:: carrying the message, and
# prints nothing else.
refused_as_error() {
  local description=$1 message=$2
  shift 2
  run "$@"
  if [ "$status" -ne 1 ] || [[ $out != ::error::*"$message"* ]] || [ "$(grep -c . "$tmp/out")" -ne 1 ]; then
    fail "$description: status $status, printed '$out'"
  fi
}

# refused <description> <argument>...
# The script refuses with status 2, the usage on standard error alone.
refused() {
  local description=$1
  shift
  run "$repo" "$@"
  [ "$status" -eq 2 ] && [ ! -s "$tmp/out" ] && grep -q '^Usage: ' "$tmp/err" \
    || fail "$description: status $status, printed '$out'"
}

# controls <script>
# Runs the controls against <script>, and counts in $fails those that fail.
controls() {
  fails=0
  under_test=$1
  local wf=.github/workflows/wf.yml

  reads "absent before it was added"             "$repo" "$wf"  99 ''
  reads "in force at the second it was added"    "$repo" "$wf" 100 "$A_cron"
  reads "rescheduled"                            "$repo" "$wf" 250 "$B_cron"
  reads "a branch's commit before its merge"     "$repo" "$wf" 350 "$B_cron"
  reads "a second before the merge"              "$repo" "$wf" 499 "$B_cron"
  reads "at the merge"                           "$repo" "$wf" 500 "$C_cron"
  reads "now"                                    "$repo" "$wf" 9999 "$C_cron"
  reads "no cron line"                           "$repo" .github/workflows/none.yml 9999 ''
  reads "a file the history never held"          "$repo" .github/workflows/other.yml 9999 ''
  reads "from a subdirectory"                    "$repo/sub" "$wf" 250 "$B_cron"
  reads "a shallow clone, at its tip"            "$tmp/shallow" "$wf" 9999 "$C_cron"
  reads "a blobless clone fetches the file"      "$tmp/blobless" "$wf" 250 "$B_cron"

  refused_as_error "two cron lines"        "carries 2 cron lines" "$repo" .github/workflows/two.yml "$((B + 9999))"
  refused_as_error "before the root"       "does not reach back"  "$repo" "$wf" "$((B - 1))"
  refused_as_error "a clone too shallow"   "does not reach back"  "$tmp/shallow" "$wf" "$((B + 250))"
  refused_as_error "no commits"            "cannot be read"       "$tmp/empty" "$wf" "$((B + 250))"
  refused_as_error "outside a repository"  "not in a git repository" "$tmp/outside" "$wf" "$((B + 250))"
  refused_as_error "a file that cannot be fetched" "the file cannot be read" \
                   "$tmp/blobless-cut-off" "$wf" "$((B + 250))"
  refused_as_error "a tree that cannot be fetched" "the tree of" \
                   "$tmp/treeless-cut-off" "$wf" "$((B + 250))"

  refused "no arguments"
  refused "one argument"         "$wf"
  refused "three arguments"      "$wf" "$B" extra
  refused "an empty path"        '' "$B"
  refused "a time that is not a number" "$wf" yesterday
  refused "a negative time"      "$wf" -1
  refused "a leading zero"       "$wf" "0$B"
}

# Each mutant breaks one behaviour that the controls claim. Its entry holds a
# description, the text it replaces, and the replacement.
mutations=(
  'every ancestor, not the first parents'  ' --first-parent'                 ''
  'the newest commit, whatever its time'   ' --before="@$time"'              ''
  'a commit at the time not in force'      '--before="@$time"'               '--before="@$((time - 1))"'
  'more than one commit'                   'git rev-list -1'                 'git rev-list -2'
  'a history too short read as absent'     'error "the history of HEAD does not reach back that far"' 'exit 0'
  'an unreadable history read as absent'   'error "the history of HEAD cannot be read"'  'exit 0'
  'an unreadable tree read as absent'      'error "the tree of $commit cannot be read"'  'exit 0'
  'an unreadable file read as absent'      'error "the file cannot be read from $commit"' 'exit 0'
  'an absent file read'                    '[ -n "$listed" ] || exit 0'      ':'
  'the path read from where it is run'     'cd "$top" || error'              ': || error'
  'outside a repository read as absent'    'error "not in a git repository"' 'exit 0'
  'two cron lines accepted'                '[ "$lines" -le 1 ]'              '[ "$lines" -le 2 ]'
  'no cron line refused'                   '[ "$lines" -le 1 ]'              '[ "$lines" -eq 1 ]'
  'a commented-out line read'              's/^ *- cron:'                    's/^.*- cron:'
  'an empty line for no cron line'         '[ -z "$cron" ] || echo "$cron"'  'echo "$cron"'
  'an error succeeds'                      $'  exit 1\n}'                    $'  exit 0\n}'
  'the error not an annotation'            'echo "::error::'                 'echo "error: '
  'the usage on standard output'           '<epoch second>" >&2'             '<epoch second>"'
  'a refusal fails as any error'           '  exit 2'                        '  exit 1'
  'an argument fewer accepted'             '[ $# -eq 2 ]'                    '[ $# -ge 1 ]'
  'an argument more accepted'              '[ $# -eq 2 ]'                    '[ $# -ge 2 ]'
  'an empty path accepted'                 ' && [ -n "$1" ]'                 ''
  'a leading zero accepted'                '^(0|[1-9][0-9]*)$'               '^[0-9]+$'
  'any time accepted'                      ' && [[ $2 =~ ^(0|[1-9][0-9]*)$ ]]' ''
)

if [ "$*" != --mutations ]; then
  controls "$script"
  if [ "$fails" -eq 0 ]; then echo "cron_line_at: all controls pass"; else exit 1; fi
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
