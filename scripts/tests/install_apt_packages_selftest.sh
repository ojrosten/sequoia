#!/bin/bash
# Controls for install_apt_packages.sh, and a mutation check of the controls.
#
#   install_apt_packages_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls against the script. With
# it, the selftest runs them against the script and against each mutant of the
# script: the script must fail no control, and each mutant at least one.
#
# Each control runs the script on a fixture standing in for /etc/apt, with the
# real retry.sh and apt_get_demoting_preferred_mirrors_on_retry.sh, and with
# stand-ins for apt-get, apt-cache and id. The stand-in for apt-get records
# each call, and fails the calls a control chooses. Each control is a claim the
# script exists to keep:
#   - Chrome's and Microsoft's lists go, and the image's other sources stay;
#   - apt-get refreshes the index, downloads the packages, then installs them
#     with no download, in that order, and with the options the script
#     documents; the selftest pins the options;
#   - the index and the download are fetched through retry.sh, up to three
#     attempts each, and through the demoting script, so that a later attempt
#     fetches from another mirror first;
#   - --no-install-recommends reaches both the download and the install;
#   - a name that is not a package's exact name is refused with status 1 and
#     an ::error::, and nothing is downloaded: a prefix of a name, and a name
#     whose `.` matches another character in a name, are refused;
#   - a fetch that fails on every attempt fails the script with status 1, and
#     nothing more runs;
#   - a failed install exits with apt-get's status;
#   - the script refuses with status 2, running nothing and removing nothing,
#     when no package is given, when an argument begins with - and is not a
#     leading --no-install-recommends, and when it does not run as root.
#
# The limits of each attempt are not controlled: a control would wait for them.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
scripts=$(cd "$here/.." && pwd -P)
tmp=$(mktemp -d "${TMPDIR:-/tmp}/install_apt_packages_selftest.XXXXXX")
readonly here scripts tmp
trap 'rm -rf "$tmp"' EXIT

mkdir "$tmp/bin"
cat > "$tmp/bin/apt-get" <<'STANDIN'
#!/bin/bash
# Fails a call whose arguments hold STANDIN_FAIL_ON, at attempts before
# STANDIN_FAIL_BEFORE (every attempt if unset), with STANDIN_STATUS.
echo "$*" >> "$(dirname "$0")/calls"
if [ -n "${STANDIN_FAIL_ON-}" ] && [[ " $* " == *" $STANDIN_FAIL_ON "* ]] \
     && [ "${RETRY_ATTEMPT:-1}" -lt "${STANDIN_FAIL_BEFORE:-1000}" ]; then
  exit "${STANDIN_STATUS:-100}"
