#!/bin/bash
# Controls for propose_tightened_baseline.sh, and a mutation check of the
# controls.
#
#   propose_tightened_baseline_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls against the script. With
# it, the selftest runs the controls against the script, and then against each
# mutant listed below until a control fails. The script must fail no control,
# and each mutant at least one. The last line gives the verdict:
#   -# "propose_tightened_baseline: <N> mutants, every one killed", with status
#      0;
#   -# otherwise, the number of mutants which survived, or the unmutated
#      script's failures, with status 1.
#
# Each control is a claim the script exists to keep:
#   - arguments of any other form are refused with status 2;
#   - an unchanged baseline proposes nothing, and closes this repository's
#     open PR, but never a fork's;
#   - a proposal is one commit on top of the measured one, changing the
#     baseline alone, pushed to the PR's branch, even over a branch left
#     behind;
#   - a proposal which adds, moves or changes a line, or which changes a
#     header, adds an entry or moves a key between files, is refused with
#     status 1 and nothing pushed or opened;
#   - an open PR proposing this baseline on this commit is kept, and armed if
#     it is not; any other is closed and replaced;
#   - a fork's PR from a branch of the same name is never adopted, edited,
#     closed or armed, nor is a PR whose repository gh does not report;
#   - the PR is armed to merge only with the head the script proposed;
#   - the script waits for GitHub's answer on whether the PR can merge, and
#     fails on a conflict or on no answer.
#
# Every control runs against a local bare repository as origin, and a stand-in
# for gh. The stand-in keeps each PR's state in a JSON file, applies the
# script's own --jq expressions to it with jq, and logs each call. A stand-in
# for sleep makes the waits instant.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
scripts=$(cd "$here/.." && pwd -P)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

fail() {
  echo "FAIL: $1"
  fails=$((fails+1))
  [ "$stop_at_first_failure" = no ] || exit 1
}

baseline_text='# demangler: fake 1.2
# tool: faketool 9.9.1
Source/a.cpp
    ns::alpha()
    ns::beta()
Source/b.cpp
    ns::gamma()
    ns::gamma()
'

# The stand-in for gh. State lives in $GH_STATE: prs.json lists the open PRs,
# pr-<N>.json holds each PR's state, and gh.log every call.
mkdir -p "$tmp/bin"
cat > "$tmp/bin/gh" <<'GH'
#!/bin/bash
echo "gh $*" >> "$GH_STATE/gh.log"
jq_of() { # jq_of <file>: applies the --jq expression of the call to <file>
  local expression= previous=
  for argument in "${ARGS[@]}"; do
    [ "$previous" = --jq ] && expression=$argument
    previous=$argument
  done
  jq -r "$expression" "$1"
}
ARGS=("$@")
case "$1 $2" in
  "pr list")
    jq_of "$GH_STATE/prs.json" ;;
  "pr view")
    number=$3
    # A PR may read UNKNOWN for its first few asks whether it can merge, as
    # GitHub's does.
    views=0
    if [[ " $* " == *" state,mergeable "* ]]; then
      views=$(( $(cat "$GH_STATE/views-$number" 2>/dev/null || echo 0) + 1 ))
      echo "$views" > "$GH_STATE/views-$number"
    fi
    if [ "$views" -ge 1 ] && [ "$views" -le "$(cat "$GH_STATE/unknown_views" 2>/dev/null || echo 0)" ]; then
      jq '.mergeable = "UNKNOWN"' "$GH_STATE/pr-$number.json" > "$GH_STATE/view.json"
      jq_of "$GH_STATE/view.json"
    else
      jq_of "$GH_STATE/pr-$number.json"
    fi ;;
  "pr close")
    number=$3
    jq --argjson n "$number" 'map(select(.number != $n))' "$GH_STATE/prs.json" > "$GH_STATE/prs.tmp"
    mv "$GH_STATE/prs.tmp" "$GH_STATE/prs.json"
    jq '.state = "CLOSED"' "$GH_STATE/pr-$number.json" > "$GH_STATE/pr.tmp" && mv "$GH_STATE/pr.tmp" "$GH_STATE/pr-$number.json"
    head=$(jq -r .headRefName "$GH_STATE/pr-$number.json")
    [ "$(jq -r .isCrossRepository "$GH_STATE/pr-$number.json")" = true ] \
      || git --git-dir="$GH_ORIGIN" branch -q -D "$head" 2> /dev/null
    true ;;
  "pr create")
    head=
    previous=
    for argument in "$@"; do
      [ "$previous" = --head ] && head=$argument
      previous=$argument
    done
    number=$(( $(cat "$GH_STATE/next" 2>/dev/null || echo 300) ))
    echo $((number + 1)) > "$GH_STATE/next"
    jq --arg h "$head" --arg m "$(cat "$GH_STATE/mergeable" 2>/dev/null || echo MERGEABLE)" -n \
      '{state: "OPEN", mergeable: $m, autoMergeRequest: null, headRefName: $h, isCrossRepository: false}' \
      > "$GH_STATE/pr-$number.json"
    jq --argjson n "$number" '. + [{number: $n, isCrossRepository: false}]' "$GH_STATE/prs.json" > "$GH_STATE/prs.tmp"
    mv "$GH_STATE/prs.tmp" "$GH_STATE/prs.json"
    echo "https://github.com/owner/repository/pull/$number" ;;
  "pr merge")
    number=$3
    jq '.autoMergeRequest = {}' "$GH_STATE/pr-$number.json" > "$GH_STATE/pr.tmp" && mv "$GH_STATE/pr.tmp" "$GH_STATE/pr-$number.json"
    if [ -e "$GH_STATE/merge_at_once" ]; then
      jq '.state = "MERGED"' "$GH_STATE/pr-$number.json" > "$GH_STATE/pr.tmp" && mv "$GH_STATE/pr.tmp" "$GH_STATE/pr-$number.json"
    fi ;;
  *)
    echo "gh stand-in: unexpected call: $*" >&2
    exit 1 ;;
