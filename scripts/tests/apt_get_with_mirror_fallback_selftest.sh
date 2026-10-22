#!/bin/bash
# Controls for apt_get_with_mirror_fallback.sh, and a mutation check of the
# controls.
#
#   apt_get_with_mirror_fallback_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls against the script. With
# it, the selftest runs them against the script and against each mutant of the
# script: the script must fail no control, and each mutant at least one.
#
# Each control runs the script on a fixture standing in for /etc/apt, with a
# stand-in for apt-get which records its arguments and exits with a status the
# control chooses. The fixture's mirror list is the hosted images' (144 bytes
# on ubuntu-24.04 and ubuntu-26.04, on 2026-10-22). Each control is a claim the
# script exists to keep:
#   - on the first attempt, apt-get runs with the arguments given, and with
#     Acquire::Progress::Ignore::ShowErrorText set, and the list is unchanged;
#   - on a later attempt, every mirror of the smallest priority gets a
#     priority larger than every other mirror's, and the rest of the list,
#     comments and other tags included, is unchanged;
#   - the image's list is kept as <list>.image, and every rewrite starts from
#     it, so a third attempt, and a later command's, write the same list;
#   - a later command's first attempt leaves the rewritten list in place;
#   - the sources name the list in either format, .sources or .list, and a
#     commented-out source names nothing;
#   - apt-get's status is the script's;
#   - the script refuses with status 2 and an ::error::, running nothing and
#     changing nothing, on every input from which it cannot tell which mirror
#     apt prefers, on the first attempt as on a later one.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
original="$here/../apt_get_with_mirror_fallback.sh"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/apt_get_with_mirror_fallback_selftest.XXXXXX")
trap 'rm -rf "$tmp"' EXIT

