#!/bin/bash
# Controls for store_symbol_table_record.sh, and a mutation check of the
# controls.
#
#   store_symbol_table_record_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls against the script. With
# it, the selftest runs the controls against the script, and then against each
# mutant listed below. The script must fail no control, and each mutant at
# least one. The last line gives the verdict.
#
# Each control is a claim the script exists to keep:
#   - a branch which does not exist is created as one commit with no parent,
#     holding the record alone, at <directory>/<commit>.json, byte for byte;
#   - a record for another commit is a commit on top of the branch's tip,
#     beside the records already there;
#   - a record for a commit the branch holds, the same but for its run,
#     leaves the branch as it was, and says which run made the record held;
#     one which differs elsewhere is refused, with the difference shown, and
#     the branch left as it was;
#   - in a clone without blobs, a held record whose blob cannot be fetched is
#     a failure, not a record absent;
#   - a push refused because another push came first is retried on the new
#     tip, so that both pushes' records are kept, whether the branch existed
#     or the other push created it; a push refused every time fails, after a
#     bounded number of attempts;
#   - a remote which cannot be listed is a failure, not a branch to create;
#   - a record which cannot be read, or names no commit, or was made from a
#     tree which was not clean, and arguments of any other number, are each
#     refused, and nothing is pushed;
#   - the clone the script runs in keeps its branches, index and working tree;
#     a push updates the clone's remote-tracking branch, as any push does.
#
# Every control runs against a local bare repository as the remote, and a
# clone of it. A stand-in for git stands in for another run too, by pushing
# its own commit just before the script's push.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script="$here/../store_symbol_table_record.sh"
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
verbose=yes
export GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_NOSYSTEM=1
identity=(-c user.name=selftest -c user.email=selftest@example.invalid)

fail() {
  fails=$((fails+1))
  if [[ $verbose == yes ]]; then echo "FAIL: $*"; fi
}

claim() { # claim <name> <command>...: the command succeeds
  total=$((total+1))
  if ! "${@:2}" > /dev/null 2>&1; then fail "$1"; fi
}

same() { # same <name> <expected> <got>
  total=$((total+1))
  if [[ $2 != "$3" ]]; then fail "$1 (expected '$2', got '$3')"; fi
}

says() { # says <name> <pattern> <file>
  total=$((total+1))
  if ! grep -qE -- "$2" "$3"; then fail "$1 (pattern: $2; said: $(cat "$3"))"; fi
}

exits() { # exits <name> <status|nonzero>
  total=$((total+1))
  if [[ $2 == nonzero ]]; then
    if [[ $rc -eq 0 ]]; then fail "$1 (expected a non-zero exit, got 0)"; fi
  elif [[ $rc -ne $2 ]]; then fail "$1 (expected exit $2, got $rc: $(cat "$err"))"; fi
}

A=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
B=bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb

# record <file> <commit> <symbols> <run id> [<clean>]: a record, laid out
# unlike jq's output, so that a stored copy which jq has rewritten differs
record() {
  printf '{"run": {"id": %s, "url": "https://example.invalid/runs/%s"},\n' "$4" "$4" > "$1"
  printf ' "commit": "%s", "clean": %s,\n "executable": {"symbols": %s}}\n' "$2" "${5:-true}" "$3" >> "$1"
}

# fixture: a bare remote, the clone the script runs in with one commit of its
# own, and the records
fixture() {
  remote="$work/remote.git" clone="$work/clone"
  mkdir "$work/bin"
  git init --quiet --bare --initial-branch=main "$remote"
  git init --quiet --initial-branch=main "$clone"
  git -C "$clone" remote add origin "$remote"
  echo source > "$clone/source"
  git -C "$clone" add source
  git -C "$clone" "${identity[@]}" commit --quiet -m source
  echo uncommitted > "$clone/source"
  record "$work/A.json"        "$A" 100 1
  record "$work/A-rerun.json"  "$A" 100 2
  record "$work/A-other.json"  "$A" 101 3
  record "$work/B.json"        "$B" 200 4
  clone_state=$(clone_state)
}

