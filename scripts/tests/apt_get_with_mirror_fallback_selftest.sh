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
# on ubuntu-24.04 and ubuntu-26.04, on 2026-10-22), and its sources name the
# list twice, as the images' do. Each control is a claim the script exists to
# keep:
#   - on the first attempt, apt-get runs with the arguments given, and with
#     Acquire::Progress::Ignore::ShowErrorText set, and the list is unchanged;
#   - on a later attempt, every mirror of the smallest priority gets a
#     priority larger than every other mirror's, and the rest of the list,
#     comments and other tags included, is unchanged;
#   - the image's list is kept as <list>.image, and every rewrite starts from
#     that copy, so a third attempt, and a later command's second, write the
#     same list;
#   - a later command's first attempt leaves the rewritten list in place;
#   - the sources name the list in either format, .sources or .list, and
#     neither a commented-out source nor a comment after a source names a
#     list;
#   - apt-get's status is the script's;
#   - the script refuses with status 2 and an ::error::, running nothing and
#     changing nothing, on each input below from which it cannot tell which
#     mirror apt prefers, on the first attempt as on a later one.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
original="$here/../apt_get_with_mirror_fallback.sh"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/apt_get_with_mirror_fallback_selftest.XXXXXX")
trap 'rm -rf "$tmp"' EXIT

