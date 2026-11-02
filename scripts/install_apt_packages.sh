#!/bin/bash
# Usage: install_apt_packages.sh [--no-install-recommends] <package>...
#
# Installs each <package> from the hosted image's apt repositories, other than
# Chrome's and Microsoft's, with the packages it depends on. The script also
# installs the packages that each recommends, unless the first argument is
# --no-install-recommends. Each <package> is a package's exact name. A caller
# runs the script as root, with sudo.
#
# The script refuses with status 2, and runs nothing, when:
#   - the arguments name no <package>;
#   - an argument other than a leading --no-install-recommends begins with -;
#   - the script does not run as root.
# The script fails with status 1 and an ::error::, and installs nothing, in
# each of these cases:
#   - a <package> is not a package's name in the refreshed index;
#   - every attempt to fetch the index fails;
#   - every attempt to fetch the packages fails.
# An install that fails exits with apt-get's status.
#
# The fetches take at most 1590 s in all: three attempts at the index, of at
# most 190 s each, three at the packages, of at most 310 s each, and the
# pauses of scripts/retry.sh between them. So a caller's timeout must leave
# room for 1590 s and the install.
#
# APT_ETC replaces /etc/apt, APT_GET replaces apt-get, and APT_CACHE replaces
# apt-cache, for the selftest.

set -euo pipefail

usage() {
  echo "Usage: $0 [--no-install-recommends] <package>..." >&2
  exit 2
}

recommends=()
if [ "${1-}" = --no-install-recommends ]; then
  recommends=(--no-install-recommends)
  shift
fi
[ $# -ge 1 ] || usage
for package in "$@"; do
  [[ $package != -* ]] || usage
done
[ "$(id -u)" -eq 0 ] || { echo "::error::$0 must run as root, with sudo"; exit 2; }

here=$(dirname "$0")
etc=${APT_ETC:-/etc/apt}

# The hosted image ships Chrome's and Microsoft's apt repositories. A bad
# index from either fails `apt-get update`.
rm -f "$etc"/sources.list.d/google-chrome.list "$etc"/sources.list.d/microsoft-prod.list

# The archive is a live dependency of every caller, and an index refresh has
# hung until the job's timeout. So scripts/retry.sh runs each apt command that
# fetches: an attempt that fails, or runs past its limit, is retried a bounded
# number of times. The limits lie above the 22-126 s that a whole install has
# taken, so that only a fetch which stalls or crawls is stopped. Unretried,
# symbol-table.yml's install of valgrind took 562-630 s in three of eight runs
# from 2026-10-26 to 10-28: one download of 26.5 MB ran at 46.5 kB/s (run
# 40501869274).
#
# The command retried is
# scripts/apt_get_demoting_preferred_mirrors_on_retry.sh, so that a later
# attempt fetches from another mirror first. On 2026-10-22,
# azure.archive.ubuntu.com, the mirror the image prefers, stalled most of an
# attempt, and the retry fetched from it again, too slowly to finish (run
# 39767016365, its docs job). In another run, apt turned to archive.ubuntu.com
# within one attempt, of its own accord, and fetched 95 MB from it in 6 s (run
# 39772556372, attempt 2, its docs job).
attempts=3
index_limit=180
download_limit=300
retry=$here/retry.sh
apt_get_demoting=$here/apt_get_demoting_preferred_mirrors_on_retry.sh

# `apt-get update` reports an index it could not fetch as a warning, and exits
# 0. The packages of that index are then missing from `apt-cache pkgnames`,
# so the check below blames the image, as it did for a PPA in runs
# 39614191959 and 39617611706. `--error-on=any` turns a failed index into an
# error, which scripts/retry.sh retries. The refresh is quiet (-q) rather
# than silent (-qq), so that the log names the repository a slow index comes
# from.
"$retry" "$attempts" "$index_limit" -- "$apt_get_demoting" update -q --error-on=any

# `apt-cache pkgnames` lists names exactly. `apt-get install` does not match
# names exactly: given a name with regex metacharacters that matches no
# package, it treats the name as a regex. So `g++-15` on noble resolves to
# clang-15, flang-15 and llvm-15, and apt-get installs all three and exits 0.
# -F because a name may hold a `.`, which a regex matches against any
# character.
names=$("${APT_CACHE:-apt-cache}" pkgnames)
for package in "$@"; do
  grep -qFx -- "$package" <<< "$names" \
    || { echo "::error::$package is not available on this image"; exit 1; }
done

# Only the download is retried. The install then runs once, from the
# downloaded packages, with no download. apt-get runs dpkg in a session of its
# own, so stopping an attempt leaves dpkg running and holding its lock. Every
# later attempt then fails at once (run 39748507037, its docs job).
"$retry" "$attempts" "$download_limit" -- "$apt_get_demoting" \
  install -y ${recommends[@]+"${recommends[@]}"} --download-only "$@"
"${APT_GET:-apt-get}" install -y ${recommends[@]+"${recommends[@]}"} --no-download "$@"
