#!/bin/bash
# Usage: propose_tightened_baseline.sh <branch> <tightened baseline> <run url>
#
# Proposes <tightened baseline> as coverage_reports/uncalled_functions.txt on
# <branch>, through one PR from the branch
# tighten-uncalled-functions-into-<branch>, set to merge itself. The script
# runs in a checkout of the commit that was measured, with gh authenticated
# and origin the repository on GitHub. <run url> names the measurement in the
# PR and its commit.
#
#   -# If the tightened baseline equals the committed one, the script closes
#      the open PR, if any, and exits with status 0.
#   -# Otherwise it refuses, with status 1 and nothing pushed, a proposal which
#      does more than remove lines: the tightened baseline must be the
#      committed one with lines deleted, and read as the gate reads it, must
#      keep the header and add no entry to any file.
#   -# An open PR which already proposes this baseline on this commit is kept.
#      Any other open PR is closed, and its branch deleted, and a new PR is
#      opened from one commit on top of the measured one, changing the
#      baseline alone.
#   -# The PR is set to merge itself, as soon as its head is the commit
#      proposed. The script fails if GitHub reports that the PR conflicts with
#      <branch>.
#
# Only a PR from this repository is looked for. A fork's PR may come from a
# branch of the same name, and the script must never arm one.
#
# Arguments of any other form are refused with status 2.

set -euo pipefail

if [ $# -ne 3 ] || [ -z "$1" ] || [ ! -f "$2" ] || [ -z "$3" ]; then
  echo "Usage: $0 <branch> <tightened baseline> <run url>" >&2
  exit 2
fi

branch=$1 tightened=$2 run_url=$3
baseline=coverage_reports/uncalled_functions.txt
head=tighten-uncalled-functions-into-$branch
measured=$(git rev-parse HEAD)
here=$(cd "$(dirname "$0")" && pwd -P)

pr=$(gh pr list --base "$branch" --head "$head" --state open --json number,isCrossRepository \
       --jq 'map(select(.isCrossRepository | not)) | .[0].number // empty')

close_pr() { # close_pr <comment>
  gh pr close "$pr" --delete-branch --comment "$1"
  echo "closed #$pr"
  pr=
}

if cmp -s "$baseline" "$tightened"; then
  echo "$branch at $measured needs every entry of $baseline"
  [ -z "$pr" ] || close_pr "$branch at $measured needs every entry of \`$baseline\`, so nothing is left to remove: $run_url"
  exit 0
fi

# The check reads the two baselines as lines, and then as the gate reads them.
# A line moved counts as a line added, so a reordered baseline is refused. A
# file's line deleted would move its keys to the file before it, which the
# second reading sees. The check prints the lines removed.
removed=$(python3 - "$here" "$baseline" "$tightened" <<'CHECK'
import sys
from collections import Counter
sys.path.insert(0, sys.argv[1])
from uncalled_functions import read_baseline
with open(sys.argv[2], encoding='utf-8', errors='surrogateescape') as file:
    committed_lines = file.read().splitlines()
with open(sys.argv[3], encoding='utf-8', errors='surrogateescape') as file:
    proposed_lines = file.read().splitlines()
kept, at = [False] * len(committed_lines), 0
for line in proposed_lines:
    while at < len(committed_lines) and committed_lines[at] != line:
        at += 1
    if at == len(committed_lines):
        print(f'error: the tightened baseline is not the committed one with lines deleted: {line}', file=sys.stderr)
        sys.exit(1)
    kept[at], at = True, at + 1
committed_header, committed = read_baseline(sys.argv[2])
proposed_header,  proposed  = read_baseline(sys.argv[3])
added = [(file, key) for file, keys in proposed.items() for key in (keys - committed.get(file, Counter())).elements()]
if proposed_header != committed_header:
    print(f'error: the tightened baseline changes the header to {proposed_header}', file=sys.stderr)
    sys.exit(1)
if added:
    print(f'error: the tightened baseline adds entries: {added}', file=sys.stderr)
    sys.exit(1)
print('\n'.join(line for line, was_kept in zip(committed_lines, kept) if not was_kept))
CHECK
)

echo "Lines the tightened baseline removes:"
echo "$removed"

proposed=
if [ -n "$pr" ]; then
  if git fetch --quiet --depth=2 origin "+refs/heads/$head:refs/remotes/origin/$head" \
       && [ "$(git rev-parse "refs/remotes/origin/$head^")" = "$measured" ] \
       && git show "refs/remotes/origin/$head:$baseline" | cmp -s - "$tightened"; then
    proposed=$(git rev-parse "refs/remotes/origin/$head")
    echo "#$pr already proposes this baseline on $measured"
  else
    close_pr "Superseded by a measurement of $branch at $measured: $run_url"
  fi
fi

if [ -z "$proposed" ]; then
  cp "$tightened" "$baseline"
  git switch --quiet -c "$head"
  git -c user.name="sequoia CI" -c user.email=ci@example.invalid commit --quiet -F - -- "$baseline" <<MESSAGE
Tighten the uncalled-functions baseline

Measured on $branch at $measured, by $run_url. The lines removed:

$removed
MESSAGE
  git push --quiet --force origin "$head"
  proposed=$(git rev-parse HEAD)
  url=$(gh pr create --base "$branch" --head "$head" --title "Tighten the uncalled-functions baseline" \
          --body "Measured on \`$branch\` at $measured by $run_url. These entries of \`$baseline\` name functions that are now called, or gone:

\`\`\`
$removed
\`\`\`

Opened by baseline-tightener.yml, which says why. It removes entries only, and merges itself once its gate is green.")
  pr=${url##*/}
  echo "opened $url"
fi

if [ "$(gh pr view "$pr" --json autoMergeRequest --jq '.autoMergeRequest != null')" != true ]; then
  gh pr merge "$pr" --auto --merge --match-head-commit "$proposed"
fi

# GitHub works out whether a PR can merge after the PR changes, so the script
# asks until it has an answer, for up to a minute. A conflict would leave
# auto-merge waiting without a word.
for attempt in $(seq 1 12); do
  read -r state mergeable <<< "$(gh pr view "$pr" --json state,mergeable --jq '"\(.state) \(.mergeable)"')"
  [ "$state" = OPEN ] && [ "$mergeable" = UNKNOWN ] || break
  sleep 5
done
case "$state $mergeable" in
  MERGED\ *)          echo "#$pr has merged";;
  "OPEN MERGEABLE")   echo "#$pr is mergeable and set to merge itself";;
  "OPEN CONFLICTING") echo "error: #$pr conflicts with $branch, whose baseline has changed since $measured"; exit 1;;
  *)                  echo "error: #$pr reads '$state $mergeable'"; exit 1;;
esac