clone_state() {
  git -C "$clone" for-each-ref --format='%(refname) %(objectname)' refs/heads refs/tags
  git -C "$clone" symbolic-ref HEAD
  git -C "$clone" status --porcelain
  git -C "$clone" diff
}

run() { # run <argument>...: in the clone; output in $out, errors in $err, status in $rc
  (cd "$clone" && PATH="$work/bin:$PATH" bash "$subject" "$@") > "$out" 2> "$err"
  rc=$?
}

store() { run origin metrics symbol-table/linux "$work/$1.json"; }

tip() { git --git-dir="$remote" rev-parse --verify --quiet refs/heads/metrics; }
parents() { git --git-dir="$remote" log -1 --format=%P "$1"; }
files() { git --git-dir="$remote" ls-tree -r --name-only refs/heads/metrics | tr '\n' ' '; }
held() { git --git-dir="$remote" show "refs/heads/metrics:symbol-table/linux/$1.json" 2>&1; }

# rival <body>: a stand-in for git, ahead of it on the script's PATH, which
# runs <body> before each push, and a second clone whose commit $rival the
# body can push by calling rival. The stand-in pushes before the script's
# push reads the remote, as another run's push would: a pre-push hook would
# push only after, too late for a forced push to overwrite it.
rival() {
  local tip_now blob tree
  git clone --quiet "$remote" "$work/rival" 2> /dev/null
  echo rival > "$work/rival-file"
  blob=$(git -C "$work/rival" hash-object -w "$work/rival-file")
  tip_now=$(tip)
  rm -f "$work/rival-index"
  if [[ -n $tip_now ]]; then GIT_INDEX_FILE="$work/rival-index" git -C "$work/rival" read-tree "$tip_now"; fi
  GIT_INDEX_FILE="$work/rival-index" git -C "$work/rival" update-index --add --cacheinfo "100644,$blob,rival"
  tree=$(GIT_INDEX_FILE="$work/rival-index" git -C "$work/rival" write-tree)
  rival=$(git -C "$work/rival" "${identity[@]}" commit-tree "$tree" ${tip_now:+-p "$tip_now"} -m rival)
  {
    echo '#!/bin/bash'
    printf 'git=%q mark=%q\n' "$(command -v git)" "$work/pushes"
    printf 'rival() { "$git" -C %q push --quiet origin %q:refs/heads/metrics; }\n' "$work/rival" "$rival"
    echo "if [ \"\$1\" = push ]; then $1; fi"
    echo 'exec "$git" "$@"'
  } > "$work/bin/git"
  chmod +x "$work/bin/git"
}

unchanged_clone() { same "$1: the clone keeps its branches, index and working tree" "$clone_state" "$(clone_state)"; }

create_controls() {
  fixture
  store A
  exits "a record is stored on a new branch" 0
  says "the branch is said to be created" 'has no branch metrics: it is created' "$out"
  same "the new branch's commit has no parent" "" "$(parents "$(tip)")"
  same "the new branch holds the record alone" "symbol-table/linux/$A.json " "$(files)"
  same "the record is held byte for byte" "$(cat "$work/A.json")" "$(held "$A")"
  same "the commit is titled by the record's path" "symbol-table/linux/$A.json" \
       "$(git --git-dir="$remote" log -1 --format=%s refs/heads/metrics)"
  says "the commit names the run" 'Recorded by https://example.invalid/runs/1' \
       <(git --git-dir="$remote" log -1 --format=%b refs/heads/metrics)
  unchanged_clone "a new branch"
}

beside_controls() {
  local first
  fixture
  store A; first=$(tip)
  store B
  exits "a second commit's record is stored" 0
  same "it is a commit on the tip" "$first" "$(parents "$(tip)")"
  same "it is beside the first" "symbol-table/linux/$A.json symbol-table/linux/$B.json " "$(files)"
  same "the first is unchanged" "$(cat "$work/A.json")" "$(held "$A")"
  same "the second is held byte for byte" "$(cat "$work/B.json")" "$(held "$B")"
  unchanged_clone "a second record"
}