esac
GH
printf '#!/bin/sh\necho "sleep $*" >> "$GH_STATE/gh.log"\n' > "$tmp/bin/sleep"
chmod +x "$tmp/bin/gh" "$tmp/bin/sleep"

# Makes a fresh origin, a checkout of its one commit, and the stand-in's state,
# and sets `checkout`, `origin`, `state` and `measured`.
fresh() {
  local root
  root=$(mktemp -d "$tmp/case.XXXXXX")
  origin=$root/origin.git checkout=$root/checkout state=$root/state
  mkdir -p "$state" "$checkout/coverage_reports"
  echo '[]' > "$state/prs.json"
  git init -q --bare "$origin"
  git -C "$checkout" init -q
  printf '%s' "$baseline_text" > "$checkout/coverage_reports/uncalled_functions.txt"
  echo "source" > "$checkout/source.txt"
  git -C "$checkout" add -A
  git -C "$checkout" -c user.name=t -c user.email=t@t commit -q -m measured
  git -C "$checkout" remote add origin "$origin"
  git -C "$checkout" push -q origin HEAD:refs/heads/staging
  git -C "$checkout" checkout -q --detach
  measured=$(git -C "$checkout" rev-parse HEAD)
}

# Adds an open PR to the stand-in's state: add_pr <number> <cross repository>
add_pr() {
  jq --argjson n "$1" --argjson c "$2" '. + [{number: $n, isCrossRepository: $c}]' "$state/prs.json" > "$state/prs.tmp"
  mv "$state/prs.tmp" "$state/prs.json"
  jq -n --argjson c "$2" \
    '{state: "OPEN", mergeable: "MERGEABLE", autoMergeRequest: null,
      headRefName: "tighten-uncalled-functions-into-staging", isCrossRepository: $c}' > "$state/pr-$1.json"
}

proposal() { # proposal <text>: the path of a tightened baseline holding <text>
  local path
  path=$(mktemp "$tmp/proposal.XXXXXX")
  printf '%s' "$1" > "$path"
  echo "$path"
}

