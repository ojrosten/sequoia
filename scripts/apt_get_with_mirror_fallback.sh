#!/bin/bash
# Usage: apt_get_with_mirror_fallback.sh <apt-get arguments>...
#
# Runs apt-get with the arguments given. RETRY_ATTEMPT holds the attempt's
# number, as set by scripts/retry.sh. From the second attempt on, apt fetches
# from the other mirrors in the image's mirror list before the mirrors the
# image prefers.
#
# The hosted images' apt sources name a list of mirrors with mirror+file:.
# Each mirror in the list has a priority, and apt fetches every file from the
# mirror whose priority is the smallest number. When a file times out, apt
# retries it from the same mirror, Acquire::Retries times, and only then turns
# to the next mirror. Under a stall, the files time out one at a time, so apt
# can stay on the stalled mirror for minutes. Every apt-get starts again from
# that mirror.
#
# So when RETRY_ATTEMPT is 2 or more, the script first rewrites the list,
# giving the mirrors of the smallest priority a priority larger than every
# other mirror's. The rest of the list is unchanged. The rewritten list stays
# in place after the script ends, so every later apt command fetches from
# another mirror first too. Before its first rewrite, the script copies the
# list to <list>.image, and every rewrite starts from that copy.
#
# The script refuses with status 2, an ::error:: saying why, and apt-get not
# run, when:
#   - RETRY_ATTEMPT is not a positive whole number with no leading zero;
#   - the sources name no mirror list, or more than one;
#   - the mirror list is not a file;
#   - the list names no mirror;
#   - a mirror in the list has no priority, or more than one, or one which is
#     not a whole number;
#   - every mirror in the list has the same priority;
#   - the list cannot be copied or rewritten.
# The script checks the list on the first attempt too, so that a list it
# cannot read fails the first job after the image changes, not the next stall.
#
# apt prints a line beginning Ign: for each file it will fetch again. So that
# the line says why, the script sets Acquire::Progress::Ignore::ShowErrorText.
#
# APT_ETC replaces /etc/apt, and APT_GET replaces apt-get, for the selftest.

set -u

refuse() {
  echo "::error::$0: $1"
  exit 2
}

[[ ${RETRY_ATTEMPT-} =~ ^[1-9][0-9]*$ ]] \
  || refuse "RETRY_ATTEMPT is '${RETRY_ATTEMPT-}', not a positive whole number with no leading zero"

# apt reads a source's line up to its first #. The images' sources name one
# list twice, once in each stanza.
etc=${APT_ETC:-/etc/apt}
sources=()
for file in "$etc"/sources.list "$etc"/sources.list.d/*.list "$etc"/sources.list.d/*.sources; do
  [ -f "$file" ] && sources+=("$file")
done
mirror_lists=$(cat /dev/null ${sources[@]+"${sources[@]}"} | sed 's/#.*//' \
                 | grep -oE 'mirror\+file:[^[:space:]]+' | sed 's/^mirror+file://' | sort -u)
case $(grep -c . <<< "$mirror_lists") in
  0) refuse "the sources in $etc name no mirror list" ;;
  1) ;;
  *) refuse "the sources in $etc name more than one mirror list: $(tr '\n' ' ' <<< "$mirror_lists")" ;;
esac

mirror_list=$mirror_lists
image_mirror_list=$mirror_list.image
[ -e "$image_mirror_list" ] || image_mirror_list=$mirror_list
[ -f "$image_mirror_list" ] || refuse "the mirror list $image_mirror_list is not a file"

# Prints the image's list with each preferred mirror's priority raised above
# every other mirror's. If the list does not show which mirror apt prefers, awk
# prints the reason instead, and fails. A mirror's tags follow its first tab,
# separated by white space, as read by apt.
demoted_contents=$(awk '
  /^#/ || $0 == "" { line[NR] = $0; next }
  {
    tab = index($0, "\t")
    count = split(tab ? substr($0, tab + 1) : "", tags, /[[:space:]]+/)
    priority_tag_count = 0
    for (i = 1; i <= count; ++i)
      if (tags[i] ~ /^priority:/) {
        ++priority_tag_count
        value = tags[i]
        sub(/^priority:/, "", value)
      }
    if (priority_tag_count != 1 || value !~ /^[0-9]+$/) {
      problem = "line " NR " gives its mirror no priority, or more than one, or one which is not a whole number"
      exit 1
    }
    line[NR] = $0
    priority[NR] = value + 0
    if (mirror_count == 0 || priority[NR] < least) least = priority[NR]
    if (mirror_count == 0 || priority[NR] > most) most = priority[NR]
    ++mirror_count
  }
  END {
    if (problem == "" && mirror_count == 0) problem = "it names no mirror"
    if (problem == "" && least == most) problem = "every mirror has the same priority"
    if (problem != "") { print problem; exit 1 }
    for (n = 1; n <= NR; ++n) {
      if ((n in priority) && priority[n] == least) {
        # The space put first matches a priority tag which follows the tab.
        tab = index(line[n], "\t")
        tag_text = " " substr(line[n], tab + 1)
        match(tag_text, /[[:space:]]priority:[0-9]+/)
        line[n] = substr(line[n], 1, tab) substr(tag_text, 2, RSTART - 1) \
                  "priority:" (most + 1) substr(tag_text, RSTART + RLENGTH)
      }
      print line[n]
    }
  }' "$image_mirror_list") || refuse "the mirror list $image_mirror_list cannot be used: $demoted_contents"

if [ "$RETRY_ATTEMPT" -gt 1 ]; then
  [ -e "$mirror_list.image" ] || cp -p "$mirror_list" "$mirror_list.image" \
    || refuse "the image's mirror list cannot be kept as $mirror_list.image"
  printf '%s\n' "$demoted_contents" > "$mirror_list" || refuse "the mirror list $mirror_list cannot be rewritten"
  echo "Attempt $RETRY_ATTEMPT fetches from the image's other mirrors first:"
  sed 's/^/  /' "$mirror_list"
fi

exec "${APT_GET:-apt-get}" -o Acquire::Progress::Ignore::ShowErrorText=true "$@"