same_commit_controls() {
  local first
  fixture
  store A; first=$(tip)
  store A-rerun
  exits "a record the same but for its run succeeds" 0
  says "it says which run made the record held" 'the same but for its run: https://example.invalid/runs/1$' "$out"
  same "it leaves the branch as it was" "$first" "$(tip)"

  store A-other
  exits "a record differing beyond its run" nonzero
  says "the difference is shown" '"symbols": 101' "$err"
  same "the branch is left as it was" "$first" "$(tip)"
  same "the record held is the first" "$(cat "$work/A.json")" "$(held "$A")"
  unchanged_clone "a record for a commit held"
}

# A clone without blobs, as CI's sparse checkout makes, must fetch a held
# record's blob to read it. GIT_NO_LAZY_FETCH stands in for that fetch
# failing.
blobless_controls() {
  local first
  fixture
  store A; first=$(tip)
  git --git-dir="$remote" config uploadpack.allowFilter true
  git clone --quiet --filter=blob:none --no-checkout "file://$remote" "$work/blobless" 2> /dev/null
  clone="$work/blobless"
  GIT_NO_LAZY_FETCH=1 store A-other
  exits "a held record whose blob cannot be fetched" nonzero
  says "it says the record could not be read" 'git show .* failed' "$err"
  same "the record held is not replaced" "$first" "$(tip)"
}

rival_controls() {
  fixture
  store A
  rival '[ -e "$mark" ] || { touch "$mark"; rival; }'
  store B
  exits "a push refused because another came first is retried" 0
  says "the refusal is reported" 'Attempt 1 of 5' "$out"
  same "the retry is a commit on the other push's" "$rival" "$(parents "$(tip)")"
  same "every record is kept" "rival symbol-table/linux/$A.json symbol-table/linux/$B.json " "$(files)"
  unchanged_clone "a retried push"

  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  rival '[ -e "$mark" ] || { touch "$mark"; rival; }'
  store A
  exits "a branch another push created first is retried" 0
  same "the retry is a commit on the branch the other push created" "$rival" "$(parents "$(tip)")"
  same "both are kept" "rival symbol-table/linux/$A.json " "$(files)"

  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  rival 'exit 1'
  store A
  exits "a push refused every time" nonzero
  says "it fails after the attempts" 'failed 5 times' "$err"
  same "nothing is pushed" "" "$(tip)"
}

refusal_controls() {
  fixture
  record "$work/unclean.json" "$A" 100 1 false
  store unclean
  exits "a record of a tree not clean" nonzero
  says "it says why" 'was made from a tree which was not clean' "$err"
  same "nothing is pushed for a record of a tree not clean" "" "$(tip)"

  record "$work/short.json" abc123 100 1
  store short
  exits "a record naming no full commit" nonzero
  says "it says why" "names no commit: its commit is 'abc123'" "$err"

  echo '{"clean": true}' > "$work/none.json"
  store none
  exits "a record with no commit" nonzero
  says "it says why" "names no commit: its commit is 'null'" "$err"

  echo '{"commit": ' > "$work/broken.json"
  store broken
  exits "a record which is not JSON" nonzero
  says "it says why" 'cannot be read as JSON' "$err"
  same "nothing is pushed for a record refused" "" "$(tip)"

  run origin metrics symbol-table/linux
  exits "three arguments" 2
  run origin metrics symbol-table/linux "$work/A.json" extra
  exits "five arguments" 2
  same "nothing is pushed for the wrong arguments" "" "$(tip)"

  run "$work/no-such-remote.git" metrics symbol-table/linux "$work/A.json"
  exits "a remote which cannot be listed" nonzero
  says "it says the listing failed" 'git ls-remote .* failed' "$err"
  total=$((total+1))
  if grep -q 'it is created' "$out"; then fail "a remote which cannot be listed is taken for one with no branch"; fi
  unchanged_clone "a refusal"
}

run_controls() { # run_controls <script>: sets fails, and total, the controls run
  subject=$1 fails=0 total=0
  local control
  for control in create_controls beside_controls same_commit_controls blobless_controls rival_controls \
                 refusal_controls; do
    work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
    "$control"
  done
}

