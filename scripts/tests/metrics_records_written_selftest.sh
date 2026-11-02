#!/bin/bash
# Controls for metrics_records_written.py, against a data branch built here,
# and a mutation check of the controls.
#
#   metrics_records_written_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the script and against each mutant of the script.
# The script must fail no control, and each mutant at least one.
#
# Each control is a claim the script exists to keep:
#   - records holding every field named pass, each commit named on a line of
#     its own, among them a field nested two deep and a count of zero;
#   - a commit with no record fails, naming the commit and the path, and so
#     does a record in another directory, or on another branch;
#   - a record lacking a field fails, naming the field, as does a field
#     holding a boolean, a fraction, text or an object;
#   - a record naming another commit, or with a clean which is not true,
#     fails;
#   - a record which is not JSON, or holds no object, fails;
#   - one commit failing fails the run, whichever of the commits it is, and
#     the others are still judged;
#   - a data branch which names no commit is refused with status 2, as are
#     arguments of any other form.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script=$here/../metrics_records_written.py
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

case "$*" in
  ''|--mutations) ;;
  *) echo "Usage: $0 [--mutations]" >&2; exit 2;;
esac

# The repository: on `data`, a record of each commit named below under
# records/, and one of `elsewhere` under other/; on `other`, a record of
# `on_other` alone.
repository=$tmp/repository
git init -q -b data "$repository"
git -C "$repository" config user.name selftest
git -C "$repository" config user.email selftest@example.invalid
mkdir -p "$repository/records" "$repository/other"
record() { # record <file> <JSON>
  printf '%s\n' "$2" > "$repository/$1"
}
record records/good.json        '{"commit": "good", "clean": true, "count": 3, "section": {"inner": {"count": 0}}}'
record records/second.json      '{"commit": "second", "clean": true, "count": 7, "section": {"inner": {"count": 1}}}'
record records/no_count.json    '{"commit": "no_count", "clean": true, "section": {"inner": {"count": 1}}}'
record records/no_inner.json    '{"commit": "no_inner", "clean": true, "count": 1, "section": {}}'
record records/boolean.json     '{"commit": "boolean", "clean": true, "count": true, "section": {"inner": {"count": 1}}}'
record records/fraction.json    '{"commit": "fraction", "clean": true, "count": 1.5, "section": {"inner": {"count": 1}}}'
record records/text.json        '{"commit": "text", "clean": true, "count": "3", "section": {"inner": {"count": 1}}}'
record records/object.json      '{"commit": "object", "clean": true, "count": {}, "section": {"inner": {"count": 1}}}'
record records/misnamed.json    '{"commit": "good", "clean": true, "count": 1, "section": {"inner": {"count": 1}}}'
record records/unclean.json     '{"commit": "unclean", "clean": false, "count": 1, "section": {"inner": {"count": 1}}}'
record records/clean_text.json  '{"commit": "clean_text", "clean": "true", "count": 1, "section": {"inner": {"count": 1}}}'
record records/garbage.json     '{"commit": "garbage", "clea'
record records/array.json       '[1, 2]'
record other/elsewhere.json     '{"commit": "elsewhere", "clean": true, "count": 1, "section": {"inner": {"count": 1}}}'
git -C "$repository" add -A
git -C "$repository" commit -q -m records
git -C "$repository" checkout -q --orphan other
git -C "$repository" rm -q -rf .
mkdir -p "$repository/records"
record records/on_other.json    '{"commit": "on_other", "clean": true, "count": 1, "section": {"inner": {"count": 1}}}'
git -C "$repository" add -A
git -C "$repository" commit -q -m other
git -C "$repository" checkout -q data

fail() { echo "FAIL: $1"; fails=$((fails+1)); }

# judge <description> <status> <expected output> <commit>...
# Runs the script on `data`, under records/, for the fields `count` and
# `section.inner.count`, and checks its status and its whole output, which
# must match <expected output> as a glob pattern.
judge() {
  local description=$1 expected_status=$2 expected=$3
  shift 3
  out=$(cd "$repository" && python3 -B "$subject" --data data --directory records \
          --field count section.inner.count --commit "$@" 2>&1)
  status=$?
  [ "$status" -eq "$expected_status" ] || fail "$description: status $status, not $expected_status: '$out'"
  [[ $out == $expected ]] || fail "$description: output '$out', not '$expected'"
}

# refused <description> <argument>...
refused() {
  local description=$1
  shift
  out=$(cd "$repository" && python3 -B "$subject" "$@" 2>&1 > /dev/null)
  status=$?
  [ "$status" -eq 2 ] || fail "$description: status $status, not 2"
  [ -n "$out" ] || fail "$description: nothing on standard error"
}

