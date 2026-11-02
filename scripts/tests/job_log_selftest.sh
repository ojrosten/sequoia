#!/bin/bash
# Controls for job_log.sh, against a fake curl which serves a canned log, and a
# mutation check of the controls.
#
#   job_log_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the script and against each mutant of the script.
# The script must fail no control, and each mutant at least one.
#
# Each control is a claim the script exists to keep:
#   - the log asked for is that of the job named, in $GH_REPO, at
#     $GITHUB_API_URL or else api.github.com, with $GH_TOKEN, following
#     redirects;
#   - each line is written without its time stamp, and with the rest of the
#     line as GitHub gave it: the spaces after the stamp, and a line of a
#     step's script starting with its escape sequence;
#   - a line without a time stamp is written whole;
#   - a log which cannot be read fails the script;
#   - arguments other than one job id, and an unset or empty $GH_TOKEN or
#     $GH_REPO, are refused before any request, with the usage on standard
#     error.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script=$here/../job_log.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

case "$*" in
  ''|--mutations) ;;
  *) echo "Usage: $0 [--mutations]" >&2; exit 2;;
esac

# The fake curl writes its arguments, one per line, to $FAKE/arguments, and
# serves $FAKE/log, or fails as curl -f does on an HTTP error if there is none.
mkdir "$tmp/bin"
cat > "$tmp/bin/curl" <<'CURL'
#!/bin/bash
printf '%s\n' "$@" > "$FAKE/arguments"
[ -f "$FAKE/log" ] || { echo 'curl: (22) The requested URL returned error: 404' >&2; exit 22; }
cat "$FAKE/log"
CURL
chmod +x "$tmp/bin/curl"

escape=$'\e'
printf '%s\n' \
  '2026-11-01T22:39:25.2381229Z ##[group]Run verdict="$RUNNER_TEMP/verdict.txt"' \
  "2026-11-01T22:39:25.2381483Z ${escape}[36;1merror: a line of the script${escape}[0m" \
  '2026-11-01T22:39:25.2718667Z notice: symbols: 710693 -> 710693, +0 (+0.00%)' \
  '2026-11-01T22:39:25.2719341Z   indented 2026-11-01T22:39:25.2719341Z text' \
  'no stamp here' > "$tmp/log"
expected=$(printf '%s\n' \
  '##[group]Run verdict="$RUNNER_TEMP/verdict.txt"' \
  "${escape}[36;1merror: a line of the script${escape}[0m" \
  'notice: symbols: 710693 -> 710693, +0 (+0.00%)' \
  '  indented 2026-11-01T22:39:25.2719341Z text' \
  'no stamp here')

fail() { echo "FAIL: $1"; fails=$((fails+1)); }

# run <case> <log or ''> <env argument>...
# Runs `env <env argument>... bash <script>` in a directory of its own, $dir,
# serving <log>, or no log if it is empty. Sets $status, $out and $err.
run() {
  dir=$tmp/cases/$1
  rm -rf "$dir"
  mkdir -p "$dir"
  [ -z "$2" ] || cp "$2" "$dir/log"
  shift 2
  out=$(cd "$dir" && FAKE=$dir PATH="$tmp/bin:$PATH" env "$@" 2> "$dir/err")
  status=$?
  err=$(cat "$dir/err")
}

controls() {
  subject=$1 fails=0
  local token=GH_TOKEN=secret repo=GH_REPO=owner/repo

  run read "$tmp/log" -u GITHUB_API_URL "$token" "$repo" bash "$subject" 42
  [ "$status" -eq 0 ] || fail "a log read: status $status, err '$err'"
  [ "$out" = "$expected" ] || fail "a log read: output '$out'"
  grep -qxF 'https://api.github.com/repos/owner/repo/actions/jobs/42/logs' "$dir/arguments" \
    || fail "the URL asked for: '$(cat "$dir/arguments")'"
  grep -qxF 'Authorization: Bearer secret' "$dir/arguments" || fail "no token: '$(cat "$dir/arguments")'"
  grep -qxE -- '-[a-zA-Z]*L[a-zA-Z]*' "$dir/arguments" || fail "redirects not followed: '$(cat "$dir/arguments")'"
  grep -qxE -- '-[a-zA-Z]*f[a-zA-Z]*' "$dir/arguments" || fail "an HTTP error not a failure: '$(cat "$dir/arguments")'"

  run server "$tmp/log" GITHUB_API_URL=https://ghe.example/api/v3 "$token" "$repo" bash "$subject" 42
  grep -qxF 'https://ghe.example/api/v3/repos/owner/repo/actions/jobs/42/logs' "$dir/arguments" \
    || fail "the server named: '$(cat "$dir/arguments")'"

  run absent '' "$token" "$repo" bash "$subject" 42
  [ "$status" -ne 0 ] || fail "a log which cannot be read: status 0"

  for arguments in '' '42 43' "''"; do
    eval "run refused '$tmp/log' $token $repo bash $subject $arguments"
    [ "$status" -eq 2 ] && [ -z "$out" ] && [[ $err == Usage:* ]] && [ ! -e "$dir/arguments" ] \
      || fail "arguments '$arguments': status $status, err '$err'"
  done
  for environment in "-u GH_TOKEN $repo" "GH_TOKEN= $repo" "-u GH_REPO $token" "$token GH_REPO="; do
    eval "run refused '$tmp/log' $environment bash $subject 42"
    [ "$status" -eq 2 ] && [ ! -e "$dir/arguments" ] || fail "environment '$environment': status $status"
  done
}

# Each mutant breaks one behaviour that the controls claim. Its entry holds a
# description, the text it replaces, and the replacement.
mutations=(
  'stamps kept'               "| sed -E 's/^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9:.]+Z //'" ''
  'every stamp removed'       "| sed -E 's/^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9:.]+Z //'" "| sed -E 's/[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9:.]+Z //g'"
  'leading spaces removed'    "| sed -E 's/^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9:.]+Z //'" "| sed -E 's/^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9:.]+Z +//'"
  'escapes removed'           "| sed -E 's/^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9:.]+Z //'" "| sed -E 's/^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9:.]+Z //; s/\x1b\[[0-9;]*m//g'"
  'unstamped lines dropped'   "| sed -E 's/^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9:.]+Z //'" "| sed -nE 's/^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9:.]+Z //p'"
  'a failed read passes'      'set -euo pipefail' 'set -eu'
  'no token'                  '-H "Authorization: Bearer $GH_TOKEN" ' ''
  'redirects not followed'    'curl -fsSL' 'curl -fs'
  'the server fixed'          '${GITHUB_API_URL:-https://api.github.com}' 'https://api.github.com'
  'any arguments'             '[ $# -ne 1 ] || ' ''
  'an empty job id taken'     '[ -z "$1" ] || ' ''
  'GH_TOKEN not needed'       ' || [ -z "${GH_TOKEN:-}" ]' ''
  'GH_REPO not needed'        ' || [ -z "${GH_REPO:-}" ]' ''
)

if [ "$*" != --mutations ]; then
  controls "$script"
  if [ "$fails" -eq 0 ]; then echo "job_log: all controls pass"; else exit 1; fi
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