# The hosted images' list, in which a tab separates each mirror from its tags.
image_contents=$(printf '%s\t%s\n' \
  http://azure.archive.ubuntu.com/ubuntu/ priority:1 \
  https://archive.ubuntu.com/ubuntu/      priority:2 \
  https://security.ubuntu.com/ubuntu/     priority:3)
demoted_contents=$(printf '%s\t%s\n' \
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
  grep -qE -- "$3" "$4" && got=yes
  [ "$got" = "$2" ] || fail "$1 (expected $2, pattern: $3)"
}

# Makes a fresh fixture: an etc directory whose sources name its list in two
# stanzas of the deb822 format, and the list given.
make_fixture() { # make_fixture <contents of the list>
  rm -rf "$tmp/etc" "$tmp/arguments"
  mkdir -p "$tmp/etc/sources.list.d"
  etc=$tmp/etc
  mirror_list=$etc/apt-mirrors.txt
  printf '%s\n' "$1" > "$mirror_list"
  local suite
  for suite in noble noble-security; do
    printf 'Types: deb\nURIs: mirror+file:%s\nSuites: %s\nComponents: main\n\n' "$mirror_list" "$suite"
  done > "$etc/sources.list.d/ubuntu.sources"
}

# Runs the script under test, with RETRY_ATTEMPT set to <number>, or unset, and
# records its output and status: run_at_attempt <number> <arguments>...
run_at_attempt() {
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
expect_refusal() { # expect_refusal <claim> <message>
  expect "$1: refused with status 2" "$status" 2
  has "$1: an error names why" yes "^::error::.*$2" "$tmp/out"
  [ -e "$tmp/arguments" ] && fail "$1: apt-get does not run"
  [ -e "$mirror_list" ] && [ "$(cat "$mirror_list")" != "$mirror_list_before" ] && fail "$1: the list is unchanged"
}

run_controls() {
  fails=0

  make_fixture "$image_contents"
  run_at_attempt 1 install -y "two words" ""
  expect "the first attempt runs apt-get, with status 0" "$status" 0
  expect "apt-get gets the error text option, then the arguments intact" "$(cat "$tmp/arguments" 2> /dev/null)" \
    "$(printf '%s\n' -o Acquire::Progress::Ignore::ShowErrorText=true install -y "two words" "")"
  expect "the first attempt leaves the list unchanged" "$(cat "$mirror_list")" "$image_contents"
  [ -e "$mirror_list.image" ] && fail "the first attempt keeps no copy of the list"
  has "the first attempt prints nothing" no '.' "$tmp/out"
  has "the first attempt writes nothing on standard error" no '.' "$tmp/err"

  run_at_attempt 2 update -qq
  expect "a later attempt runs apt-get, with status 0" "$status" 0
  expect "a later attempt gives the preferred mirror the largest priority" "$(cat "$mirror_list")" "$demoted_contents"
  expect "a later attempt keeps the image's list" "$(cat "$mirror_list.image" 2> /dev/null)" "$image_contents"
  has "a later attempt says so" yes '^Attempt 2 fetches from the image.s other mirrors first:$' "$tmp/out"
  has "a later attempt prints the list" yes 'azure.archive.ubuntu.com/ubuntu/.priority:4$' "$tmp/out"
  expect "a later attempt passes the arguments" "$(tail -n 1 "$tmp/arguments" 2> /dev/null)" -qq

  run_at_attempt 3 update
  expect "a third attempt writes the same list" "$(cat "$mirror_list")" "$demoted_contents"
  expect "a third attempt keeps the image's list" "$(cat "$mirror_list.image" 2> /dev/null)" "$image_contents"

  run_at_attempt 1 install gdb
  expect "a later command's first attempt leaves the rewritten list" "$(cat "$mirror_list")" "$demoted_contents"
  expect "a later command's first attempt keeps the image's list" "$(cat "$mirror_list.image" 2> /dev/null)" "$image_contents"
  run_at_attempt 2 install gdb
  expect "a later command's second attempt writes the same list" "$(cat "$mirror_list")" "$demoted_contents"
  expect "a later command's second attempt keeps the image's list" "$(cat "$mirror_list.image" 2> /dev/null)" \
    "$image_contents"

  make_fixture "$image_contents"
  STANDIN_STATUS=100 run_at_attempt 1 install gdb
  expect "apt-get's failure is the script's" "$status" 100
  STANDIN_STATUS=100 run_at_attempt 2 install gdb
  expect "apt-get's failure on a later attempt is the script's" "$status" 100

  # Two mirrors share the smallest priority; comments, blank lines and other
  # tags survive the rewrite.
  local tied tied_demoted
  tied=$(printf '# a comment\n\n%s\t%s\n%s\t%s\n%s\t%s\n' \
    http://a.example/ "arch:amd64  priority:5 lang:en" \
    http://b.example/ priority:5 \
    http://c.example/ priority:9)
  tied_demoted=$(printf '# a comment\n\n%s\t%s\n%s\t%s\n%s\t%s\n' \
    http://a.example/ "arch:amd64  priority:10 lang:en" \
    http://b.example/ priority:10 \
    http://c.example/ priority:9)
  make_fixture "$tied"
  run_at_attempt 2 update
  expect "every mirror of the smallest priority is moved after the rest, and the rest of the list kept" \
    "$(cat "$mirror_list")" "$tied_demoted"

  # The sources in the one-line format, with a second list commented out, and a
  # third named in a comment after a source.
  make_fixture "$image_contents"
  rm "$etc/sources.list.d/ubuntu.sources"
  printf '  # deb mirror+file:/elsewhere noble main\ndeb mirror+file:%s noble main # mirror+file:/other\n' \
    "$mirror_list" > "$etc/sources.list"
  run_at_attempt 2 update
  expect "a list named by a .list source is read, and the lists in comments ignored" "$(cat "$mirror_list")" \
    "$demoted_contents"
  make_fixture "$image_contents"
  mv "$etc/sources.list.d/ubuntu.sources" "$etc/sources.list.d/ubuntu.list"
  sed -i.bak 's/^URIs: /deb /' "$etc/sources.list.d/ubuntu.list"
  rm "$etc/sources.list.d/ubuntu.list.bak"
  run_at_attempt 2 update
  expect "a list named in sources.list.d/*.list is read" "$(cat "$mirror_list")" "$demoted_contents"

  # The refusals, on the first attempt and on a later one.
  local number value
  for value in unset "" 0 02 2x -1; do
    make_fixture "$image_contents"
    mirror_list_before=$(cat "$mirror_list")
    run_at_attempt "$value" update
    expect_refusal "RETRY_ATTEMPT '$value'" "RETRY_ATTEMPT is"
  done

  for number in 1 2; do
    make_fixture "$image_contents"
    mirror_list_before=$(cat "$mirror_list")
    rm "$etc/sources.list.d/ubuntu.sources"
    run_at_attempt "$number" update
    expect_refusal "attempt $number with no sources" "name no mirror list"

    make_fixture "$image_contents"
    mirror_list_before=$(cat "$mirror_list")
    sed -i.bak 's/^URIs:/# URIs:/' "$etc/sources.list.d/ubuntu.sources"
    run_at_attempt "$number" update
    expect_refusal "attempt $number with the source commented out" "name no mirror list"

    make_fixture "$image_contents"
    mirror_list_before=$(cat "$mirror_list")
    printf 'deb mirror+file:%s/other noble main\n' "$etc" > "$etc/sources.list"
    run_at_attempt "$number" update
    expect_refusal "attempt $number with two lists" "more than one mirror list"

    make_fixture "$image_contents"
    mirror_list_before=""
    rm "$mirror_list"
    run_at_attempt "$number" update
    expect_refusal "attempt $number with no list" "is not a file"

    local bad
    for bad in "http://a.example/" "http://a.example/	arch:amd64" \
               "http://a.example/	priority:1 priority:3" "http://a.example/	priority:high"; do
      make_fixture "$(printf '%s\t%s\n%s\n' http://z.example/ priority:2 "$bad")"
      mirror_list_before=$(cat "$mirror_list")
      run_at_attempt "$number" update
      expect_refusal "attempt $number with '$bad'" "line 2 gives its mirror no priority"
    done

    make_fixture "$(printf '%s\t%s\n%s\t%s\n' http://a.example/ priority:1 http://b.example/ priority:1)"
    mirror_list_before=$(cat "$mirror_list")
    run_at_attempt "$number" update
    expect_refusal "attempt $number with one priority for every mirror" "every mirror has the same priority"

    make_fixture "# nothing but a comment"
    mirror_list_before=$(cat "$mirror_list")
    run_at_attempt "$number" update
    expect_refusal "attempt $number with no mirror in the list" "it names no mirror"
  done
  return "$fails"
}

# Each mutant is a description, the text to replace, and its replacement.
mutants=(
  'the attempt not checked'        '[[ ${RETRY_ATTEMPT-} =~ ^[1-9][0-9]*$ ]]'  'true'
  'a leading zero accepted'        '^[1-9][0-9]*$'  '^[0-9]+$'
  'never demoted'                  'if ((n in priority) && priority[n] == least) {'  'if (0) {'
  'rewritten on the first attempt' 'if [ "$RETRY_ATTEMPT" -gt 1 ]; then'  'if true; then'
  'the list not rewritten'         '"$demoted_contents" > "$mirror_list"'  '"$demoted_contents" > /dev/null'
  'the image not kept'             '[ -e "$mirror_list.image" ] || cp -p "$mirror_list" "$mirror_list.image"'  ':'
  'rewritten from the list'        '[ -e "$image_mirror_list" ] || image_mirror_list=$mirror_list'  'image_mirror_list=$mirror_list'
  'one tied mirror demoted'        'priority[n] == least) {'  'priority[n] == least && !done++) {'
  'demoted to the largest'         '"priority:" (most + 1)'  '"priority:" most'
  'the largest demoted'            'priority[NR] < least) least'  'priority[NR] > least) least'
  'the tags before dropped'        'substr(tag_text, 2, RSTART - 1)'  '""'
  'the tags after dropped'         'substr(tag_text, RSTART + RLENGTH)'  '""'
  'a priority tag first missed'    'tag_text = " " substr'  'tag_text = substr'
  'ties accepted'                  'least == most) problem'  'least == most && 0) problem'
  'no priority accepted'           'priority_tag_count != 1 ||'  'priority_tag_count > 1 ||'
  'two priorities accepted'        'priority_tag_count != 1 ||'  'priority_tag_count == 0 ||'
  'a word accepted as a priority'  'value !~ /^[0-9]+$/'  '0'
  'an empty list accepted'         'mirror_count == 0) problem'  '0) problem'
  'comments read as mirrors'       '/^#/ || $0 == ""'  '$0 == ""'
  'commented sources read'         "sed 's/#.*//'"  'cat'
  'only whole comment lines'       "sed 's/#.*//'"  "grep -v '^#'"
  'one list named twice refused'   ' | sort -u)'  ')'
  'no list accepted'               '  0) refuse'  '  0) ;; 00) refuse'
  'two lists accepted'             '  *) refuse'  '  *) ;; 00) refuse'
  'a missing list accepted'        '[ -f "$image_mirror_list" ] || refuse'  'true || refuse'
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
  grep -q '^FAIL' "$tmp/controls.log" && { cat "$tmp/controls.log"; survivors=1; }
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
