#!/bin/bash
# Usage: apt_get_with_mirror_fallback.sh <apt-get arguments>...
#
# Runs apt-get with the arguments given, as the command scripts/retry.sh
# retries. From the second attempt on, apt fetches from the image's other
# mirrors before the mirror the image prefers.
#
# The hosted images' apt sources name a list of mirrors with mirror+file:.
# Each mirror in the list has a priority, and apt fetches every file from the
# mirror whose priority is the smallest number. When a file times out, apt
# retries it from the same mirror, three times by default, and only then turns
# to the next mirror. A stalled mirror times a file out after 30 s, so apt can
# stay on it for minutes, and every apt-get starts again from that mirror.
#
# So when RETRY_ATTEMPT is 2 or more, the script first rewrites the list,
# giving the mirrors of the smallest priority a priority larger than every
# other mirror's. The list stays rewritten for the rest of the job, so every
# later apt command fetches from another mirror first too. The image's list is
# kept beside the list, as <list>.image, and every rewrite starts from it.
#
# The script refuses with status 2, before apt-get runs, when it cannot tell
# which mirror apt prefers, or there is no other mirror:
#   - RETRY_ATTEMPT is not a positive whole number;
#   - the sources name no mirror list, or more than one;
#   - a mirror in the list has no priority, or more than one, or one which is
#     not a whole number;
#   - every mirror in the list has the same priority.
# It checks the list on the first attempt too, so that an image whose list no
# longer suits it fails the first run, not the next stall.
#
# apt reports a file it will fetch again with a line beginning Ign:, and says
# why only when Acquire::Progress::Ignore::ShowErrorText is set; apt-get runs
# with it set.
#
# APT_ETC replaces /etc/apt, and APT_GET replaces apt-get, for the selftest.

set -u

refuse() {
  echo "::error::$0: $1"
  exit 2
}

[[ ${RETRY_ATTEMPT-} =~ ^[1-9][0-9]*$ ]] \
  || refuse "RETRY_ATTEMPT is '${RETRY_ATTEMPT-}', not a positive whole number"

etc=${APT_ETC:-/etc/apt}
sources=()
for file in "$etc"/sources.list "$etc"/sources.list.d/*.list "$etc"/sources.list.d/*.sources; do
  [ -f "$file" ] && sources+=("$file")
done
lists=$(cat /dev/null ${sources[@]+"${sources[@]}"} | grep -v '^[[:space:]]*#' \
          | grep -oE 'mirror\+file:[^[:space:]]+' | sed 's/^mirror+file://' | sort -u)
case $(grep -c . <<< "$lists") in
  0) refuse "the sources in $etc name no mirror list" ;;
  1) ;;
  *) refuse "the sources in $etc name more than one mirror list: $(tr '\n' ' ' <<< "$lists")" ;;
esac

list=$lists
image=$list.image
[ -e "$image" ] || image=$list
[ -f "$image" ] || refuse "the mirror list $image is not a file"

# Prints the list with the preferred mirrors' priorities replaced, or names
# the first line which stops it telling which mirror apt prefers, and fails. A
# mirror's tags follow its first tab, separated by white space, as apt reads
# them.
rewritten=$(awk '
  /^#/ || $0 == "" { line[NR] = $0; next }
  {
    tab = index($0, "\t")
    count = split(tab ? substr($0, tab + 1) : "", tags, /[[:space:]]+/)
    priorities = 0
    for (i = 1; i <= count; ++i)
      if (tags[i] ~ /^priority:/) { ++priorities; value = substr(tags[i], 10) }
    if (priorities != 1 || value !~ /^[0-9]+$/) {
      problem = "line " NR " gives its mirror no priority, or more than one, or one which is not a whole number"
      exit 1
    }
    line[NR] = $0
    priority[NR] = value + 0
    if (mirrors == 0 || priority[NR] < least) least = priority[NR]
    if (mirrors == 0 || priority[NR] > most) most = priority[NR]
    ++mirrors
  }
  END {
    if (problem == "" && mirrors == 0) problem = "it names no mirror"
    if (problem == "" && least == most) problem = "every mirror has the same priority"
    if (problem != "") { print problem; exit 1 }
    for (n = 1; n <= NR; ++n) {
      if ((n in priority) && priority[n] == least) {
        tab = index(line[n], "\t")
        count = split(substr(line[n], tab + 1), tags, /[[:space:]]+/)
        rebuilt = substr(line[n], 1, tab)
        separator = ""
        for (i = 1; i <= count; ++i) {
          if (tags[i] == "") continue
          if (tags[i] ~ /^priority:/) tags[i] = "priority:" (most + 1)
          rebuilt = rebuilt separator tags[i]
          separator = " "
        }
        line[n] = rebuilt
      }
      print line[n]
    }
  }' "$image") || refuse "the mirror list $image cannot be used: $rewritten"

if [ "$RETRY_ATTEMPT" -gt 1 ]; then
  [ -e "$list.image" ] || cp -p "$list" "$list.image" || refuse "the image's mirror list cannot be kept as $list.image"
  printf '%s\n' "$rewritten" > "$list" || refuse "the mirror list $list cannot be rewritten"
  echo "Attempt $RETRY_ATTEMPT fetches from the image's other mirrors first:"
  sed 's/^/  /' "$list"
fi

exec "${APT_GET:-apt-get}" -o Acquire::Progress::Ignore::ShowErrorText=true "$@"
