#!/bin/bash
# Controls for enter_msvc_environment.sh, and a mutation check of the controls.
#
#   enter_msvc_environment_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls against the script. With
# it, the selftest runs them against the script and against each mutant of the
# script: the script must fail no control, and each mutant at least one.
#
# The script runs on Windows alone, so each control runs it with stand-ins for
# vswhere, cygpath, cmd and choco. A directory of the fixture stands in for
# drive C:, and the stand-in for cygpath converts between the two forms of a
# path within it. The stand-in for cmd keeps the batch file it is given, and
# prints the fixture's dump of the environment, with CRLF line endings. The
# script's PATH holds only the stand-ins and the tools it needs, so that no
# ninja of this machine's is found. The selftest controls the script's logic,
# and none of the Windows tools it drives. Each control is a claim the script
# exists to keep:
#   - GITHUB_ENV receives exactly the named variables, matched by whole name
#     in any case, and none of the dump's other variables;
#   - GITHUB_PATH receives exactly the PATH entries vcvars added, in vcvars'
#     order, without Visual Studio's CMake and Ninja, and without an empty
#     entry;
#   - the batch file prints the PATH before vcvars, calls the vcvars64.bat of
#     the installation vswhere reports, then prints the environment;
#   - ninja in a forwarded entry, or on the script's own PATH, is found, and
#     Chocolatey installs nothing; ninja only in Visual Studio's CMake
#     directory, or nowhere, has Chocolatey install it;
#   - the script fails with status 1, an ::error:: naming the cause, and
#     nothing forwarded, when vswhere reports no installation, when the
#     installation has no vcvars64.bat, when vcvars fails, when it sets no
#     variable forwarded, and when it adds no PATH entry but CMake's.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
original="$here/../enter_msvc_environment.sh"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/enter_msvc_environment_selftest.XXXXXX") || exit 2
readonly here original tmp
trap 'rm -rf "$tmp"' EXIT

readonly drive=$tmp/c
readonly bin=$tmp/bin
mkdir -p "$bin"
for tool in cp grep paste sed tr; do
  ln -s "$(command -v "$tool")" "$bin/$tool"
done

# cygpath -u|-w|-pu <path>, between C:\a\b and $drive/a/b.
cat > "$bin/cygpath" <<STANDIN
#!/bin/bash
to_unix() { local p=\${1#[Cc]:\\\\}; echo "$drive/\${p//\\\\//}"; }
case \$1 in
  -u) to_unix "\$2" ;;
  -w) p=\${2#$drive/}; echo "C:\\\\\${p//\\//\\\\}" ;;
  -pu) IFS=';' read -ra entries <<< "\$2"
       for e in "\${entries[@]}"; do to_unix "\$e"; done | paste -sd ':' - ;;
  *) exit 2 ;;
