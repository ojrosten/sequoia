#!/bin/bash
# Usage: enter_msvc_environment.sh
#
# Gives the later steps of a GitHub Actions job on Windows the environment of
# the latest Visual Studio's vcvars64.bat, so that CMake's Ninja generator
# finds cl.exe on PATH, and finds ninja.
#
# With the Visual Studio generator, CMake finds the toolset itself. With
# Ninja, CMake looks for cl.exe on PATH, and nothing puts it there. vcvars
# does, but only within the cmd session that runs it. So the script captures
# what vcvars sets, and hands it to later steps:
#   - named variables through GITHUB_ENV, rather than the whole `set` dump, so
#     that the script cannot clobber the runner's own environment;
#   - PATH through GITHUB_PATH, in Windows form, which a later bash step's
#     start-up translates.

set -euo pipefail

vswhere="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
root=$("$vswhere" -latest -products '*' \
         -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 \
         -property installationPath)
[ -n "$root" ] || { echo "::error::no VC toolset reported by vswhere"; exit 1; }
vcvars="$(cygpath -u "$root")/VC/Auxiliary/Build/vcvars64.bat"
[ -f "$vcvars" ] || { echo "::error::$vcvars does not exist"; exit 1; }

# vcvars runs from a batch file in RUNNER_TEMP, not through
# `cmd //c "call ... && set"`: Git bash's MSYS layer re-quotes an argument
# holding spaces and quotes, and cmd then reads the escaping literally. The
# batch file's own path has no spaces, so MSYS has nothing to re-quote.
bat="$RUNNER_TEMP/vcvars_dump.bat"
printf '@echo off\r\necho PATH_BEFORE=%%PATH%%\r\ncall "%s" >nul || exit /b 1\r\nset\r\n' "$(cygpath -w "$vcvars")" > "$bat"
dump=$(cmd //c "$(cygpath -w "$bat")" | tr -d '\r')
printf '%s\n' "$dump" | grep -iE '^(INCLUDE|LIB|LIBPATH|VCToolsInstallDir|VCINSTALLDIR|VSINSTALLDIR|WindowsSdkDir|WindowsSDKVersion|UCRTVersion|UniversalCRTSdkDir)=' >> "$GITHUB_ENV"

# The script forwards only the PATH entries vcvars added. Forwarding the whole
# PATH would put the runner's own entries ahead of themselves in a new order:
# WindowsApps then precedes Git\bin, and `shell: bash` resolves to the WSL
# stub in every later step.
#
# Nor does the script forward Visual Studio's bundled CMake and Ninja, which
# would otherwise come ahead of the image's. The job then builds with the
# image's cmake, as a job with the Visual Studio generator does, so that a red
# run asks about the generator and nothing else.
before=$(printf '%s\n' "$dump" | grep '^PATH_BEFORE=' | sed 's/^[^=]*=//' | tr ';' '\n')
after=$(printf '%s\n' "$dump"  | grep -iE '^PATH='     | sed 's/^[^=]*=//' | tr ';' '\n')
added=$(printf '%s\n' "$after" | grep -vxF -f <(printf '%s\n' "$before") | grep -v '^$' | grep -vF 'CommonExtensions\Microsoft\CMake\')
printf '%s\n' "$added"
printf '%s\n' "$added" >> "$GITHUB_PATH"

# GITHUB_PATH reaches only later steps, so the script looks for ninja where a
# later step will: in the forwarded entries, then in the script's own PATH.
PATH="$(cygpath -pu "$(printf '%s\n' "$added" | paste -sd ';')"):$PATH" command -v ninja \
  || { echo "no ninja on the forwarded PATH or the image's; installing one"; choco install ninja -y; }