ok() { echo "ok: $1: records/$1.json holds count, section.inner.count"; }

# controls <script>
# Runs the controls against <script>, and counts in $fails those that fail.
controls() {
  subject=$1 fails=0
  judge "every field held" 0 "$(ok good)" good
  judge "two commits" 0 "$(ok good)"$'\n'"$(ok second)" good second
  judge "no record" 1 "error: absent: data holds no records/absent.json" absent
  judge "a record in another directory" 1 "error: elsewhere: data holds no records/elsewhere.json" elsewhere
  judge "a record on another branch" 1 "error: on_other: data holds no records/on_other.json" on_other
  judge "a field absent" 1 "error: no_count: its record has no whole number at count" no_count
  judge "a nested field absent" 1 "error: no_inner: its record has no whole number at section.inner.count" no_inner
  judge "a boolean" 1 "error: boolean: its record has no whole number at count" boolean
  judge "a fraction" 1 "error: fraction: its record has no whole number at count" fraction
  judge "text" 1 "error: text: its record has no whole number at count" text
  judge "an object" 1 "error: object: its record has no whole number at count" object
  judge "another commit named" 1 "error: misnamed: its record names the commit 'good'" misnamed
  judge "not clean" 1 "error: unclean: its record has a clean of False, not true" unclean
  judge "a clean of text" 1 "error: clean_text: its record has a clean of 'true', not true" clean_text
  judge "not JSON" 1 "error: garbage: its record is not JSON: *" garbage
  judge "no object" 1 "error: array: its record holds no JSON object" array
  judge "the first of two failing" 1 "error: absent: data holds no records/absent.json"$'\n'"$(ok good)" absent good
  judge "the second of two failing" 1 "$(ok good)"$'\n'"error: absent: data holds no records/absent.json" good absent

  refused "a data branch naming no commit" --data nothing --directory records --field count --commit good
  refused "no commit" --data data --directory records --field count --commit
  refused "no field" --data data --directory records --commit good
  refused "no directory" --data data --field count --commit good
  refused "no data branch" --directory records --field count --commit good
}

# Each mutant breaks one behaviour that the controls claim. Its entry holds a
# description, the text it replaces, and the replacement.
mutations=(
  'the commit not compared'       '([] if record.get("commit") == commit' '([] if True'
  'any clean passes'              '([] if record.get("clean") is True' '([] if record.get("clean")'
  'a boolean is a number'         'return isinstance(value, int) and not isinstance(value, bool)' 'return isinstance(value, int)'
  'a fraction is a number'        'return isinstance(value, int) and not isinstance(value, bool)' 'return isinstance(value, (int, float)) and not isinstance(value, bool)'
  'only the top of a field read'  'value_at(record, field.split("."))' 'value_at(record, field.split(".")[:1])'
  'no field checked'              'for field in fields
            if not' 'for field in fields[:0]
            if not'
  'only the first field checked'  'for field in fields
            if not' 'for field in fields[:1]
            if not'
  'not JSON passes'               '        return [f"its record is not JSON: {e}"]' '        return []'
  'no object passes'              '    if not isinstance(record, dict):
        return' '    if False:
        return'
  'an absent record passes'       'faults = ([f"{arguments.data} holds no {path}"] if shown.returncode' 'faults = ([] if shown.returncode'
  'the first failure forgotten'   'failed = failed or bool(faults)' 'failed = bool(faults)'
  'judging stops at a failure'    '              else f"ok: {commit}: {path} holds {'"', '"'.join(arguments.field)}")' '              else f"ok: {commit}: {path} holds {'"', '"'.join(arguments.field)}")
        if faults: break'
  'a failure exits 0'             'return 1 if failed else 0' 'return 0'
  'a branch naming nothing judged' '        return 2
    failed' '        pass
    failed'
  'commits optional'              'parser.add_argument("--commit", required=True, nargs="+")' 'parser.add_argument("--commit", required=True, nargs="*")'
)

if [ "$*" != --mutations ]; then
  controls "$script"
  if [ "$fails" -eq 0 ]; then echo "metrics_records_written: all controls pass"; else exit 1; fi
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
  printf '%s\n' "${source_text%%"$old"*}$new$after" > "$tmp/mutant.py"
  controls "$tmp/mutant.py" > "$tmp/controls.txt" 2>&1
  echo "$description: $fails controls fail$([ "$fails" -ne 0 ] || echo '  <-- SURVIVED')"
  [ "$fails" -ne 0 ] || survivors=$((survivors+1))
done
[ "$survivors" -eq 0 ]
