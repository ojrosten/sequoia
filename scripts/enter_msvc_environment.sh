#!/bin/bash
# Usage: enter_msvc_environment.sh
#
# Gives the later steps of a GitHub Actions job on Windows the variables
# named below that the latest Visual Studio's vcvars64.bat sets, and the
# entries it adds to PATH, so that CMake's Ninja generator finds cl.exe. The
# script also makes sure that those steps find ninja. It writes the variables
# to GITHUB_ENV, and the entries, other than Visual Studio's CMake and Ninja,
# to GITHUB_PATH, in Windows form, which a later bash step's start-up
# translates.
#
# With the Visual Studio generator, CMake finds the toolset itself. With
# Ninja, CMake looks for cl.exe on PATH, and nothing puts it there. vcvars
# does, but only within the cmd session that runs it.
#
# The script fails with status 1 and an ::error::, and forwards nothing, when:
#   - vswhere reports no installation with the VC toolset;
#   - the installation has no vcvars64.bat;
#   - vcvars64.bat fails;
#   - vcvars64.bat sets none of the variables forwarded;
#   - vcvars64.bat adds no entry to PATH other than Visual Studio's CMake and
#     Ninja.
# Where neither the forwarded entries nor the script's own PATH hold ninja,
# the script installs ninja with Chocolatey.
#
# ENTER_MSVC_ENVIRONMENT_VSWHERE replaces the path of vswhere.exe, for the
# selftest.

set -euo pipefail

readonly vswhere=${ENTER_MSVC_ENVIRONMENT_VSWHERE:-"/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"}
installation=$("$vswhere" -latest -products '*' \
                 -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 \
                 -property installationPath)
[ -n "$installation" ] || { echo "::error::no VC toolset reported by vswhere"; exit 1; }
readonly installation
vcvars="$(cygpath -u "$installation")/VC/Auxiliary/Build/vcvars64.bat"
[ -f "$vcvars" ] || { echo "::error::$vcvars does not exist"; exit 1; }
readonly vcvars

# vcvars runs from a batch file in RUNNER_TEMP, not through
# `cmd //c "call ... && set"`: Git bash's MSYS layer re-quotes an argument
# holding spaces and quotes, and cmd then reads the escaping literally. The
# batch file's own path has no spaces, so MSYS has nothing to re-quote.
readonly bat="$RUNNER_TEMP/vcvars_dump.bat"
printf '@echo off\r\necho PATH_BEFORE=%%PATH%%\r\ncall "%s" >nul || exit /b 1\r\nset\r\n' "$(cygpath -w "$vcvars")" > "$bat"
dump=$(cmd //c "$(cygpath -w "$bat")" | tr -d '\r') || { echo "::error::running $vcvars failed"; exit 1; }
readonly dump

# The script forwards named variables, rather than the whole `set` dump, so
# that it cannot clobber the runner's own environment.
forwarded_variables=$(printf '%s\n' "$dump" \
              | grep -iE '^(INCLUDE|LIB|LIBPATH|VCToolsInstallDir|VCINSTALLDIR|VSINSTALLDIR|WindowsSdkDir|WindowsSDKVersion|UCRTVersion|UniversalCRTSdkDir)=') \
  || { echo "::error::$vcvars set none of the variables forwarded"; exit 1; }
readonly forwarded_variables

# The script forwards only the PATH entries vcvars added. Forwarding the whole
# PATH would put the runner's own entries ahead of themselves in a new order:
# WindowsApps then precedes Git\bin, and `shell: bash` resolves to the WSL
# stub in every later step.
#
# Nor does the script forward Visual Studio's bundled CMake and Ninja, which
# would otherwise come ahead of the image's. The job then builds with the
# image's cmake, as a job with the Visual Studio generator does, so that a red
# run asks about the generator and nothing else.
#
# path_entries_of <name> prints the entries of the dump's variable <name>, one
# to a line. cmd's `set` writes PATH as Path.
path_entries_of() {
  printf '%s\n' "$dump" | grep -iE "^$1=" | sed 's/^[^=]*=//' | tr ';' '\n'
}
path_before=$(path_entries_of PATH_BEFORE)
path_after=$(path_entries_of PATH)
readonly path_before path_after
forwarded_entries=$(printf '%s\n' "$path_after" | grep -vxF -f <(printf '%s\n' "$path_before") | grep -v '^$' \
                      | grep -vF 'CommonExtensions\Microsoft\CMake\') \
  || { echo "::error::$vcvars added no entry to PATH other than Visual Studio's CMake and Ninja"; exit 1; }
readonly forwarded_entries
printf '%s\n' "$forwarded_entries"
printf '%s\n' "$forwarded_variables" >> "$GITHUB_ENV"
printf '%s\n' "$forwarded_entries" >> "$GITHUB_PATH"

# GITHUB_PATH reaches only later steps, so the script looks for ninja where a
# later step will: in the forwarded entries, then in the script's own PATH.
PATH="$(cygpath -pu "$(printf '%s\n' "$forwarded_entries" | paste -sd ';' -)"):$PATH" command -v ninja \
  || { echo "no ninja on the forwarded PATH or the image's; installing one"; choco install ninja -y; }