mutants=0
mutant() { # mutant <description> <text> <replacement>
  mutant_description[mutants]=$1 mutant_text[mutants]=$2 mutant_replacement[mutants]=$3
  mutants=$((mutants+1))
}
mutant 'arguments unchecked'           'if [ $# -ne 4 ]; then' 'if false; then'
mutant 'any commit accepted'           '[[ $commit =~ ^[0-9a-f]{40}$ ]] ||' 'true ||'
mutant 'a tree not clean accepted'     '[ "$(jq '"'"'.clean'"'"' "$record")" = true ] \' 'true \'
mutant 'a record held read from its blob' \
       'git rev-parse --verify --quiet "$tip:$path" > /dev/null' \
       'git cat-file -e "$tip:$path" 2> /dev/null'
mutant 'stored under one name'         'path="$directory/$commit.json"' 'path="$directory/record.json"'
mutant 'a failed listing taken for no branch' '    2) echo "$remote has no branch' '    *) echo "$remote has no branch'
mutant 'the run compared too'          "without_run() { jq -S 'del(.run)'" "without_run() { jq -S '.'"
mutant 'a differing record accepted'   'if [ "$held" = "$offered" ]; then' 'if true; then'
mutant 'a differing record overwritten' $'>&2\n    exit 1\n  fi' $'>&2\n  fi'
mutant 'the tip forgotten'             '${tip:+-p "$tip"}' ''
mutant 'the tip'"'"'s tree forgotten'  'if [ -n "$tip" ]; then GIT_INDEX_FILE=$index git read-tree' \
                                       'if false; then GIT_INDEX_FILE=$index git read-tree'
mutant 'the clone'"'"'s index used'    'GIT_INDEX_FILE=$index git update-index' 'git update-index'
mutant 'the record rewritten'          'git hash-object -w -- "$record"' 'jq . "$record" | git hash-object -w --stdin'
mutant 'the push forced'               'git push --quiet "$remote"' 'git push --quiet --force "$remote"'
mutant 'no retry'                      'attempts=5' 'attempts=1'
mutant 'the tip read once'             'tip=$(git rev-parse --verify FETCH_HEAD)' \
                                       'tip=${fetched:=$(git rev-parse --verify FETCH_HEAD)}'

mutations() {
  local i content stripped occurrences survivors=0
  verbose=no
  run_controls "$script"
  if ((fails)); then echo "unmutated: $fails of $total controls fail"; return 1; fi
  echo "unmutated: no control fails"
  for ((i = 0; i < mutants; i++)); do
    content=$(cat "$script"; echo x)
    content=${content%x}
    stripped=${content//"${mutant_text[i]}"/}
    occurrences=$(( (${#content} - ${#stripped}) / ${#mutant_text[i]} ))
    if [ "$occurrences" -ne 1 ]; then
      echo "${mutant_description[i]}: the text to mutate occurs $occurrences times  <-- SURVIVED"
      survivors=$((survivors+1))
      continue
    fi
    printf '%s' "${content%%"${mutant_text[i]}"*}${mutant_replacement[i]}${content#*"${mutant_text[i]}"}" \
      > "$tmp/mutant.sh"
    run_controls "$tmp/mutant.sh"
    if ((fails)); then
      echo "${mutant_description[i]}: $fails of $total controls fail"
    else
      echo "${mutant_description[i]}: SURVIVED"
      survivors=$((survivors+1))
    fi
  done
  if [ "$survivors" -ne 0 ]; then
    echo "store_symbol_table_record: $survivors of $mutants mutants survived"
    return 1
  fi
  echo "store_symbol_table_record: $mutants mutants, every one killed"
}

case "${1-}" in
  --mutations) mutations; exit ;;
  "")          ;;
  *)           echo "Usage: $0 [--mutations]" >&2; exit 2 ;;
esac

run_controls "$script"
if [[ $fails -eq 0 ]]; then echo "store_symbol_table_record selftest: all $total controls pass"; else
  echo "store_symbol_table_record selftest: $fails of $total controls failed"; fi
exit $((fails > 0))
