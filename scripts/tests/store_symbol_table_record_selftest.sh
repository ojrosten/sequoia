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
#   - a record for a commit held is merged with the held one, as a union: a
#     field only the offered record has is added, as a commit on the tip
#     naming the field added, with the offered run and the keys sorted; a
#     field only the held record has is kept, whether the offer adds another
#     or nothing, a field with a null value included; the same record again
#     adds nothing and leaves the branch as it was; a field both have and
#     which differs, whether beside an addition or not, whether the offered
#     one is new or was itself added, is refused and the held record kept;
#   - the model name of the CPU of a measurement, cpu_model in the section of
#     prune's analysis, of the runner's setup, of the cost of leftover
#     materials or of the cost of compiling, is never compared: an offer
#     differing from the held record in it alone, or in it and by a count
#     within its tolerance, is accepted; the held section keeps its model
#     name with its counts, whether the offer adds a field or not; a held
#     section without one is not given the offered one, though a section
#     which only the offer has is added with its own; a count beyond its
#     tolerance is still refused beside a differing model name; a sub-field
#     which the offer adds to a held section leaves it its model name only if
#     the offered section has the same, and otherwise, or if the held section
#     has none, leaves it none;
#   - the exit status of each run a section measured (prune's stamping run, the
#     runner setup's run, and each of the two runs of the cost of leftover
#     materials) is a fact of the run, so it is never compared either: an
#     offer differing in one alone, or in all, is accepted and the held exit
#     status kept, as is a no-op when it adds nothing; an offer which differs
#     in them and adds a field or a section stores the union with the held
#     exit statuses, and a section which only the offer has with its own;
#   - the union is taken at every depth: a sub-field of a held object is
#     added, and one the offer lacks is kept;
#   - a measured count, with the tolerance of its own (prune's instructions 1
#     part in 1000, the runner setup's instructions 1 part in 50 and its system
#     calls 1 part in 200, the instructions of each of the two runs of the cost
#     of leftover materials 1 part in 50, and that section's entries and tests
#     none, so they are compared exactly; the instructions of compiling each
#     unit 1 part in 100,000, the boundary none), which differs from the
#     held one by less than that
#     is taken to be the held one: the record is accepted, the difference
#     said, and the held count kept, in a record which adds a field as in one
#     which adds none; a difference of exactly the tolerance, or more, is
#     refused with the difference shown, as is a difference in a field which
#     is not measured (the symbols) however small, even beside a measured
#     count within its tolerance; a tolerance is the field's own, so a
#     difference which one field's tolerance accepts is refused by a field
#     with a smaller; the entry naming every unit's instructions holds for
#     no count beside the units, nor within a unit;
#   - in a clone without blobs, a held record whose blob cannot be fetched is
#     a failure, not a record absent;
#   - a push refused because another push came first is retried on the new
#     tip, so that both pushes' records are kept, whether the branch existed
#     or the other push created it; a push refused every time fails, after a
#     bounded number of attempts;
#   - a remote which cannot be listed is a failure, not a branch to create;
#   - a record which cannot be read, or was made from a tree which was not
#     clean, or names no commit of the clone by its full name, and arguments
#     of any other number, are each refused, and nothing is pushed. A record
#     names no commit by its full name if its commit is an absent object, an
#     object which is no commit, a branch's name or an abbreviated name;
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

# record <file> <commit> <symbols> <run id> [<clean> [<more fields>]]: a record, laid out
# unlike jq's output, so that a stored copy which jq has rewritten differs
record() {
  printf '{"run": {"id": %s, "url": "https://example.invalid/runs/%s"},\n' "$4" "$4" > "$1"
  printf ' "commit": "%s", "clean": %s,\n "executable": {"symbols": %s}%s}\n' "$2" "${5:-true}" "$3" "${6-}" >> "$1"
}