# Runs the script in the checkout, at the measured commit, as each run in CI
# has a fresh checkout. Sets `status` and `output`.
propose() { # propose <tightened baseline> [<branch>]
  git -C "$checkout" checkout -q --force --detach "$measured"
  git -C "$checkout" branch -q -D tighten-uncalled-functions-into-staging 2> /dev/null
  output=$(cd "$checkout" && PATH="$tmp/bin:$PATH" GH_STATE="$state" GH_ORIGIN="$origin" \
             bash "$script" "${2-staging}" "$1" https://example.invalid/run 2>&1)
  status=$?
}

remote_head() { git --git-dir="$origin" rev-parse --verify -q refs/heads/tighten-uncalled-functions-into-staging; }

logged() { grep -qF -- "$1" "$state/gh.log" 2> /dev/null; }

removal=$(printf '%s' "$baseline_text" | grep -v 'ns::beta()')

run_controls() {
  fails=0
  local removal_file pushed first

  # Refusals of the arguments.
  fresh
  removal_file=$(proposal "$removal"$'\n')
  for arguments in "" "staging" "staging $removal_file" "staging $tmp/absent https://example.invalid/run" \
                   "staging $removal_file https://example.invalid/run extra"; do
    (cd "$checkout" && PATH="$tmp/bin:$PATH" GH_STATE="$state" GH_ORIGIN="$origin" bash "$script" $arguments \
       > /dev/null 2>&1)
    [ $? -eq 2 ] || fail "the arguments '$arguments' were not refused with status 2"
  done
  (cd "$checkout" && PATH="$tmp/bin:$PATH" GH_STATE="$state" GH_ORIGIN="$origin" \
     bash "$script" "" "$removal_file" https://example.invalid/run > /dev/null 2>&1)
  [ $? -eq 2 ] || fail "an empty branch was not refused with status 2"
  [ ! -e "$state/gh.log" ] || fail "a refusal of the arguments called gh"

  # An unchanged baseline proposes nothing.
  fresh
  propose "$(proposal "$baseline_text")"
  [ "$status" -eq 0 ] || fail "an unchanged baseline gave status $status: $output"
  [ -z "$(remote_head)" ] || fail "an unchanged baseline pushed a branch"
  ! logged "pr create" || fail "an unchanged baseline opened a PR"

  # ... and closes this repository's open PR, but not a fork's.
  fresh
  add_pr 7 true
  add_pr 8 false
  propose "$(proposal "$baseline_text")"
  [ "$status" -eq 0 ] || fail "an unchanged baseline with open PRs gave status $status: $output"
  logged "pr close 8 --delete-branch" || fail "an unchanged baseline left this repository's PR open"
  ! logged "pr close 7" || fail "an unchanged baseline closed a fork's PR"

  # A PR whose repository gh does not report is not taken for this one's.
  fresh
  add_pr 10 false
  jq 'map(del(.isCrossRepository))' "$state/prs.json" > "$state/prs.tmp" && mv "$state/prs.tmp" "$state/prs.json"
  propose "$(proposal "$baseline_text")"
  ! logged "pr close 10" || fail "a PR whose repository is not reported was taken for this repository's"

  # A removal, with no PR open.
  fresh
  propose "$(proposal "$removal"$'\n')"
  [ "$status" -eq 0 ] || fail "a removal gave status $status: $output"
  pushed=$(remote_head)
  if [ -z "$pushed" ]; then
    fail "a removal pushed no branch"
  else
    [ "$(git --git-dir="$origin" rev-parse "$pushed^")" = "$measured" ] \
      || fail "the proposal is not one commit on top of the measured one"
    [ "$(git --git-dir="$origin" diff --name-only "$measured" "$pushed")" = coverage_reports/uncalled_functions.txt ] \
      || fail "the proposal changes more than the baseline"
    [ "$(git --git-dir="$origin" show "$pushed:coverage_reports/uncalled_functions.txt")" = "$removal" ] \
      || fail "the proposal's baseline is not the tightened one"
    git --git-dir="$origin" log -1 --format=%B "$pushed" | grep -qF '    ns::beta()' \
      || fail "the proposal's commit does not list the line it removes"
    logged "pr create --base staging --head tighten-uncalled-functions-into-staging" \
      || fail "a removal opened no PR from its branch into staging"
    logged "pr merge 300 --auto --merge --match-head-commit $pushed" \
      || fail "the PR was not armed to merge with the proposed head alone"
  fi
  [[ $output == *"#300 is mergeable and set to merge itself"* ]] \
    || fail "a removal did not report a mergeable PR: $output"

  # The same proposal again keeps the PR, and does not arm it again.
  first=$pushed
  : > "$state/gh.log"
  propose "$(proposal "$removal"$'\n')"
  [ "$status" -eq 0 ] || fail "the same proposal again gave status $status: $output"
  [ "$(remote_head)" = "$first" ] || fail "the same proposal again pushed a new commit"
  ! logged "pr close" || fail "the same proposal again closed its PR"
  ! logged "pr create" || fail "the same proposal again opened a PR"
  ! logged "pr merge" || fail "the same proposal again armed a PR already armed"

  # ... and arms the PR if it is not armed.
  jq '.autoMergeRequest = null' "$state/pr-300.json" > "$state/pr.tmp" && mv "$state/pr.tmp" "$state/pr-300.json"
  : > "$state/gh.log"
  propose "$(proposal "$removal"$'\n')"
  logged "pr merge 300 --auto --merge --match-head-commit $first" \
    || fail "an open PR proposing the same baseline was not armed"

  # A different proposal closes the open PR and opens another.
  : > "$state/gh.log"
  propose "$(proposal "$(printf '%s' "$removal" | grep -v 'ns::alpha()')"$'\n')"
  [ "$status" -eq 0 ] || fail "a different proposal gave status $status: $output"
  logged "pr close 300 --delete-branch" || fail "a different proposal left the old PR open"
  logged "pr create" || fail "a different proposal opened no PR"
  [ "$(remote_head)" != "$first" ] || fail "a different proposal pushed nothing"

  # A branch left behind, with no PR open, is overwritten.
  fresh
  git -C "$checkout" push -q origin "$measured:refs/heads/tighten-uncalled-functions-into-staging"
  git -C "$checkout" -c user.name=t -c user.email=t@t commit -q --allow-empty -m "left behind"
  git -C "$checkout" push -q --force origin HEAD:refs/heads/tighten-uncalled-functions-into-staging
  git -C "$checkout" checkout -q --detach "$measured"
  propose "$(proposal "$removal"$'\n')"
  [ "$status" -eq 0 ] || fail "a removal over a branch left behind gave status $status: $output"
  [ "$(git --git-dir="$origin" rev-parse "$(remote_head)^")" = "$measured" ] \
    || fail "a branch left behind was not overwritten"

  # A fork's PR from a branch of the same name is never adopted.
  fresh
  add_pr 9 true
  propose "$(proposal "$removal"$'\n')"
  [ "$status" -eq 0 ] || fail "a removal beside a fork's PR gave status $status: $output"
  ! logged "pr merge 9" || fail "a fork's PR was armed"
  ! logged "pr close 9" || fail "a fork's PR was closed"
  logged "pr create" || fail "a removal beside a fork's PR opened no PR of its own"

  # The same baseline proposed on a later commit closes the open PR, and
  # proposes it again on that commit.
  fresh
  removal_file=$(proposal "$removal"$'\n')
  propose "$removal_file"
  : > "$state/gh.log"
  git -C "$checkout" checkout -q --force --detach "$measured"
  echo "later source" > "$checkout/source.txt"
  git -C "$checkout" -c user.name=t -c user.email=t@t commit -q -am later
  measured=$(git -C "$checkout" rev-parse HEAD)
  propose "$removal_file"
  [ "$status" -eq 0 ] || fail "the same proposal on a later commit gave status $status: $output"
  logged "pr close 300 --delete-branch" || fail "the same proposal on a later commit kept the PR on the earlier one"
  [ "$(git --git-dir="$origin" rev-parse "$(remote_head)^")" = "$measured" ] \
    || fail "the same proposal on a later commit was not made on that commit"

  # Proposals which do more than remove lines, each refused for its reason.
  for refused in "added|not the committed one with lines deleted|$baseline_text    ns::delta()"$'\n' \
                 "moved|not the committed one with lines deleted|$(printf '%s' "$baseline_text" | awk '/ns::alpha\(\)/ { held = $0; next } { print } /ns::beta\(\)/ { print held }')"$'\n' \
                 "changed last line|not the committed one with lines deleted|$(printf '%s' "$baseline_text" | sed '$ s/ns::gamma()/ns::delta()/')"$'\n' \
                 "header|changes the header|$(printf '%s' "$baseline_text" | grep -v '^# tool')"$'\n' \
                 "file line|adds entries|$(printf '%s' "$baseline_text" | grep -vx 'Source/b.cpp')"$'\n'; do
    fresh
    reason=${refused#*|} reason=${reason%%|*}
    propose "$(proposal "${refused#*|*|}")"
    [ "$status" -eq 1 ] || fail "a proposal with a ${refused%%|*} change gave status $status, not 1: $output"
    [[ $output == *"$reason"* ]] || fail "a proposal with a ${refused%%|*} change was not refused as one that $reason: $output"
    [ -z "$(remote_head)" ] || fail "a proposal with a ${refused%%|*} change was pushed"
    ! logged "pr create" || fail "a proposal with a ${refused%%|*} change opened a PR"
  done

  # GitHub's answer on whether the PR can merge.
  fresh
  echo CONFLICTING > "$state/mergeable"
  propose "$(proposal "$removal"$'\n')"
  [ "$status" -eq 1 ] || fail "a conflicting PR gave status $status, not 1: $output"
  [[ $output == *"conflicts with staging"* ]] || fail "a conflict was not reported: $output"

  fresh
  echo 2 > "$state/unknown_views"
  propose "$(proposal "$removal"$'\n')"
  [ "$status" -eq 0 ] || fail "a PR whose answer came on the third ask gave status $status: $output"
  [ "$(grep -c '^sleep' "$state/gh.log")" -eq 2 ] || fail "the script did not wait for GitHub's answer"

  fresh
  echo 100 > "$state/unknown_views"
  propose "$(proposal "$removal"$'\n')"
  [ "$status" -eq 1 ] || fail "a PR with no answer gave status $status, not 1: $output"
  [ "$(grep -c '^sleep' "$state/gh.log")" -ge 10 ] || fail "the script gave up on GitHub's answer too soon"

  fresh
  touch "$state/merge_at_once"
  propose "$(proposal "$removal"$'\n')"
  [ "$status" -eq 0 ] || fail "a PR which merged at once gave status $status: $output"
  [[ $output == *"#300 has merged"* ]] || fail "a PR which merged at once was not reported: $output"
}

# Each mutant is a file, a description, the text to replace, which must occur
# exactly once, and its replacement.
mutants=0
mutant() {
  mutant_description[mutants]=$1 mutant_text[mutants]=$2 mutant_replacement[mutants]=$3
  mutants=$((mutants+1))
}
mutant 'arguments unchecked'       '[ $# -ne 3 ] || ' ''
mutant "a fork's PR adopted"        'map(select(.isCrossRepository == false)) | ' ''
mutant 'an unreported PR adopted'   '.isCrossRepository == false' '.isCrossRepository | not'
mutant 'nothing closed'             '[ -z "$pr" ] || close_pr' 'true || close_pr'
mutant 'a move accepted'            "    if at == len(committed_lines):" "    if False:"
mutant 'a line added past the end'  "    while at < len(committed_lines) and committed_lines[at] != line:" \
                                    "    while at < len(committed_lines) - 1 and committed_lines[at] != line:"
mutant 'the removed lines unlisted' 'zip(committed_lines, kept) if not was_kept' 'zip(committed_lines, kept) if False'
mutant 'a header change accepted'   'if proposed_header != committed_header:' 'if False:'
mutant 'an added entry accepted'    'if added:' 'if False:'
mutant 'the check ignored'          'python3 - "$here" "$baseline" "$tightened" <<' 'cat > /dev/null <<'
mutant 'any open PR kept'           '[ "$(git rev-parse "refs/remotes/origin/$head^")" = "$measured" ]' 'true'
mutant 'a different baseline kept'  'git show "refs/remotes/origin/$head:$baseline" | cmp -s - "$tightened"' 'true'
mutant 'no open PR kept'            'if git fetch --quiet --depth=2' 'if false && git fetch --quiet --depth=2'
mutant 'never armed'                $'!= true ]; then\n  gh pr merge' $'= never ]; then\n  gh pr merge'
mutant 'armed again'                $'!= true ]; then\n  gh pr merge' $'!= never ]; then\n  gh pr merge'
mutant 'armed for any head'         ' --match-head-commit "$proposed"' ''
mutant 'not forced'                 'git push --quiet --force' 'git push --quiet'
mutant 'a conflict accepted'        $'"OPEN CONFLICTING") echo "error: #$pr conflicts with $branch, whose baseline has changed since $measured"; exit 1;;' \
                                    $'"OPEN CONFLICTING") echo "conflicts with $branch";;'
mutant 'no wait for an answer'      '[ "$state" = OPEN ] && [ "$mergeable" = UNKNOWN ] || break' 'break'
mutant 'no answer accepted'         $'*)                  echo "error: #$pr reads \'$state $mergeable\'"; exit 1;;' \
                                    $'*)                  echo "#$pr reads \'$state $mergeable\'";;'

# Runs the controls against the script in <directory> until a control fails,
# and prints that failure, or nothing.
first_failure() { # first_failure <directory>
  (script="$1/propose_tightened_baseline.sh" stop_at_first_failure=yes; run_controls) 2>&1 | grep -m 1 '^FAIL: '
}

mutations() {
  local i survivors=0 file content stripped occurrences failure
  mkdir "$tmp/unmutated"
  cp "$scripts/propose_tightened_baseline.sh" "$scripts/uncalled_functions.py" "$tmp/unmutated/"
  failure=$(first_failure "$tmp/unmutated")
  if [ -n "$failure" ]; then
    echo "propose_tightened_baseline: the unmutated script fails: ${failure#FAIL: }"
    return 1
  fi
  echo "unmutated: no control fails"
  for ((i = 0; i < mutants; i++)); do
    rm -rf "$tmp/mutant"
    mkdir "$tmp/mutant"
    cp "$scripts/propose_tightened_baseline.sh" "$scripts/uncalled_functions.py" "$tmp/mutant/"
    file="$tmp/mutant/propose_tightened_baseline.sh"
    content=$(cat "$file"; echo x)
    content=${content%x}
    stripped=${content//"${mutant_text[i]}"/}
    occurrences=$(( (${#content} - ${#stripped}) / ${#mutant_text[i]} ))
    if [ "$occurrences" -ne 1 ]; then
      echo "${mutant_description[i]}: the text to mutate occurs $occurrences times  <-- SURVIVED"
      survivors=$((survivors+1))
      continue
    fi
    printf '%s' "${content%%"${mutant_text[i]}"*}${mutant_replacement[i]}${content#*"${mutant_text[i]}"}" > "$file"
    failure=$(first_failure "$tmp/mutant")
    if [ -n "$failure" ]; then
      echo "${mutant_description[i]}: killed by ${failure#FAIL: }"
    else
      echo "${mutant_description[i]}: SURVIVED"
      survivors=$((survivors+1))
    fi
  done
  if [ "$survivors" -ne 0 ]; then
    echo "propose_tightened_baseline: $survivors of $mutants mutants survived"
    return 1
  fi
  echo "propose_tightened_baseline: $mutants mutants, every one killed"
}

case "${1-}" in
  --mutations) mutations; exit ;;
  "")          ;;
  *)           echo "Usage: $0 [--mutations]" >&2; exit 2 ;;
esac

script=$scripts/propose_tightened_baseline.sh stop_at_first_failure=no
run_controls
if [ "$fails" -eq 0 ]; then echo "propose_tightened_baseline: all controls pass"; else exit 1; fi