# The hosted images' list, whose lines are separated from their tags by tabs.
image_list=$(printf '%s\t%s\n' \
  http://azure.archive.ubuntu.com/ubuntu/ priority:1 \
  https://archive.ubuntu.com/ubuntu/      priority:2 \
  https://security.ubuntu.com/ubuntu/     priority:3)
demoted_list=$(printf '%s\t%s\n' \
  http://azure.archive.ubuntu.com/ubuntu/ priority:4 \
  https://archive.ubuntu.com/ubuntu/      priority:2 \
  https://security.ubuntu.com/ubuntu/     priority:3)

cat > "$tmp/apt-get" <<'STANDIN'
#!/bin/bash
printf '%s\n' "$@" > "$(dirname "$0")/arguments"
exit "${STANDIN_STATUS:-0}"
STANDIN
chmod +x "$tmp/apt-get"

fails=0
fail() { echo "FAIL: $1"; fails=$((fails + 1)); }

expect() { # expect <claim> <actual> <expected>
  [ "$2" = "$3" ] || fail "$1 (expected '$3', got '$2')"
}

has() { # has <claim> <yes|no> <pattern> <file>
  local got=no
  grep -E -- "$3" "$4" > /dev/null && got=yes
  [ "$got" = "$2" ] || fail "$1 (expected $2, pattern: $3)"
}

# Makes a fresh fixture: an etc directory whose sources name its list in the
# deb822 format, and the list given.
fixture() { # fixture <list>
  rm -rf "$tmp/etc" "$tmp/arguments"
  mkdir -p "$tmp/etc/sources.list.d"
  etc=$tmp/etc
  list=$etc/apt-mirrors.txt
  printf '%s\n' "$1" > "$list"
  printf 'Types: deb\nURIs: mirror+file:%s\nSuites: noble\nComponents: main\n' "$list" \
    > "$etc/sources.list.d/ubuntu.sources"
}

# Runs the script under test: attempt <number> <arguments>...
attempt() {
  local number=$1
  shift
  rm -f "$tmp/arguments"
  if [ "$number" = unset ]; then
    env -u RETRY_ATTEMPT APT_ETC="$etc" APT_GET="$tmp/apt-get" bash "$script" "$@" > "$tmp/out" 2> "$tmp/err"
  else
    RETRY_ATTEMPT=$number APT_ETC="$etc" APT_GET="$tmp/apt-get" bash "$script" "$@" > "$tmp/out" 2> "$tmp/err"
  fi
  status=$?
}

# A refusal: status 2, an ::error:: containing <message>, apt-get not run, and
# the list as it was.
refused() { # refused <claim> <message>
  expect "$1: refused with status 2" "$status" 2
  has "$1: an error names why" yes "^::error::.*$2" "$tmp/out"
  [ -e "$tmp/arguments" ] && fail "$1: apt-get does not run"
  [ -e "$list" ] && [ "$(cat "$list")" != "$listed" ] && fail "$1: the list is unchanged"
}

run_controls() {
  fails=0

  fixture "$image_list"
  attempt 1 install -y "two words" ""
  expect "the first attempt runs apt-get, with status 0" "$status" 0
  expect "apt-get gets the error text option, then the arguments intact" "$(cat "$tmp/arguments" 2> /dev/null)" \
    "$(printf '%s\n' -o Acquire::Progress::Ignore::ShowErrorText=true install -y "two words" "")"
  expect "the first attempt leaves the list unchanged" "$(cat "$list")" "$image_list"
  [ -e "$list.image" ] && fail "the first attempt keeps no copy of the list"
  has "the first attempt prints nothing" no '.' "$tmp/out"
  has "the first attempt writes nothing on standard error" no '.' "$tmp/err"

  attempt 2 update -qq
  expect "a later attempt runs apt-get, with status 0" "$status" 0
  expect "a later attempt gives the preferred mirror the largest priority" "$(cat "$list")" "$demoted_list"
  expect "a later attempt keeps the image's list" "$(cat "$list.image" 2> /dev/null)" "$image_list"
  has "a later attempt says so" yes '^Attempt 2 fetches from the image.s other mirrors first:$' "$tmp/out"
  has "a later attempt prints the list" yes 'azure.archive.ubuntu.com/ubuntu/.priority:4$' "$tmp/out"
  expect "a later attempt passes the arguments" "$(tail -n 1 "$tmp/arguments" 2> /dev/null)" -qq

  attempt 3 update
  expect "a third attempt writes the same list" "$(cat "$list")" "$demoted_list"
  expect "a third attempt keeps the image's list" "$(cat "$list.image" 2> /dev/null)" "$image_list"

  attempt 1 install gdb
  expect "a later command's first attempt leaves the rewritten list" "$(cat "$list")" "$demoted_list"
  expect "a later command's first attempt keeps the image's list" "$(cat "$list.image" 2> /dev/null)" "$image_list"

  fixture "$image_list"
  STANDIN_STATUS=100 attempt 1 install gdb
  expect "apt-get's failure is the script's" "$status" 100
  STANDIN_STATUS=100 attempt 2 install gdb
  expect "apt-get's failure on a later attempt is the script's" "$status" 100

  # Two mirrors share the smallest priority; comments, blank lines and other
  # tags survive the rewrite.
  local tied tied_demoted
  tied=$(printf '# a comment\n\n%s\t%s\n%s\t%s\n%s\t%s\n' \
    http://a.example/ "arch:amd64  priority:5 lang:en" \
    http://b.example/ priority:5 \
    http://c.example/ priority:9)
  tied_demoted=$(printf '# a comment\n\n%s\t%s\n%s\t%s\n%s\t%s\n' \
    http://a.example/ "arch:amd64 priority:10 lang:en" \
    http://b.example/ priority:10 \
    http://c.example/ priority:9)
  fixture "$tied"
  attempt 2 update
  expect "every mirror of the smallest priority is moved after the rest, and the rest of the list kept" \
    "$(cat "$list")" "$tied_demoted"

  # The sources in the one-line format, with a commented-out second list.
  fixture "$image_list"
  rm "$etc/sources.list.d/ubuntu.sources"
  printf '# deb mirror+file:/elsewhere noble main\ndeb mirror+file:%s noble main\n' "$list" > "$etc/sources.list"
  attempt 2 update
  expect "a list named by a .list source is read, and a commented-out one ignored" "$(cat "$list")" "$demoted_list"
  fixture "$image_list"
  mv "$etc/sources.list.d/ubuntu.sources" "$etc/sources.list.d/ubuntu.list"
  sed -i.bak 's/^URIs: /deb /' "$etc/sources.list.d/ubuntu.list"
  rm "$etc/sources.list.d/ubuntu.list.bak"
  attempt 2 update
  expect "a list named in sources.list.d/*.list is read" "$(cat "$list")" "$demoted_list"

  # The refusals, on the first attempt and on a later one.
  local number value
  for value in unset "" 0 02 2x -1; do
    fixture "$image_list"
    listed=$(cat "$list")
    attempt "$value" update
    refused "RETRY_ATTEMPT '$value'" "RETRY_ATTEMPT is"
  done

  for number in 1 2; do
    fixture "$image_list"
    listed=$(cat "$list")
    rm "$etc/sources.list.d/ubuntu.sources"
    attempt "$number" update
    refused "attempt $number with no sources" "name no mirror list"

    fixture "$image_list"
    listed=$(cat "$list")
    sed -i.bak 's/^URIs:/# URIs:/' "$etc/sources.list.d/ubuntu.sources"
    attempt "$number" update
    refused "attempt $number with the source commented out" "name no mirror list"

    fixture "$image_list"
    listed=$(cat "$list")
    printf 'deb mirror+file:%s/other noble main\n' "$etc" > "$etc/sources.list"
    attempt "$number" update
    refused "attempt $number with two lists" "more than one mirror list"

    fixture "$image_list"
    listed=""
    rm "$list"
    attempt "$number" update
    refused "attempt $number with no list" "is not a file"

    local bad
    for bad in "http://a.example/" "http://a.example/	arch:amd64" \
               "http://a.example/	priority:1 priority:3" "http://a.example/	priority:high"; do
      fixture "$(printf '%s\t%s\n%s\n' http://z.example/ priority:2 "$bad")"
      listed=$(cat "$list")
      attempt "$number" update
      refused "attempt $number with '$bad'" "line 2 gives its mirror no priority"
    done

    fixture "$(printf '%s\t%s\n%s\t%s\n' http://a.example/ priority:1 http://b.example/ priority:1)"
    listed=$(cat "$list")
    attempt "$number" update
    refused "attempt $number with one priority for every mirror" "every mirror has the same priority"

    fixture "# nothing but a comment"
    listed=$(cat "$list")
    attempt "$number" update
    refused "attempt $number with no mirror in the list" "it names no mirror"
  done
  return "$fails"
}

# Each mutant is a description, the text to replace, and its replacement.
mutants=(
  'the attempt not checked'        '[[ ${RETRY_ATTEMPT-} =~ ^[1-9][0-9]*$ ]]'  'true'
  'a leading zero accepted'        '^[1-9][0-9]*$'  '^[0-9]+$'
  'never demoted'                  'if ((n in priority) && priority[n] == least) {'  'if (0) {'
  'rewritten on the first attempt' 'if [ "$RETRY_ATTEMPT" -gt 1 ]; then'  'if true; then'
  'the list not rewritten'         'printf '"'"'%s\n'"'"' "$rewritten" > "$list"'  'printf '"'"'%s\n'"'"' "$rewritten" > /dev/null'
  'the image not kept'             '[ -e "$list.image" ] || cp -p "$list" "$list.image"'  ':'
  'rewritten from the list'        '[ -e "$image" ] || image=$list'  'image=$list'
  'one tied mirror demoted'        'priority[n] == least) {'  'priority[n] == least && !done++) {'
  'demoted to the largest'         'tags[i] = "priority:" (most + 1)'  'tags[i] = "priority:" most'
  'the largest demoted'            'priority[NR] < least) least'  'priority[NR] > least) least'
  'other tags dropped'             '          if (tags[i] ~ /^priority:/) tags[i]'  '          if (tags[i] !~ /^priority:/) continue
          tags[i]'
  'ties accepted'                  'least == most) problem'  'least == most && 0) problem'
  'no priority accepted'           'priorities != 1 ||'  'priorities > 1 ||'
  'two priorities accepted'        'priorities != 1 ||'  'priorities == 0 ||'
  'a word accepted as a priority'  'value !~ /^[0-9]+$/'  '0'
  'an empty list accepted'         'mirrors == 0) problem'  '0) problem'
  'comments read as mirrors'       '/^#/ || $0 == ""'  '$0 == ""'
  'commented sources read'         "grep -v '^[[:space:]]*#'"  'cat'
  'no list accepted'               '  0) refuse'  '  0) ;; 00) refuse'
  'two lists accepted'             '  *) refuse'  '  *) ;; 00) refuse'
  'a missing list accepted'        '[ -f "$image" ] || refuse'  'true || refuse'
  '.sources not read'              ' "$etc"/sources.list.d/*.sources'  ''
  '.list not read'                 ' "$etc"/sources.list.d/*.list'  ''
  'sources.list not read'          ' "$etc"/sources.list "$etc"'  ' "$etc"'
  'no error text'                  ' -o Acquire::Progress::Ignore::ShowErrorText=true'  ''
  'arguments joined'               '=true "$@"'  '=true $*'
  'a refusal not an error'         'echo "::error::$0: $1"'  'echo "$0: $1"'
  'a refusal of status 1'          '  exit 2'  '  exit 1'
  'a later attempt silent'         'echo "Attempt $RETRY_ATTEMPT fetches'  'true "Attempt $RETRY_ATTEMPT fetches'
)

mutations() {
  local i survivors=0 content stripped occurrences failed
  script=$original
  run_controls > "$tmp/controls.log" 2>&1
  echo "unmutated: $(grep -c '^FAIL' "$tmp/controls.log") controls fail"
  grep '^FAIL' "$tmp/controls.log" > /dev/null && { cat "$tmp/controls.log"; survivors=1; }
  for ((i = 0; i < ${#mutants[@]}; i += 3)); do
    content=$(cat "$original"; echo x)
    content=${content%x}
    stripped=${content//"${mutants[i+1]}"/}
    occurrences=$(( (${#content} - ${#stripped}) / ${#mutants[i+1]} ))
    if [ "$occurrences" -ne 1 ]; then
      echo "${mutants[i]}: the text to mutate occurs $occurrences times  <-- must be once"
      survivors=1
      continue
    fi
    printf '%s' "${content%%"${mutants[i+1]}"*}${mutants[i+2]}${content#*"${mutants[i+1]}"}" > "$tmp/mutant.sh"
    script="$tmp/mutant.sh"
    run_controls > "$tmp/controls.log" 2>&1
    failed=$(grep -c '^FAIL' "$tmp/controls.log")
    if [ "$failed" -gt 0 ]; then
      echo "${mutants[i]}: $failed controls fail"
    else
      echo "${mutants[i]}: SURVIVED"
      survivors=1
    fi
  done
  return "$survivors"
}

case "${1-}" in
  --mutations) mutations; exit ;;
  "")          ;;
  *)           echo "Usage: $0 [--mutations]" >&2; exit 2 ;;
esac

script=$original
run_controls
if [ "$fails" -eq 0 ]; then echo "apt_get_with_mirror_fallback: all controls pass"; else exit 1; fi