esac
STANDIN
cat > "$bin/cmd" <<STANDIN
#!/bin/bash
[ "\$1" = //c ] || exit 2
"$bin/cygpath" -u "\$2" > "$tmp/bat_path"
cp "\$("$bin/cygpath" -u "\$2")" "$tmp/bat_seen"
while IFS= read -r line; do printf '%s\r\n' "\$line"; done < "$tmp/dump"
exit "\${STANDIN_CMD_STATUS:-0}"
STANDIN
cat > "$bin/vswhere" <<STANDIN
#!/bin/bash
printf '%s\n' "\$*" > "$tmp/vswhere_arguments"
[ -n "\${STANDIN_NO_INSTALLATION-}" ] || echo 'C:\\VS'
STANDIN
cat > "$bin/choco" <<STANDIN
#!/bin/bash
echo "\$*" >> "$tmp/choco_calls"
STANDIN
chmod +x "$bin"/*

readonly msvc='C:\VS\VC\Tools\MSVC\14.44\bin\HostX64\x64'
readonly kits='C:\Kits\10\bin\x64'
readonly vs_cmake='C:\VS\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin'
readonly vs_ninja='C:\VS\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja'
readonly runner_path='C:\Windows;C:\Program Files\Git\bin;C:\Users\runner\AppData\Local\Microsoft\WindowsApps'
forwarded_variables=$(printf '%s\n' \
  'INCLUDE=C:\VS\include' 'LIB=C:\VS\lib' 'LibPath=C:\VS\libpath' 'UCRTVersion=10.0.26100.0' \
  'UniversalCRTSdkDir=C:\Kits\10\' 'VCINSTALLDIR=C:\VS\VC\' 'VCToolsInstallDir=C:\VS\VC\Tools\MSVC\14.44\' \
  'VSINSTALLDIR=C:\VS\' 'WindowsSdkDir=C:\Kits\10\' 'WindowsSDKVersion=10.0.26100.0\')
readonly forwarded_variables

# Writes the dump that the stand-in for cmd prints: write_dump <PATH after>
# [<variables>]. The dump names PATH as Path, as cmd's `set` does.
write_dump() {
  printf '%s\n' "PATH_BEFORE=$runner_path" "${2-$forwarded_variables}" 'LIBX=C:\not\forwarded' \
    'XINCLUDE=C:\not\forwarded' "Path=$1" 'PROMPT=$P$G' 'VSCMD_ARG_TGT_ARCH=x64' > "$tmp/dump"
}

# Makes a fresh fixture: the installation, with its vcvars64.bat, and a dump
# in which vcvars puts its four entries, and an empty one, before the
# runner's.
make_fixture() {
  rm -rf "$drive" "$tmp/out" "$tmp/bat_seen" "$tmp/bat_path" "$tmp/vswhere_arguments" "$tmp/choco_calls" \
         "$tmp/env" "$tmp/path"
  mkdir -p "$drive/VS/VC/Auxiliary/Build" "$drive/runner_temp"
  : > "$drive/VS/VC/Auxiliary/Build/vcvars64.bat"
  : > "$tmp/env"
  : > "$tmp/path"
  write_dump "$msvc;$vs_cmake;;$vs_ninja;$kits;$runner_path"
}

# Places an executable ninja in a directory, given in Windows form.
place_ninja() {
  local directory
  directory=$("$bin/cygpath" -u "$1")
  mkdir -p "$directory"
  printf '#!/bin/bash\n' > "$directory/ninja"
  chmod +x "$directory/ninja"
}

run() {
  PATH="$bin" ENTER_MSVC_ENVIRONMENT_VSWHERE="$bin/vswhere" RUNNER_TEMP="$drive/runner_temp" GITHUB_ENV="$tmp/env" \
    GITHUB_PATH="$tmp/path" "$BASH" "$script" > "$tmp/out" 2>&1
  status=$?
}

fails=0
fail() { echo "FAIL: $1"; fails=$((fails + 1)); }

expect() { # expect <claim> <actual> <expected>
  [ "$2" = "$3" ] || fail "$1 (expected '$3', got '$2')"
}

expect_failure() { # expect_failure <claim> <message>
  expect "$1: fails with status 1" "$status" 1
  expect "$1: an ::error:: names the cause" "$(grep -c -- "^::error::.*$2" "$tmp/out")" 1
  expect "$1: no variable is forwarded" "$(cat "$tmp/env")" ""
  expect "$1: no PATH entry is forwarded" "$(cat "$tmp/path")" ""
}

run_controls() {
  fails=0

  make_fixture
  place_ninja "$kits"
  run
  expect "the environment is entered, with status 0" "$status" 0
  expect "GITHUB_ENV receives exactly the named variables" "$(LC_ALL=C sort "$tmp/env")" \
    "$(LC_ALL=C sort <<< "$forwarded_variables")"
  expect "GITHUB_PATH receives the added entries but CMake's, in order" "$(cat "$tmp/path")" \
    "$(printf '%s\n' "$msvc" "$kits")"
  expect "the batch file prints PATH, calls the installation's vcvars, then prints the environment" \
    "$(cat "$tmp/bat_seen" 2> /dev/null)" \
    "$(printf '@echo off\r\necho PATH_BEFORE=%%PATH%%\r\ncall "C:\\VS\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul || exit /b 1\r\nset\r\n')"
  expect "the batch file is in RUNNER_TEMP" "$(cat "$tmp/bat_path" 2> /dev/null)" "$drive/runner_temp/vcvars_dump.bat"
  expect "vswhere is asked for the latest installation with the VC toolset" "$(cat "$tmp/vswhere_arguments")" \
    "-latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath"
  expect "ninja in a forwarded entry: Chocolatey installs nothing" "$(cat "$tmp/choco_calls" 2> /dev/null)" ""

  make_fixture
  place_ninja 'C:\image_bin'
  ln -s "$drive/image_bin/ninja" "$bin/ninja"
  run
  rm "$bin/ninja"
  expect "ninja on the script's own PATH: status 0" "$status" 0
  expect "ninja on the script's own PATH: Chocolatey installs nothing" "$(cat "$tmp/choco_calls" 2> /dev/null)" ""

  make_fixture
  place_ninja "$vs_ninja"
  run
  expect "ninja only in Visual Studio's CMake: status 0" "$status" 0
  expect "ninja only in Visual Studio's CMake: Chocolatey installs it" "$(cat "$tmp/choco_calls" 2> /dev/null)" \
    "install ninja -y"

  make_fixture
  run
  expect "no ninja: Chocolatey installs it" "$(cat "$tmp/choco_calls" 2> /dev/null)" "install ninja -y"

  make_fixture
  STANDIN_NO_INSTALLATION=1 run
  expect_failure "no installation" "no VC toolset reported by vswhere"

  make_fixture
  rm "$drive/VS/VC/Auxiliary/Build/vcvars64.bat"
  run
  expect_failure "no vcvars64.bat" "vcvars64.bat does not exist"

  make_fixture
  STANDIN_CMD_STATUS=1 run
  expect_failure "vcvars fails" "vcvars64.bat failed"

  make_fixture
  write_dump "$msvc;$kits;$runner_path" 'OTHER=1'
  run
  expect_failure "no variable forwarded" "set none of the variables forwarded"

  make_fixture
  write_dump "$vs_cmake;$vs_ninja;$runner_path"
  run
  expect_failure "no entry added but CMake's" "added no entry to PATH"

  make_fixture
  write_dump "$runner_path"
  run
  expect_failure "no entry added" "added no entry to PATH"

  return "$fails"
}

# Each mutant is a description, the text to replace, and its replacement.
readonly -a mutants=(
  'every variable forwarded'      "| grep -iE '^(INCLUDE"  "| grep -iE '|^(INCLUDE"
  'a variable matched by prefix'  'UniversalCRTSdkDir)='  'UniversalCRTSdkDir)'
  'a variable matched anywhere'   "grep -iE '^(INCLUDE"  "grep -iE '(INCLUDE"
  'a variable matched by case'    "grep -iE '^(INCLUDE"  "grep -E '^(INCLUDE"
  'the whole PATH forwarded'      'grep -vxF -f <(printf '"'"'%s\n'"'"' "$path_before") | '  ''
  'Path not read'                 'grep -iE "^$1="'  'grep -E "^$1="'
  'an empty entry forwarded'      " | grep -v '^\$' \\"  ' \\'
  'CMake forwarded'               " | grep -vF 'CommonExtensions\\Microsoft\\CMake\\')"  ")"
  'nothing forwarded to the env'  'printf '"'"'%s\n'"'"' "$forwarded_variables" >> "$GITHUB_ENV"'  ':'
  'nothing forwarded to PATH'     'printf '"'"'%s\n'"'"' "$forwarded_entries" >> "$GITHUB_PATH"'  ':'
  'no installation accepted'      '[ -n "$installation" ] ||'  'true ||'
  'no vcvars accepted'            '[ -f "$vcvars" ] ||'  'true ||'
  'vcvars failure ignored'        ' || { echo "::error::running $vcvars failed"; exit 1; }'  ' || true'
  'vcvars not called'             'call "%s" >nul || exit /b 1\r\n'  ''
  'PATH before not printed'       'echo PATH_BEFORE=%%PATH%%\r\n'  ''
  'forwarded entries not searched' 'PATH="$(cygpath -pu "$(printf '"'"'%s\n'"'"' "$forwarded_entries" | paste -sd '"'"';'"'"' -)"):$PATH" command'  'command'
  'ninja never installed'         'choco install ninja -y'  'true'
  'ninja always installed'        'command -v ninja \'  'false \'
  'no variables refused late'     '|| { echo "::error::$vcvars set none'  '|| true || { echo "::error::$vcvars set none'
  'no entries accepted'           '|| { echo "::error::$vcvars added no entry'  '|| true || { echo "::error::$vcvars added no entry'
  'forwarded before refusing'     '|| { echo "::error::$vcvars added no entry'  '|| { printf '"'"'%s\n'"'"' "$forwarded_variables" >> "$GITHUB_ENV"; echo "::error::$vcvars added no entry'
  'a refusal not annotated'       'echo "::error::no VC toolset'  'echo "no VC toolset'
  'the batch file elsewhere'      'bat="$RUNNER_TEMP/vcvars_dump.bat"'  'bat="$RUNNER_TEMP/../vcvars_dump.bat"'
  'any installation'              '-requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 \'  '\'
  "the script's PATH not searched" ':$PATH" command -v ninja'  '" command -v ninja'
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
if [ "$fails" -eq 0 ]; then echo "enter_msvc_environment: all controls pass"; else exit 1; fi