# fixture: a bare remote; the clone the script runs in, with two commits, $A
# and $B, which it has pushed to the remote; and the records
fixture() {
  remote="$work/remote.git" clone="$work/clone"
  mkdir "$work/bin"
  git init --quiet --bare --initial-branch=main "$remote"
  git init --quiet --initial-branch=main "$clone"
  git -C "$clone" remote add origin "$remote"
  echo source > "$clone/source"
  git -C "$clone" add source
  git -C "$clone" "${identity[@]}" commit --quiet -m source
  A=$(git -C "$clone" rev-parse HEAD)
  git -C "$clone" "${identity[@]}" commit --quiet --allow-empty -m later
  B=$(git -C "$clone" rev-parse HEAD)
  git -C "$clone" push --quiet origin main
  echo uncommitted > "$clone/source"
  record "$work/A.json"        "$A" 100 1
  record "$work/A-rerun.json"  "$A" 100 2
  record "$work/A-other.json"  "$A" 101 3
  record "$work/B.json"        "$B" 200 4
  record "$work/A-added.json"  "$A" 100 5 true ', "prune_analysis": {"instructions": 7}'
  record "$work/A-added-changed.json" "$A" 101 6 true ', "prune_analysis": {"instructions": 7}'
  record "$work/A-added-other.json"   "$A" 100 7 true ', "prune_analysis": {"instructions": 8}'
  record "$work/A-null.json"   "$A" 100 8 true ', "note": null'
  # measurements of both sections on CPUs of different models: the same
  # counts, counts within their tolerances, and a count beyond its tolerance
  local both='"prune_dependency_analysis": {"instructions": %s, "cpu_model": "%s"}, "runner_setup": {"instructions": 1000000000, "syscalls": 834813, "cpu_model": "%s"}'
  local amd='AMD EPYC 7763 64-Core Processor' intel='Intel(R) Xeon(R) Platinum 8370C CPU @ 2.80GHz'
  record "$work/C-amd.json"          "$A" 100 41 true ", $(printf "$both" 1000000000 "$amd" "$amd")"
  record "$work/C-intel.json"        "$A" 100 42 true ", $(printf "$both" 1000000000 "$intel" "$intel")"
  record "$work/C-intel-near.json"   "$A" 100 43 true ", $(printf "$both" 1000999999 "$intel" "$intel")"
  record "$work/C-intel-far.json"    "$A" 100 44 true ", $(printf "$both" 1001000000 "$intel" "$intel")"
  record "$work/C-sub-amd.json"      "$A" 100 46 true ", $(printf "$both" 1000000000 "$amd" "$amd" | sed 's/834813,/834813, "other": 5,/')"
  record "$work/C-sub-intel.json"    "$A" 100 47 true ", $(printf "$both" 1000000000 "$intel" "$intel" | sed 's/834813,/834813, "other": 5,/')"
  record "$work/C-intel-added.json"  "$A" 100 45 true ", $(printf "$both" 1000000000 "$intel" "$intel"), \"extra\": 1"
  # a held record with an old measurement and a nested object, offers which
  # drop the old measurement, add a nested sub-field, or lack one
  record "$work/U.json"        "$A" 100 31 true ', "prune_analysis": {"instructions": 7}, "runner_setup": {"instructions": 1000}'
  record "$work/U-new.json"    "$A" 100 32 true ', "runner_setup": {"instructions": 1000}, "prune_dependency_analysis": {"instructions": 9}'
  record "$work/U-sub.json"    "$A" 100 33 true ', "runner_setup": {"instructions": 1000, "syscalls": 5}'
  record "$work/U-lacking.json" "$A" 100 34 true ', "runner_setup": {"syscalls": 5}, "extra": 1'
  record "$work/U-nested-differs.json" "$A" 100 35 true ', "runner_setup": {"instructions": 2000, "syscalls": 5}'
  record "$work/U-only-old.json" "$A" 100 36 true ', "runner_setup": {"instructions": 1000}'

  # runner_setup: 1000000000 instructions and 834813 system calls are held,
  # with tolerances of 1 in 50 and 1 in 200, so 20000000 and 4174.065 are too
  # far; prune_dependency_analysis has 1 in 1000, so 1000000 is too far
  local setup='"runner_setup": {"instructions": %s, "syscalls": %s}'
  record "$work/S.json"          "$A" 100 11 true ", $(printf "$setup" 1000000000 834813)"
  record "$work/S-near.json"     "$A" 100 12 true ", $(printf "$setup" 1019999999 834813)"
  record "$work/S-near-down.json" "$A" 100 13 true ", $(printf "$setup" 980000001 834813)"
  record "$work/S-boundary.json" "$A" 100 14 true ", $(printf "$setup" 1020000000 834813)"
  record "$work/S-far.json"      "$A" 100 15 true ", $(printf "$setup" 1030000000 834813)"
  record "$work/S-far-down.json" "$A" 100 24 true ", $(printf "$setup" 970000000 834813)"
  record "$work/S-syscalls.json" "$A" 100 16 true ", $(printf "$setup" 1000000000 837000)"
  record "$work/S-syscalls-far.json" "$A" 100 25 true ", $(printf "$setup" 1000000000 840000)"
  record "$work/S-near-symbols.json" "$A" 101 17 true ", $(printf "$setup" 1019999999 834813)"
  record "$work/P.json"          "$A" 100 18 true ', "prune_dependency_analysis": {"instructions": 1000000000}'
  record "$work/P-near.json"     "$A" 100 19 true ', "prune_dependency_analysis": {"instructions": 1000999999}'
  record "$work/P-far.json"      "$A" 100 20 true ', "prune_dependency_analysis": {"instructions": 1001000000}'
  record "$work/P-near-added.json" "$A" 100 21 true \
         ", \"prune_dependency_analysis\": {\"instructions\": 1000999999}, $(printf "$setup" 5 7)"
  record "$work/N.json"          "$A" 100 22 true ', "other_count": {"instructions": 1000000000}'
  record "$work/N-near.json"     "$A" 100 23 true ', "other_count": {"instructions": 1000999999}'
  # the cost of leftover materials: the tolerance of 1 in 50 of 50000000 is
  # 1000000, and of 54000000 is 1080000
  local leftover='"leftover_materials_setup": {"boundary": "B", "leftover_entries": %s, "without_leftovers": {"instructions": %s, "tests": %s, "exit_status": 0}, "with_leftovers": {"instructions": %s, "tests": %s, "exit_status": 0}%s, "cpu_model": "%s"}'
  record "$work/L.json"             "$A" 100 61 true ", $(printf "$leftover" 2713 50000000 97 54000000 97 '' "$amd")"
  record "$work/L-intel.json"       "$A" 100 62 true ", $(printf "$leftover" 2713 50000000 97 54000000 97 '' "$intel")"
  record "$work/L-without-near.json"  "$A" 100 63 true ", $(printf "$leftover" 2713 50999999 97 54000000 97 '' "$intel")"
  record "$work/L-without-far.json"   "$A" 100 64 true ", $(printf "$leftover" 2713 51000000 97 54000000 97 '' "$intel")"
  record "$work/L-with-near.json" "$A" 100 65 true ", $(printf "$leftover" 2713 50000000 97 55079999 97 '' "$intel")"
  record "$work/L-with-far.json"  "$A" 100 66 true ", $(printf "$leftover" 2713 50000000 97 55080000 97 '' "$intel")"
  record "$work/L-entries.json"     "$A" 100 67 true ", $(printf "$leftover" 2714 50000000 97 54000000 97 '' "$amd")"
  record "$work/L-tests.json"       "$A" 100 68 true ", $(printf "$leftover" 2713 50000000 98 54000000 98 '' "$amd")"
  record "$work/L-sub-amd.json"     "$A" 100 69 true ", $(printf "$leftover" 2713 50000000 97 54000000 97 ', "other": 5' "$amd")"
  record "$work/L-sub-intel.json"   "$A" 100 70 true ", $(printf "$leftover" 2713 50000000 97 54000000 97 ', "other": 5' "$intel")"
  record "$work/L-setup.json"       "$A" 100 71 true ", $(printf "$setup" 1000000000 834813), $(printf "$leftover" 2713 50000000 97 54000000 97 '' "$amd")"
  # exit statuses of the runs of each section, 0 or 82: the stamping run, the
  # runner setup's run, and the leftover runs without and with leftovers
  local exits='"prune_dependency_analysis": {"instructions": 1000000000, "stamping_exit_status": %s}, "runner_setup": {"instructions": 1000000000, "syscalls": 834813, "exit_status": %s}'
  local leftover_exits='"leftover_materials_setup": {"leftover_entries": 2713, "without_leftovers": {"instructions": 50000000, "tests": 97, "exit_status": %s}, "with_leftovers": {"instructions": 54000000, "tests": 97, "exit_status": %s}}'
  record "$work/X.json"          "$A" 100 81 true ", $(printf "$exits" 0 0), $(printf "$leftover_exits" 0 0)"
  record "$work/X-stamping.json" "$A" 100 82 true ", $(printf "$exits" 82 0), $(printf "$leftover_exits" 0 0)"
  record "$work/X-setup.json"    "$A" 100 83 true ", $(printf "$exits" 0 82), $(printf "$leftover_exits" 0 0)"
  record "$work/X-without.json"  "$A" 100 84 true ", $(printf "$exits" 0 0), $(printf "$leftover_exits" 82 0)"
  record "$work/X-with.json"     "$A" 100 85 true ", $(printf "$exits" 0 0), $(printf "$leftover_exits" 0 82)"
  record "$work/X-all.json"      "$A" 100 86 true ", $(printf "$exits" 82 82), $(printf "$leftover_exits" 82 82)"
  record "$work/X-all-extra.json" "$A" 100 87 true ", $(printf "$exits" 82 82), $(printf "$leftover_exits" 82 82), \"extra\": 1"
  record "$work/X-held.json"     "$A" 100 88 true ", $(printf "$exits" 0 0)"
  # the cost of compiling two units: the tolerance of 1 in 100,000 of
  # 50000000000 is 500000, and of 30000000000 is 300000
  local compile='"compile_cost": {"boundary": "%s", "units": {"Tests/A.cpp": %s, "Tests/B.cpp": %s%s}%s, "cpu_model": "%s"}'
  record "$work/K.json"           "$A" 100 91 true ", $(printf "$compile" B 50000000000 30000000000 '' '' "$amd")"
  record "$work/K-intel.json"     "$A" 100 92 true ", $(printf "$compile" B 50000000000 30000000000 '' '' "$intel")"
  record "$work/K-a-near.json"    "$A" 100 93 true ", $(printf "$compile" B 50000499999 30000000000 '' '' "$intel")"
  record "$work/K-a-far.json"     "$A" 100 94 true ", $(printf "$compile" B 50000500000 30000000000 '' '' "$intel")"
  record "$work/K-b-near.json"    "$A" 100 95 true ", $(printf "$compile" B 50000000000 30000299999 '' '' "$intel")"
  record "$work/K-b-far.json"     "$A" 100 96 true ", $(printf "$compile" B 50000000000 30000300000 '' '' "$intel")"
  record "$work/K-boundary.json"  "$A" 100 97 true ", $(printf "$compile" B2 50000000000 30000000000 '' '' "$amd")"
  record "$work/K-unit-amd.json"  "$A" 100 98 true ", $(printf "$compile" B 50000000000 30000000000 ', "Tests/C.cpp": 7' '' "$amd")"
  record "$work/K-unit-intel.json" "$A" 100 99 true ", $(printf "$compile" B 50000000000 30000000000 ', "Tests/C.cpp": 7' '' "$intel")"
  # counts within the section which no entry names: one beside the units, and
  # one within a unit, each 1 in 100,000,000 apart
  record "$work/K-other.json"     "$A" 100 100 true ", $(printf "$compile" B 50000000000 30000000000 '' ', "other": {"x": 100000000}' "$amd")"
  record "$work/K-other-near.json" "$A" 100 101 true ", $(printf "$compile" B 50000000000 30000000000 '' ', "other": {"x": 100000001}' "$amd")"
  record "$work/K-nested.json"    "$A" 100 102 true ", $(printf "$compile" B 50000000000 30000000000 ', "Tests/D.cpp": {"n": 100000000}' '' "$amd")"
  record "$work/K-nested-near.json" "$A" 100 103 true ", $(printf "$compile" B 50000000000 30000000000 ', "Tests/D.cpp": {"n": 100000001}' '' "$amd")"
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
paths() { # paths <path>...: the paths in the order `files` lists them
  printf '%s\n' "$@" | LC_ALL=C sort | tr '\n' ' '
}
held() { git --git-dir="$remote" show "refs/heads/metrics:symbol-table/linux/$1.json" 2>&1; }

# rival <body>: a stand-in for git, ahead of it on the script's PATH, which
# runs <body> before each push, and a second clone whose commit $rival the
# body can push by calling rival. The stand-in pushes before the script's
# push reads the remote, as another run's push would, so that a forced push
# overwrites the rival's commit and a control sees the rival's record lost.
# A rival pushed from a pre-push hook lands after the script's push has read
# the remote, and receive-pack then refuses even a forced push, so a mutant
# forcing its push would survive.
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
  same "it is beside the first" "$(paths "symbol-table/linux/$A.json" "symbol-table/linux/$B.json")" "$(files)"
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

# The CPU model of a measurement stays with the measurement's counts.
cpu_controls() {
  local first amd='AMD EPYC 7763 64-Core Processor' intel='Intel(R) Xeon(R) Platinum 8370C CPU @ 2.80GHz'
  fixture
  store C-amd; first=$(tip)
  store C-intel
  exits "an offer differing in the CPU models alone succeeds" 0
  says "it is the same but for its run" 'the same but for its run: https://example.invalid/runs/41$' "$out"
  same "it leaves the branch as it was" "$first" "$(tip)"
  same "prune's section keeps its CPU model" "$amd" "$(held "$A" | jq -r '.prune_dependency_analysis.cpu_model')"
  same "the setup's section keeps its CPU model" "$amd" "$(held "$A" | jq -r '.runner_setup.cpu_model')"

  store C-intel-near
  exits "an offer differing in the CPU models and by counts within their tolerances succeeds" 0
  says "the counts are said to be within tolerance" 'prune_dependency_analysis.instructions: held 1000000000, offered 1000999999' "$out"
  same "the branch is as it was after counts within tolerance" "$first" "$(tip)"

  store C-intel-far
  exits "an offer differing in the CPU models and by a count beyond its tolerance" nonzero
  says "the count is shown" '"instructions": 1001000000' "$err"
  same "the branch is as it was after a count beyond" "$first" "$(tip)"

  store C-intel-added
  exits "an offer adding a field, with another CPU model, succeeds" 0
  same "the held CPU model of prune's section stands" "$amd" "$(held "$A" | jq -r '.prune_dependency_analysis.cpu_model')"
  same "the held CPU model of the setup's section stands" "$amd" "$(held "$A" | jq -r '.runner_setup.cpu_model')"
  same "the field added is there" "1" "$(held "$A" | jq '.extra')"
  unchanged_clone "differing CPU models"

  # A sub-field which the offer adds to a held section
  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  store C-amd; first=$(tip)
  store C-sub-intel
  exits "a sub-field added beside another CPU model succeeds" 0
  same "the sub-field is added" "5" "$(held "$A" | jq '.runner_setup.other')"
  same "the section has no CPU model" "false" "$(held "$A" | jq '.runner_setup | has("cpu_model")')"
  same "prune's section, which gained nothing, keeps its CPU model" "$amd" \
       "$(held "$A" | jq -r '.prune_dependency_analysis.cpu_model')"

  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  store C-amd; first=$(tip)
  store C-sub-amd
  exits "a sub-field added beside the same CPU model succeeds" 0
  same "the sub-field is added to the section" "5" "$(held "$A" | jq '.runner_setup.other')"
  same "the section keeps its CPU model" "$amd" "$(held "$A" | jq -r '.runner_setup.cpu_model')"

  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  store S; first=$(tip)
  store C-sub-amd
  exits "a sub-field added to a held section without a CPU model succeeds" 0
  same "the sub-field is added" "5" "$(held "$A" | jq '.runner_setup.other')"
  same "the section has none" "false" "$(held "$A" | jq '.runner_setup | has("cpu_model")')"
  same "prune's section, added whole, has the offer's" "$amd" "$(held "$A" | jq -r '.prune_dependency_analysis.cpu_model')"

  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  store A; first=$(tip)
  store C-intel
  exits "an offer adding both sections succeeds" 0
  same "prune's section is added with the offer's CPU model" "$intel" "$(held "$A" | jq -r '.prune_dependency_analysis.cpu_model')"
  same "the setup's section is added with the offer's CPU model" "$intel" "$(held "$A" | jq -r '.runner_setup.cpu_model')"

  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  store S; first=$(tip)
  store C-amd
  exits "an offer with CPU models to a held setup without one succeeds" 0
  says "it adds the section of prune alone" 'adds to: prune_dependency_analysis$' "$out"
  same "the held setup has no CPU model" "false" "$(held "$A" | jq '.runner_setup | has("cpu_model")')"
  same "the held setup counts stand" "1000000000" "$(held "$A" | jq '.runner_setup.instructions')"
  same "prune's section, which the offer added, has its CPU model" "$amd" \
       "$(held "$A" | jq -r '.prune_dependency_analysis.cpu_model')"

  store C-amd
  exits "the offer again" 0
  says "it adds nothing, though the held setup lacks its CPU model" 'the same but for its run' "$out"

  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  store C-amd; first=$(tip)
  store S
  exits "an offer without CPU models to a held section with them" 0
  same "the branch is as it was" "$first" "$(tip)"
  same "the held CPU model stands" "$amd" "$(held "$A" | jq -r '.runner_setup.cpu_model')"
}

# The cost of leftover materials: its instructions are measured counts with
# tolerances of their own, its other counts are compared exactly, and its CPU
# model stays with its counts.
leftover_controls() {
  local first amd='AMD EPYC 7763 64-Core Processor' intel='Intel(R) Xeon(R) Platinum 8370C CPU @ 2.80GHz'
  fixture
  store L; first=$(tip)
  store L-intel
  exits "an offer differing in the CPU model alone succeeds" 0
  says "it is the same but for its run" 'the same but for its run: https://example.invalid/runs/61$' "$out"
  same "it leaves the branch as it was" "$first" "$(tip)"
  same "the section keeps its CPU model" "$amd" "$(held "$A" | jq -r '.leftover_materials_setup.cpu_model')"

  store L-without-near
  exits "the run without leftovers' instructions within 1 in 50, beyond 1 in 100, succeed" 0
  says "the difference and the tolerance are shown" \
       'leftover_materials_setup.without_leftovers.instructions: held 50000000, offered 50999999, tolerance 1 in 50' "$out"
  same "the branch is as it was after the run without leftovers' instructions" "$first" "$(tip)"

  store L-without-far
  exits "the run without leftovers' instructions at exactly 1 in 50" nonzero
  says "the difference is shown" '"instructions": 51000000' "$err"
  same "the branch is as it was after the run without leftovers' far instructions" "$first" "$(tip)"

  store L-with-near
  exits "the run with leftovers' instructions within 1 in 50, beyond 1 in 100, succeed" 0
  says "the difference of the run with leftovers is shown" \
       'leftover_materials_setup.with_leftovers.instructions: held 54000000, offered 55079999, tolerance 1 in 50' "$out"
  same "the branch is as it was after the run with leftovers' instructions" "$first" "$(tip)"

  store L-with-far
  exits "the run with leftovers' instructions at exactly 1 in 50" nonzero
  says "the difference of the run with leftovers is shown" '"instructions": 55080000' "$err"

  store L-entries
  exits "one more leftover entry" nonzero
  says "the entries are named" 'in: leftover_materials_setup.leftover_entries$' "$err"
  same "the branch is as it was after the entries" "$first" "$(tip)"

  store L-tests
  exits "one more test in each run" nonzero
  says "the tests are named" 'in: leftover_materials_setup.with_leftovers.tests, leftover_materials_setup.without_leftovers.tests$' "$err"
  same "the branch is as it was after the tests" "$first" "$(tip)"
  unchanged_clone "the cost of leftover materials"

  # A sub-field which the offer adds to the held section
  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  store L
  store L-sub-intel
  exits "a sub-field added beside another CPU model succeeds" 0
  same "the sub-field is added" "5" "$(held "$A" | jq '.leftover_materials_setup.other')"
  same "the section has no CPU model" "false" "$(held "$A" | jq '.leftover_materials_setup | has("cpu_model")')"

  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  store L
  store L-sub-amd
  exits "a sub-field added beside the same CPU model succeeds" 0
  same "the sub-field is added to the section" "5" "$(held "$A" | jq '.leftover_materials_setup.other')"
  same "the section keeps its CPU model" "$amd" "$(held "$A" | jq -r '.leftover_materials_setup.cpu_model')"

  # The section beside the field it replaces
  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  store S; first=$(tip)
  store L-setup
  exits "an offer adding the section to a held runner_setup succeeds" 0
  says "the section is named" 'adds to: leftover_materials_setup$' "$out"
  same "the held runner_setup stands" "1000000000" "$(held "$A" | jq '.runner_setup.instructions')"
  same "the section is added with the offer's CPU model" "$amd" "$(held "$A" | jq -r '.leftover_materials_setup.cpu_model')"
  same "the addition is a commit on the tip" "$first" "$(parents "$(tip)")"

  first=$(tip)
  store L
  exits "an offer without runner_setup, beside a record with it" 0
  says "it adds nothing, and the held record has more" 'with every field of .* and more' "$out"
  same "the branch is as it was" "$first" "$(tip)"
}

# The cost of compiling the units: each unit's instructions are a measured
# count, matched by the entry whose path ends in a null, with a tolerance of
# its own; its CPU model stays with its counts.
compile_cost_controls() {
  local first amd='AMD EPYC 7763 64-Core Processor' intel='Intel(R) Xeon(R) Platinum 8370C CPU @ 2.80GHz'
  fixture
  store K; first=$(tip)
  store K-intel
  exits "an offer of the compile cost differing in the CPU model alone succeeds" 0
  says "the offer is the same but for its run" 'the same but for its run: https://example.invalid/runs/91$' "$out"
  same "the section keeps its CPU model" "$amd" "$(held "$A" | jq -r '.compile_cost.cpu_model')"

  store K-a-near
  exits "a unit's instructions within 1 in 100,000, beyond 1 in 200,000, succeed" 0
  says "the difference and the tolerance are shown" \
       'compile_cost.units.Tests/A.cpp: held 50000000000, offered 50000499999, tolerance 1 in 100000' "$out"
  same "the branch is as it was after a unit's instructions" "$first" "$(tip)"

  store K-a-far
  exits "a unit's instructions at exactly 1 in 100,000" nonzero
  says "the unit is named" 'in: compile_cost.units.Tests/A.cpp$' "$err"

  store K-b-near
  exits "another unit's instructions within its own tolerance succeed" 0
  says "the other unit's difference is shown" \
       'compile_cost.units.Tests/B.cpp: held 30000000000, offered 30000299999, tolerance 1 in 100000' "$out"

  store K-b-far
  exits "another unit's instructions at exactly its own tolerance" nonzero
  says "the other unit is named" 'in: compile_cost.units.Tests/B.cpp$' "$err"

  store K-boundary
  exits "another boundary" nonzero
  says "the boundary is named" 'in: compile_cost.boundary$' "$err"
  same "the branch is as it was after the refusals" "$first" "$(tip)"
  unchanged_clone "the compile cost"

  store K-unit-amd
  exits "a unit added beside the same CPU model succeeds" 0
  says "the unit is named" 'adds to: compile_cost.units.Tests/C.cpp$' "$out"
  same "the unit is added" "7" "$(held "$A" | jq '.compile_cost.units["Tests/C.cpp"]')"
  same "the section keeps its CPU model beside the unit added" "$amd" "$(held "$A" | jq -r '.compile_cost.cpu_model')"

  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  store K
  store K-unit-intel
  exits "a unit added beside another CPU model succeeds" 0
  same "the section has no CPU model" "false" "$(held "$A" | jq '.compile_cost | has("cpu_model")')"

  # A null stands for one key, at its own depth, below the keys before it
  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  store K-other
  store K-other-near
  exits "a count beside the units, 1 in 100,000,000 apart" nonzero
  says "the count beside the units is named" 'in: compile_cost.other.x$' "$err"
  store K-nested
  store K-nested-near
  exits "a count within a unit, 1 in 100,000,000 apart" nonzero
  says "the count within a unit is named" 'in: compile_cost.units.Tests/D.cpp.n$' "$err"
}

# The exit status of a run is a fact of the run which measured a section, not
# of the commit: it is never compared, and the held one stays.
exit_status_controls() {
  local first path
  fixture
  store X; first=$(tip)
  for path in stamping setup without with; do
    store "X-$path"
    exits "an offer whose $path exit status differs succeeds" 0
    says "it is the same but for its run" 'the same but for its run: https://example.invalid/runs/81$' "$out"
    same "it leaves the branch as it was after a differing $path exit status" "$first" "$(tip)"
  done
  store X-all
  exits "an offer differing in every exit status succeeds" 0
  same "it leaves the branch as it was after every exit status differs" "$first" "$(tip)"
  same "the held exit statuses stand" "0 0 0 0" \
       "$(held "$A" | jq -r '[.prune_dependency_analysis.stamping_exit_status, .runner_setup.exit_status, .leftover_materials_setup.without_leftovers.exit_status, .leftover_materials_setup.with_leftovers.exit_status] | join(" ")')"

  store X-all-extra
  exits "an offer differing in every exit status, and adding a field, succeeds" 0
  says "the field is named" 'adds to: extra$' "$out"
  same "the addition is a commit on the tip" "$first" "$(parents "$(tip)")"
  same "the union holds the added field" "1" "$(held "$A" | jq '.extra')"
  same "the union holds the held exit statuses" "0 0 0 0" \
       "$(held "$A" | jq -r '[.prune_dependency_analysis.stamping_exit_status, .runner_setup.exit_status, .leftover_materials_setup.without_leftovers.exit_status, .leftover_materials_setup.with_leftovers.exit_status] | join(" ")')"

  # An offer which adds a section, beside exit statuses which differ
  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  store X-held; first=$(tip)
  store X-all
  exits "an offer adding a section, beside differing exit statuses, succeeds" 0
  says "the section is named" 'adds to: leftover_materials_setup$' "$out"
  same "the addition is a commit on the tip" "$first" "$(parents "$(tip)")"
  same "the held exit statuses stand" "0 0" \
       "$(held "$A" | jq -r '[.prune_dependency_analysis.stamping_exit_status, .runner_setup.exit_status] | join(" ")')"
  same "the added section has the offer's exit statuses" "82 82" \
       "$(held "$A" | jq -r '[.leftover_materials_setup.without_leftovers.exit_status, .leftover_materials_setup.with_leftovers.exit_status] | join(" ")')"
  unchanged_clone "an exit status"
}

# A backfill measures commits recorded before the measurement existed, so
# the record it stores adds a field to the one held.
adding_controls() {
  local first added
  fixture
  store A; first=$(tip)
  store A-added
  exits "a record which adds a field to the one held succeeds" 0
  says "it names the field added" 'adds to: prune_analysis$' "$out"
  added=$(tip)
  same "it is a commit on the tip" "$first" "$(parents "$added")"
  same "it is beside nothing: the one path" "symbol-table/linux/$A.json " "$(files)"
  same "the held record is the union, keys sorted" "$(jq -S . "$work/A-added.json")" "$(held "$A")"
  unchanged_clone "a record added to"

  store A-added
  exits "the record again" 0
  says "it is the same but for its run" 'the same but for its run: https://example.invalid/runs/5$' "$out"
  same "it leaves the branch as it was" "$added" "$(tip)"

  store A
  exits "a record lacking the field added" 0
  says "it says the held record has more" 'with every field of .* and more: https://example.invalid/runs/5$' "$out"
  same "the field is kept" "$added" "$(tip)"

  store A-added-changed
  exits "a record which adds a field and changes another" nonzero
  says "the difference is shown" '"symbols": 101' "$err"
  same "the held record is kept" "$added" "$(tip)"

  store A-added-other
  exits "a record which changes the field added" nonzero
  says "the difference is shown" '"instructions": 8' "$err"
  same "the held record is kept after a changed added field" "$added" "$(tip)"
  unchanged_clone "a refused addition"

  # A null field absent from the offered record is kept, though null is what
  # looking it up gives.
  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  store A-null; first=$(tip)
  store A-added
  exits "a record lacking a field of the held record which is null" 0
  same "the null field is kept in the union" "true" "$(held "$A" | jq 'has("note") and .note == null')"
  same "the field added is in the union" "7" "$(held "$A" | jq '.prune_analysis.instructions')"
  same "it is a commit on the tip" "$first" "$(parents "$(tip)")"
}

# The union of the held record and the offered, at every depth.
union_controls() {
  local first added
  fixture
  store U; first=$(tip)
  store U-new
  exits "an offer lacking a held field and adding another succeeds" 0
  says "it names the field added" 'adds to: prune_dependency_analysis$' "$out"
  added=$(tip)
  same "it is a commit on the tip" "$first" "$(parents "$added")"
  same "the held field the offer lacks is kept" "7" "$(held "$A" | jq '.prune_analysis.instructions')"
  same "the field added is there" "9" "$(held "$A" | jq '.prune_dependency_analysis.instructions')"
  same "the run is the offer's" "32" "$(held "$A" | jq '.run.id')"

  store U
  exits "the held record again" 0
  says "it has every field and more" 'with every field of .* and more' "$out"
  same "the branch is as it was after the held record again" "$added" "$(tip)"

  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  store U; first=$(tip)
  store U-sub
  exits "an offer adding a sub-field to a held object succeeds" 0
  says "it names the path added" 'adds to: runner_setup.syscalls$' "$out"
  same "the sub-field is added" "5" "$(held "$A" | jq '.runner_setup.syscalls')"
  same "the held sub-field stands" "1000" "$(held "$A" | jq '.runner_setup.instructions')"
  added=$(tip)

  store U-lacking
  exits "an offer lacking a held sub-field and adding a field succeeds" 0
  same "the held sub-field is kept" "1000" "$(held "$A" | jq '.runner_setup.instructions')"
  same "the held old measurement is kept" "7" "$(held "$A" | jq '.prune_analysis.instructions')"
  same "the field added is there" "1" "$(held "$A" | jq '.extra')"
  added=$(tip)

  store U-nested-differs
  exits "a sub-field which differs" nonzero
  says "its path is named" 'in: runner_setup.instructions' "$err"
  same "the held record is kept after a sub-field differs" "$added" "$(tip)"

  store U-only-old
  exits "an offer which adds nothing and lacks fields" 0
  same "no commit is made" "$added" "$(tip)"
  unchanged_clone "a union"
}

# A measured count of a rerun may differ slightly from the held one, which
# stands; any other difference is refused.
tolerance_controls() {
  local first
  fixture
  store S; first=$(tip)
  store S-near
  exits "a runner_setup count within 1 in 50, beyond 1 in 200, succeeds" 0
  says "it says the counts are within their tolerances" 'within their tolerances of those of symbol-table/linux/' "$out"
  says "it shows the difference and the tolerance" \
       'runner_setup.instructions: held 1000000000, offered 1019999999, tolerance 1 in 50' "$out"
  same "it leaves the branch as it was" "$first" "$(tip)"
  same "the held record is the first measurement, byte for byte" "$(cat "$work/S.json")" "$(held "$A")"

  store S-near-down
  exits "a count within the tolerance below the held one" 0
  same "the branch is as it was after a lower count" "$first" "$(tip)"

  store S-boundary
  exits "a count differing by exactly 1 part in 50" nonzero
  says "the difference is shown" '"instructions": 1020000000' "$err"
  same "the branch is as it was after the boundary" "$first" "$(tip)"

  store S-far
  exits "a count beyond the tolerance" nonzero
  says "the difference of a count beyond is shown" '"instructions": 1030000000' "$err"
  same "the held record is kept after a count beyond" "$first" "$(tip)"

  store S-far-down
  exits "a count beyond the tolerance below the held one" nonzero
  says "the difference of a count below is shown" '"instructions": 970000000' "$err"

  store S-syscalls
  exits "a system call count within 1 in 200 but beyond 1 in 10000 succeeds" 0
  says "it shows the system calls" 'runner_setup.syscalls: held 834813, offered 837000, tolerance 1 in 200' "$out"
  same "the branch is as it was after the system calls" "$first" "$(tip)"

  store S-syscalls-far
  exits "a system call count beyond 1 in 200" nonzero
  says "the difference of the system calls is shown" '"syscalls": 840000' "$err"
  same "the held record is kept after system calls beyond" "$first" "$(tip)"

  store S-near-symbols
  exits "a count within the tolerance beside a field which differs" nonzero
  says "the differing field is shown" '"symbols": 101' "$err"
  same "the held record is kept despite the field" "$first" "$(tip)"
  unchanged_clone "a tolerance"

  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  store P; first=$(tip)
  store P-near
  exits "a prune_dependency_analysis count within the tolerance succeeds" 0
  says "it shows the difference" 'prune_dependency_analysis.instructions: held 1000000000, offered 1000999999' "$out"
  same "the branch is as it was after the prune count" "$first" "$(tip)"
  store P-far
  exits "a prune_dependency_analysis count at exactly 1 in 1000, within 1 in 50" nonzero
  same "the branch is as it was after a prune count beyond" "$first" "$(tip)"

  # A record which adds a field, and has a count within the tolerance, is
  # stored with the count held.
  store P-near-added
  exits "an addition beside a count within the tolerance succeeds" 0
  says "the field added is named" 'adds to: runner_setup$' "$out"
  same "the addition is a commit on the tip" "$first" "$(parents "$(tip)")"
  same "the held count stands in the new record" "1000000000" \
       "$(held "$A" | jq '.prune_dependency_analysis.instructions')"
  same "the field added is in it" "5" "$(held "$A" | jq '.runner_setup.instructions')"

  # A count which is not named as measured is refused however small the
  # difference.
  work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
  fixture
  store N; first=$(tip)
  store N-near
  exits "a count which is not measured, differing by a little" nonzero
  same "the branch is as it was after an unmeasured count" "$first" "$(tip)"
}

# A clone without blobs, as a backfill might make, must fetch a held
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
  same "every record is kept" "$(paths rival "symbol-table/linux/$A.json" "symbol-table/linux/$B.json")" "$(files)"
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

  local name
  for name in "${A:0:12}" main "$(git -C "$clone" rev-parse "$A^{tree}")" \
              ffffffffffffffffffffffffffffffffffffffff ""; do
    record "$work/misnamed.json" "$name" 100 1
    store misnamed
    exits "a record whose commit is '$name'" nonzero
    says "it says why" "names no commit of this clone by its full name: its commit is '$name'" "$err"
  done

  echo '{"clean": true}' > "$work/none.json"
  store none
  exits "a record with no commit" nonzero
  says "it says why" "names no commit of this clone by its full name: its commit is 'null'" "$err"

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
  for control in create_controls beside_controls same_commit_controls cpu_controls exit_status_controls leftover_controls compile_cost_controls adding_controls union_controls tolerance_controls blobless_controls rival_controls \
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
mutant 'any commit accepted'           'full_name=$(git rev-parse --verify --quiet "$commit^{commit}") && [' \
                                       'true || ['
mutant 'a commit by any name'          ' && [ "$full_name" = "$commit" ] \' ' \'
mutant 'the status of rev-parse ignored' \
                                       'full_name=$(git rev-parse --verify --quiet "$commit^{commit}") && [' \
                                       'full_name=$(git rev-parse --verify --quiet "$commit^{commit}" || true) && ['
mutant 'any object accepted'           '"$commit^{commit}"' '"$commit"'
mutant 'the shape alone checked' \
       'full_name=$(git rev-parse --verify --quiet "$commit^{commit}") && [ "$full_name" = "$commit" ]' \
       '[[ $commit =~ ^[0-9a-f]{40}$ ]]'
mutant 'a tree not clean accepted'     '[ "$(jq '"'"'.clean'"'"' "$record")" = true ] \' 'true \'
mutant 'a record held read from its blob' \
       'git rev-parse --verify --quiet "$tip:$path" > /dev/null' \
       'git cat-file -e "$tip:$path" 2> /dev/null'
mutant 'stored under one name'         'path="$directory/$commit.json"' 'path="$directory/record.json"'
mutant 'a failed listing taken for no branch' '    2) echo "$remote has no branch' '    *) echo "$remote has no branch'
mutant 'held fields dropped'           '.value[$key] = $h[$key] | .kept += [$path + [$key]]' '.kept += [$path + [$key]]'
mutant 'offered fields not added'      '.value[$key] = $o[$key] | .added += [$path + [$key]] end)' '.added += [$path + [$key]] end)'
mutant 'a held field not noted as kept' ' | .kept += [$path + [$key]]' ''
mutant 'sub-fields not merged'         'merge($h[$key]; $o[$key]; $path + [$key]) as $r' '{value: $o[$key]} + none as $r'
mutant 'the offered value overwrites'  '{value: $h} + none + {conflicts: [$path]}' '{value: $o} + none'
mutant 'a tolerated count overwritten' '{value: $h} + none + {tolerated:' '{value: $o} + none + {tolerated:'
mutant 'a conflict not refused'        'if [ "$(jq '"'"'.conflicts | length'"'"' <<< "$merged")" -ne 0 ]; then' 'if false; then'
mutant 'a no-op committed'             'if [ "$(jq '"'"'.added | length'"'"' <<< "$merged")" -eq 0 ]; then' 'if false; then'
mutant 'a record with more not said'   'echo "$branch already holds $path, with every field of $record and more' 'echo "$branch already holds $path, the same but for its run'
mutant 'an addition not named'         '[.added[] | join(".")] | join(", ")' '""'
mutant 'the CPU model of prune compared' \
       '  ["prune_dependency_analysis", "cpu_model"],
' ''
mutant 'the CPU model of the setup compared' \
       ',
  ["runner_setup",              "cpu_model"]' ''
mutant 'a model kept beside an added sub-field' \
       'then del(.value.cpu_model) else . end' 'then . else . end'
mutant 'a model dropped beside an added sub-field of the same CPU' \
       'and ($h.cpu_model == null or $h.cpu_model != $o.cpu_model)' 'and true'
mutant 'the offered model adopted beside an added sub-field' \
       'then del(.value.cpu_model) else . end' 'then .value.cpu_model = $o.cpu_model else . end'
mutant 'an offered CPU model added to a held section' '           elif is_measuring_run_path($path + [$key]) then .
' ''
mutant 'the offered CPU model replaces the held' '{value: $h} + none
       elif ($h | type) == "object"' '{value: $o} + none
       elif ($h | type) == "object"'
mutant 'the CPU model of the leftover section compared' \
       ',
  ["leftover_materials_setup",  "cpu_model"]' ''
mutant 'the stamping exit status compared' \
       '  ["prune_dependency_analysis", "stamping_exit_status"],
' ''
mutant 'the exit status of the setup compared' \
       '  ["runner_setup",              "exit_status"],
' ''
mutant 'the exit status of the run without leftovers compared' \
       ',
  ["leftover_materials_setup",  "without_leftovers", "exit_status"]' ''
mutant 'the exit status of the run with leftovers compared' \
       ',
  ["leftover_materials_setup",  "with_leftovers",    "exit_status"]' ''
mutant 'the run compared too'          "without_run() { jq -S 'del(.run)'" "without_run() { jq -S '.'"
mutant 'every count measured'          '  {"path": ["runner_setup", "syscalls"],                                      "denominator": 200}' \
                                       '  {"path": ["runner_setup", "syscalls"],                                      "denominator": 200},
  {"path": ["other_count", "instructions"],               "denominator": 200}'
mutant 'prune counts not measured'     '  {"path": ["prune_dependency_analysis", "instructions"],                     "denominator": 1000},
' ''
mutant 'setup instructions not measured' '  {"path": ["runner_setup", "instructions"],                                  "denominator": 50},
' ''
mutant 'setup system calls not measured' ',
  {"path": ["runner_setup", "syscalls"],                                      "denominator": 200}' ''
mutant 'prune with the setup tolerance' '["prune_dependency_analysis", "instructions"],                     "denominator": 1000}' \
                                       '["prune_dependency_analysis", "instructions"],                     "denominator": 50}'
mutant "prune with its old tolerance"  '["prune_dependency_analysis", "instructions"],                     "denominator": 1000}' \
                                       '["prune_dependency_analysis", "instructions"],                     "denominator": 10000}'
mutant 'setup instructions with the prune tolerance' \
       '["runner_setup", "instructions"],                                  "denominator": 50}' \
       '["runner_setup", "instructions"],                                  "denominator": 10000}'
mutant 'setup system calls with the prune tolerance' \
       '["runner_setup", "syscalls"],                                      "denominator": 200}' \
       '["runner_setup", "syscalls"],                                      "denominator": 10000}'
mutant 'the tolerances of prune and the setup swapped' \
       '  {"path": ["prune_dependency_analysis", "instructions"],                     "denominator": 1000},
  {"path": ["runner_setup", "instructions"],                                  "denominator": 50},' \
       '  {"path": ["prune_dependency_analysis", "instructions"],                     "denominator": 50},
  {"path": ["runner_setup", "instructions"],                                  "denominator": 1000},'
mutant 'the tolerance of the setup 1 in 100' '["runner_setup", "instructions"],                                  "denominator": 50}' \
                                       '["runner_setup", "instructions"],                                  "denominator": 100}'
mutant "setup instructions with their old tolerance" '["runner_setup", "instructions"],                                  "denominator": 50}' \
                                       '["runner_setup", "instructions"],                                  "denominator": 200}'
mutant 'the tolerance of the setup 1 in 400' '["runner_setup", "syscalls"],                                      "denominator": 200}' \
                                       '["runner_setup", "syscalls"],                                      "denominator": 400}'
mutant 'the run without leftovers'"'"' instructions not measured' \
       '  {"path": ["leftover_materials_setup", "without_leftovers", "instructions"], "denominator": 50},
' ''
mutant 'the run with leftovers'"'"' instructions not measured' \
       ',
  {"path": ["leftover_materials_setup", "with_leftovers", "instructions"],    "denominator": 50}' ''
mutant 'the run without leftovers'"'"' instructions with the prune tolerance' '["leftover_materials_setup", "without_leftovers", "instructions"], "denominator": 50}' '["leftover_materials_setup", "without_leftovers", "instructions"], "denominator": 1000}'
mutant 'the run without leftovers'"'"' instructions with 1 in 100' '["leftover_materials_setup", "without_leftovers", "instructions"], "denominator": 50}' '["leftover_materials_setup", "without_leftovers", "instructions"], "denominator": 100}'
mutant 'the run without leftovers'"'"' instructions with 1 in 25' '["leftover_materials_setup", "without_leftovers", "instructions"], "denominator": 50}' '["leftover_materials_setup", "without_leftovers", "instructions"], "denominator": 25}'
mutant 'the run with leftovers'"'"' instructions with the prune tolerance' '["leftover_materials_setup", "with_leftovers", "instructions"],    "denominator": 50}' '["leftover_materials_setup", "with_leftovers", "instructions"],    "denominator": 1000}'
mutant 'the run with leftovers'"'"' instructions with 1 in 100' '["leftover_materials_setup", "with_leftovers", "instructions"],    "denominator": 50}' '["leftover_materials_setup", "with_leftovers", "instructions"],    "denominator": 100}'
mutant 'the run with leftovers'"'"' instructions with 1 in 25' '["leftover_materials_setup", "with_leftovers", "instructions"],    "denominator": 50}' '["leftover_materials_setup", "with_leftovers", "instructions"],    "denominator": 25}'
mutant 'the leftover entries measured' '  {"path": ["leftover_materials_setup", "without_leftovers", "instructions"], "denominator": 50},
' '  {"path": ["leftover_materials_setup", "without_leftovers", "instructions"], "denominator": 50},
  {"path": ["leftover_materials_setup", "leftover_entries"],                  "denominator": 50},
'
mutant 'the tests measured' '  {"path": ["leftover_materials_setup", "without_leftovers", "instructions"], "denominator": 50},
' '  {"path": ["leftover_materials_setup", "without_leftovers", "instructions"], "denominator": 50},
  {"path": ["leftover_materials_setup", "without_leftovers", "tests"],        "denominator": 50},
  {"path": ["leftover_materials_setup", "with_leftovers", "tests"],           "denominator": 50},
'
mutant 'the CPU model of the compile cost compared' \
       ',
  ["compile_cost",              "cpu_model"]' ''
mutant 'compiled units not measured'   ',
  {"path": ["compile_cost", "units", null],                                   "denominator": 100000}' ''
mutant 'compiled units with 1 in 10,000' '["compile_cost", "units", null],                                   "denominator": 100000}' \
                                       '["compile_cost", "units", null],                                   "denominator": 10000}'
mutant 'compiled units with 1 in 1,000,000' '["compile_cost", "units", null],                                   "denominator": 100000}' \
                                       '["compile_cost", "units", null],                                   "denominator": 1000000}'
mutant 'a null matching nothing'       'all(.[0] == null or .[0] == .[1])' 'all(.[0] == .[1])'
mutant 'every key matching'            'all(.[0] == null or .[0] == .[1])' 'all(true)'
mutant 'a path longer than its entry matching' \
       '($pattern | length) == ($path | length)
       and ' ''
mutant 'the boundary accepted'         '* $denominator < $held);' '* $denominator <= $held);'
mutant 'only a higher count tolerated' '((($offered - $held) | fabs)' '((($offered - $held))'
mutant 'the record stored as given'    'git hash-object -w -- "$stored")' 'git hash-object -w -- "$record")'
mutant 'a tolerated count not said'    'jq -r '"'"'.tolerated[] |' 'jq -r '"'"'.tolerated[] | empty |'
mutant 'a differing record overwritten' $'>&2\n      exit 1\n    fi' $'>&2\n    fi'
mutant 'the tip forgotten'             '${tip:+-p "$tip"}' ''
mutant 'the tip'"'"'s tree forgotten'  'if [ -n "$tip" ]; then GIT_INDEX_FILE=$index git read-tree' \
                                       'if false; then GIT_INDEX_FILE=$index git read-tree'
mutant 'the clone'"'"'s index used'    'GIT_INDEX_FILE=$index git update-index' 'git update-index'
mutant 'the record rewritten'          'git hash-object -w -- "$stored"' 'jq . "$stored" | git hash-object -w --stdin'
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