fi
exit 0
STANDIN
cat > "$tmp/bin/apt-cache" <<'STANDIN'
#!/bin/bash
[ "$1" = pkgnames ] || exit 1
printf '%s\n' clang-15 flang-15 gdb gdbserver libc6-dbg libtbb-dev ninja-build texlive-latex-base
STANDIN
cat > "$tmp/bin/id" <<'STANDIN'
#!/bin/bash
[ "$1" = -u ] && echo "${STANDIN_UID:-0}"
STANDIN
chmod +x "$tmp/bin"/*

# The hosted images' list, in which a tab separates each mirror from its tags.
image_contents=$(printf '%s\t%s\n' \
  http://azure.archive.ubuntu.com/ubuntu/ priority:1 \
  https://archive.ubuntu.com/ubuntu/      priority:2)
readonly image_contents
demoted_contents=$(printf '%s\t%s\n' \
  http://azure.archive.ubuntu.com/ubuntu/ priority:3 \
  https://archive.ubuntu.com/ubuntu/      priority:2)
readonly demoted_contents
readonly error_text=-o\ Acquire::Progress::Ignore::ShowErrorText=true

fails=0
fail() { echo "FAIL: $1"; fails=$((fails + 1)); }

expect() { # expect <claim> <actual> <expected>
  [ "$2" = "$3" ] || fail "$1 (expected '$3', got '$2')"
}

# Makes a fresh fixture: an etc directory whose sources name its mirror list,
# with Chrome's and Microsoft's lists beside them.
make_fixture() {
  rm -rf "$tmp/etc" "$tmp/bin/calls"
  etc=$tmp/etc
  mkdir -p "$etc/sources.list.d"
  mirror_list=$etc/apt-mirrors.txt
  printf '%s\n' "$image_contents" > "$mirror_list"
  printf 'Types: deb\nURIs: mirror+file:%s\nSuites: noble\nComponents: main\n' "$mirror_list" \
    > "$etc/sources.list.d/ubuntu.sources"
  echo 'deb https://dl.google.com/linux/chrome/deb/ stable main' > "$etc/sources.list.d/google-chrome.list"
  echo 'deb https://packages.microsoft.com/ubuntu/24.04/prod noble main' \
    > "$etc/sources.list.d/microsoft-prod.list"
}

# Runs the script under test on a fresh fixture, and records its output, its
# status and the calls apt-get received: run <arguments>...
run() {
  make_fixture
  : > "$tmp/bin/calls"
  PATH="$tmp/bin:$PATH" APT_ETC="$etc" APT_GET="$tmp/bin/apt-get" APT_CACHE="$tmp/bin/apt-cache" \
    RETRY_PAUSE_SECONDS=0 bash "$script" "$@" > "$tmp/out" 2> "$tmp/err"
  status=$?
  calls=$(cat "$tmp/bin/calls")
}

sources_left() { (cd "$etc/sources.list.d" && ls | tr '\n' ' '); }

expect_refusal() { # expect_refusal <claim>
  expect "$1: refused with status 2" "$status" 2
  expect "$1: apt-get does not run" "$calls" ""
  expect "$1: no source is removed" "$(sources_left)" "google-chrome.list microsoft-prod.list ubuntu.sources "
}

run_controls() {
  fails=0

  run gdb libtbb-dev
  expect "an install succeeds with status 0" "$status" 0
  expect "an install refreshes, downloads, then installs, with the documented options" "$calls" \
    "$(printf '%s\n' "$error_text update -q --error-on=any" \
                     "$error_text install -y --download-only gdb libtbb-dev" \
                     "install -y --no-download gdb libtbb-dev")"
  expect "Chrome's and Microsoft's lists go, and the other sources stay" "$(sources_left)" "ubuntu.sources "
  expect "a first attempt leaves the mirror list as it was" "$(cat "$mirror_list")" "$image_contents"

  run --no-install-recommends gdb
  expect "--no-install-recommends reaches the download and the install" "$calls" \
    "$(printf '%s\n' "$error_text update -q --error-on=any" \
                     "$error_text install -y --no-install-recommends --download-only gdb" \
                     "install -y --no-install-recommends --no-download gdb")"

  STANDIN_FAIL_ON=update STANDIN_FAIL_BEFORE=2 run gdb
  expect "a refresh that fails once is retried, and the install succeeds" "$status" 0
  expect "the retried refresh comes second" "$(sed -n 2p <<< "$calls")" "$error_text update -q --error-on=any"
  expect "the retried refresh fetches from another mirror first" "$(cat "$mirror_list")" "$demoted_contents"

  STANDIN_FAIL_ON=--download-only STANDIN_FAIL_BEFORE=2 run gdb
  expect "a download that fails once is retried, and the install succeeds" "$status" 0
  expect "the download is attempted twice, then the install runs" "$(tail -n 3 <<< "$calls")" \
    "$(printf '%s\n' "$error_text install -y --download-only gdb" "$error_text install -y --download-only gdb" \
                     "install -y --no-download gdb")"

  STANDIN_FAIL_ON=update run gdb
  expect "a refresh failing every attempt fails with status 1" "$status" 1
  expect "a refresh failing every attempt is tried three times, and nothing follows" "$calls" \
    "$(printf '%s\n' "$error_text update -q --error-on=any" "$error_text update -q --error-on=any" \
                     "$error_text update -q --error-on=any")"

  STANDIN_FAIL_ON=--download-only run gdb
  expect "a download failing every attempt fails with status 1" "$status" 1
  expect "a download failing every attempt is tried three times, and nothing is installed" \
    "$(grep -c -- --download-only <<< "$calls") $(grep -c -- --no-download <<< "$calls")" "3 0"

  STANDIN_FAIL_ON=--no-download STANDIN_STATUS=42 run gdb
  expect "a failed install exits with apt-get's status" "$status" 42
  expect "a failed install is not retried" "$(grep -c -- --no-download <<< "$calls")" 1

  local name
  for name in gd libc6.dbg g++-15; do
    run gdb "$name"
    expect "'$name' is refused with status 1" "$status" 1
    expect "'$name' is named by an ::error::" "$(grep -c "^::error::$name is not available" "$tmp/out")" 1
    expect "'$name': nothing is downloaded" "$calls" "$error_text update -q --error-on=any"
  done

  run
  expect_refusal "no package"
  run --no-install-recommends
  expect_refusal "no package after the option"
  run --quiet gdb
  expect_refusal "another option"
  run gdb --no-install-recommends
  expect_refusal "the option after a package"
  STANDIN_UID=1000 run gdb
  expect_refusal "not root"
  expect "not root: an ::error:: says so" "$(grep -c '^::error::.*must run as root' "$tmp/out")" 1

  return "$fails"
}

# Each mutant is a description, the text to replace, and its replacement.
mutants=(
  'no list removed'               'rm -f "$etc"'  ': "$etc"'
  "Microsoft's list kept"         ' "$etc"/sources.list.d/microsoft-prod.list'  ''
  'the refresh not through retry' '"$retry" "$attempts" "$index_attempt_seconds" -- '  'RETRY_ATTEMPT=1 '
  'the download not through retry' '"$retry" "$attempts" "$download_attempt_seconds" -- '  'RETRY_ATTEMPT=1 '
  'the refresh straight to apt-get' '"$index_attempt_seconds" -- "$apt_get_demoting_preferred_mirrors_on_retry"'  '"$index_attempt_seconds" -- "$APT_GET"'
  'the download straight to apt-get' '"$download_attempt_seconds" -- "$apt_get_demoting_preferred_mirrors_on_retry"'  '"$download_attempt_seconds" -- "$APT_GET"'
  'two attempts'                  'attempts=3'  'attempts=2'
  'a failed index a warning'      ' --error-on=any'  ''
  'silent'                        'update -q '  'update -qq '
  'no name checked'               'grep -qFx -- "$package"'  'true'
  'names as regexes'              'grep -qFx'  'grep -qx'
  'names as substrings'           'grep -qFx'  'grep -qF'
  'an unknown name not an error'  'echo "::error::$package'  'echo "$package'
  'an unknown name tolerated'     'available on this image"; exit 1; }'  'available on this image"; }'
  'no download first'             ' --download-only "$@"'  ' "$@"'
  'the install downloads'         ' --no-download "$@"'  ' "$@"'
  'the install retried'           '"${APT_GET:-apt-get}" install'  '"$retry" "$attempts" "$download_attempt_seconds" -- "$apt_get_demoting_preferred_mirrors_on_retry" install'
  'recommends kept on download'   '${recommends_option[@]+"${recommends_option[@]}"} --download-only'  '--download-only'
  'recommends kept on install'    '${recommends_option[@]+"${recommends_option[@]}"} --no-download'  '--no-download'
  'no package accepted'           '[ $# -ge 1 ] || usage'  'true'
  'options accepted'              '[[ $package != -* ]] || usage'  'true'
  'not root accepted'             '[ "$(id -u)" -eq 0 ] ||'  'true ||'
  'not root silent'               'echo "::error::$0 must run as root'  'echo "$0 must run as root'
)

mutations() {
  local i survivors=0 content stripped occurrences failed mutant_dir=$tmp/mutant
  mkdir -p "$mutant_dir"
  cp -p "$scripts/retry.sh" "$scripts/apt_get_demoting_preferred_mirrors_on_retry.sh" "$mutant_dir/"
  script=$scripts/install_apt_packages.sh
  run_controls > "$tmp/controls.log" 2>&1
  echo "unmutated: $(grep -c '^FAIL' "$tmp/controls.log") controls fail"
  grep -q '^FAIL' "$tmp/controls.log" && { cat "$tmp/controls.log"; survivors=1; }
  for ((i = 0; i < ${#mutants[@]}; i += 3)); do
    content=$(cat "$scripts/install_apt_packages.sh"; echo x)
    content=${content%x}
    stripped=${content//"${mutants[i+1]}"/}
    occurrences=$(( (${#content} - ${#stripped}) / ${#mutants[i+1]} ))
    if [ "$occurrences" -ne 1 ]; then
      echo "${mutants[i]}: the text to mutate occurs $occurrences times  <-- must be once"
      survivors=1
      continue
    fi
    printf '%s' "${content%%"${mutants[i+1]}"*}${mutants[i+2]}${content#*"${mutants[i+1]}"}" \
      > "$mutant_dir/install_apt_packages.sh"
    script=$mutant_dir/install_apt_packages.sh
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

script=$scripts/install_apt_packages.sh
run_controls
if [ "$fails" -eq 0 ]; then echo "install_apt_packages: all controls pass"; else exit 1; fi
