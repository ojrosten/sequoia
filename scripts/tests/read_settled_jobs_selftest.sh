#!/bin/bash
# Controls for read_settled_jobs.sh, against a fake gh which serves one canned read of the jobs
# API per call. The wait is shrunk to 4 s, reading every 1 s. Each control is a claim the script
# exists to keep:
#
#   - a read on which the filter awaits nothing ends the wait at once, and is the jobs file;
#   - the read asks for every page of the jobs of the run named, in $GH_REPO;
#   - a read on which the filter awaits something is logged, naming it, and read again
#     after a pause;
#   - the options after the filter reach jq;
#   - a read that fails, or gives output that is not JSON, is retried;
#   - when the time runs out, a warning names what the filter still awaits, the jobs file holds
#     the last successful read, and the script succeeds, leaving the judgement to the caller;
#   - if no read succeeds, the script fails with an error;
#   - a filter that jq cannot run fails the script;
#   - fewer than three arguments are refused.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
fails=0

fail() { echo "FAIL: $1"; fails=$((fails+1)); }

check() { # check <name> <yes|no> <pattern> <file>
  if grep -qE "$3" "$4" 2> /dev/null; then got=yes; else got=no; fi
  [ "$got" = "$2" ] || fail "$1 (expected $2, got $got, pattern: $3)"
}

grep -q '^wait_limit=420 interval=10$' "$here/../read_settled_jobs.sh" \
  || { echo "FAIL: the wait's constants are not where this expects them"; exit 1; }
sed 's/^wait_limit=420 interval=10$/wait_limit=4 interval=1/' "$here/../read_settled_jobs.sh" > "$tmp/read_settled_jobs.sh"

# The n-th call serves $tmp/reads/<n>, or the last of them past the end. A read holding
# `fail` makes the call fail as gh does on an HTTP error: the error body on stdout, exit 1.
# A read holding `garbled` succeeds with output that is not JSON.
mkdir "$tmp/bin"
cat > "$tmp/bin/gh" <<'GH'
#!/bin/bash
n=$(( $(cat "$FAKE/calls" 2> /dev/null || echo 0) + 1 ))
echo "$n" > "$FAKE/calls"
echo "$*" > "$FAKE/arguments"
read=$FAKE/reads/$n
[ -f "$read" ] || read=$FAKE/reads/$(ls "$FAKE/reads" | sort -n | tail -1)
case "$(cat "$read")" in
  fail)    echo '{"message":"Server Error","status":"502"}'; exit 1;;
  garbled) echo '{"name": "t / A", "concl'; exit 0;;
esac
jq -c '.[]' "$read"
GH
chmod +x "$tmp/bin/gh"

pending='[{"name": "t / A", "conclusion": "success"}, {"name": "t / B", "conclusion": null}]'
settled='[{"name": "t / A", "conclusion": "success"}, {"name": "t / B", "conclusion": "failure"}]'
awaiting='[.[] | select(.conclusion == null) | .name] | join(", ")'

run() { # run <case> <read>... ; the reads are served in order
  local case=$1
  shift
  mkdir -p "$tmp/$case/reads"
  local n=1
  for read in "$@"; do echo "$read" > "$tmp/$case/reads/$n"; n=$((n+1)); done
  (cd "$tmp/$case" && FAKE=$tmp/$case PATH="$tmp/bin:$PATH" GH_REPO=owner/repo \
     bash "$tmp/read_settled_jobs.sh" 1 jobs.json "${filter:-$awaiting}" "${options[@]+"${options[@]}"}" > out.txt 2>&1)
  status=$?
  calls=$(cat "$tmp/$case/calls" 2> /dev/null || echo 0)
}

same_json() { [ "$(jq -c . "$1" 2> /dev/null)" = "$(jq -c . <<<"$2")" ]; }

options=()

run settled "$settled"
[ "$status" -eq 0 ] && [ "$calls" -eq 1 ] || fail "a settled read: status $status after $calls reads"
same_json "$tmp/settled/jobs.json" "$settled" || fail "a settled read is not the jobs file"
check "a settled read logs no wait" no "reading again" "$tmp/settled/out.txt"
[ "$(cat "$tmp/settled/arguments")" = "api repos/owner/repo/actions/runs/1/jobs?per_page=100 --paginate --jq .jobs[]" ] \
  || fail "the read asked for: $(cat "$tmp/settled/arguments")"

run pending_then_settled "$pending" "$settled"
[ "$status" -eq 0 ] && [ "$calls" -eq 2 ] || fail "a pending read, then a settled: status $status after $calls reads"
check "a pending read is logged, naming what is awaited" yes "awaited for t / B; reading again" "$tmp/pending_then_settled/out.txt"
same_json "$tmp/pending_then_settled/jobs.json" "$settled" || fail "the settled read is not the jobs file"

filter='[.[] | select(.name == $job and .conclusion == null) | .name] | join(", ")'
options=(--arg job "t / A")
run options_reach_jq "$pending"
[ "$status" -eq 0 ] && [ "$calls" -eq 1 ] || fail "the options after the filter did not reach jq: status $status after $calls reads"
unset filter
options=()

run fail_then_settled fail "$settled"
[ "$status" -eq 0 ] && [ "$calls" -eq 2 ] || fail "a failed read, then a settled: status $status after $calls reads"
check "a failed read is logged" yes "could not be read; reading again" "$tmp/fail_then_settled/out.txt"

run garbled_then_settled garbled "$settled"
[ "$status" -eq 0 ] && [ "$calls" -eq 2 ] || fail "a garbled read, then a settled: status $status after $calls reads"
same_json "$tmp/garbled_then_settled/jobs.json" "$settled" || fail "the settled read after a garbled one is not the jobs file"

run never "$pending"
[ "$status" -eq 0 ] || fail "a read that never settles: status $status"
[ "$calls" -ge 3 ] && [ "$calls" -le 6 ] || fail "a read that never settles was read $calls times in 4 s, a second apart"
check "a read that never settles ends in a warning naming what is awaited" yes "^::warning::.*awaited for t / B, 4s after the wait began" "$tmp/never/out.txt"
same_json "$tmp/never/jobs.json" "$pending" || fail "the last read is not the jobs file"

run pending_then_failing "$pending" fail
[ "$status" -eq 0 ] || fail "a pending read, then failing reads: status $status"
check "failing reads end in a warning" yes "^::warning::The jobs API could not be read" "$tmp/pending_then_failing/out.txt"
same_json "$tmp/pending_then_failing/jobs.json" "$pending" || fail "the last successful read is not the jobs file"

run pending_then_garbled "$pending" garbled
same_json "$tmp/pending_then_garbled/jobs.json" "$pending" || fail "a garbled read replaced the last successful read"

run always_failing fail
[ "$status" -ne 0 ] || fail "no read succeeded, yet the script succeeded"
check "no successful read is an error" yes "^::error::No read of the jobs API succeeded" "$tmp/always_failing/out.txt"

filter='.[] | no_such_function'
run bad_filter "$settled"
[ "$status" -ne 0 ] || fail "a filter jq cannot run did not fail the script"
unset filter

bash "$tmp/read_settled_jobs.sh" 1 jobs.json > /dev/null 2>&1
[ $? -eq 2 ] || fail "two arguments were not refused"

if [ "$fails" -eq 0 ]; then echo "read_settled_jobs: all controls pass"; else exit 1; fi
