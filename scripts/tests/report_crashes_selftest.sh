#!/bin/bash
# Controls for report_crashes.sh and macos_crash_reports.sh, and a mutation
# check of the controls.
#
#   report_crashes_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the scripts and against each mutant of them. The
# scripts must fail no control, and each mutant at least one.
#
# Two stand-ins for a crashing suite are compiled here, and crash once. One
# aborts in a function whose name the report must show. The other is
# optimised, with line tables, and aborts in a function inlined into its
# caller. The optimised stand-in is compiled and linked in separate steps, as a
# CMake build is, so the system reads its debug information through the object
# file. A dSYM, which CI links, gives the report the same fields.
#
# Each control is a claim the scripts exist to keep. On macOS:
#   - a crash since the since file is reported with its path, its exception
#     and the frames of its faulting thread, then the whole report, and is
#     counted;
#   - an optimised crash is reported with the inlined function and its source
#     line, marked as inlined, and the function it is inlined into is not
#     marked;
#   - a crash before the since file is not reported, so a job reports only its
#     own suite's crashes;
#   - with no crash, the count is zero, rather than the last line being absent;
#   - a crash report in any of the directories is reported and counted;
#   - a report of another kind, a file not named as a report, and a directory
#     are not;
#   - a report whose fields are absent is summarised with "?" in their place,
#     and one that cannot be summarised is printed whole;
#   - the summary shows at most 50 frames;
#   - a directory that cannot be read, or cannot be searched, is named before
#     any crash; one that does not exist is not named;
#   - with no directory to read, the count is the only output;
#   - the directories listed are the user's and the system's
#     DiagnosticReports, unless REPORT_CRASHES_MACOS_DIRECTORIES names others.
# On Linux and Windows, which run here under a uname that names them:
#   - a dump since the since file is reported and counted, and one before it,
#     one in a subdirectory, a file not named as a dump, and a directory are
#     not;
#   - Linux: gdb reads a core with the executable the core names. If that
#     executable is absent or unknown, gdb reads the core alone, and the
#     report says why. With no gdb, the report says so;
#   - Windows: with no cdb, the report says so.
# On every platform:
#   - the script's status is zero whenever it reports;
#   - arguments which do not name a directory and a file are refused.
#
# The controls run on macOS alone, where the system writes a crash report a
# few seconds after each crash. The Linux branch runs here with stubs for file
# and gdb; the stub file prints what GNU file prints for a core. The Windows
# branch runs here without cdb, since windows_debugger.sh looks for cdb only
# under /c. So no control runs the real gdb, file or cdb.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
tmp=$(mktemp -d)
unset REPORT_CRASHES_MACOS_DIRECTORIES
source "$here/../macos_crash_reports.sh"
reports=()
while IFS= read -r directory; do reports+=("$directory"); done < <(macos_crash_report_directories)

# The fixtures include directories which cannot be read, and which rm cannot
# remove until they can.
clean_up() {
  chmod -R u+rwx "$tmp"
  rm -rf "$tmp"
  for directory in "${reports[@]}"; do rm -f "$directory"/CrashingStandIn* "$directory"/InliningStandIn*; done
}
trap clean_up EXIT
fails=0

case "$*" in
  "")          mode=controls  ;;
  --mutations) mode=mutations ;;
  *)           echo "Usage: $0 [--mutations]" >&2; exit 2 ;;
esac

fail() { echo "FAIL: $1"; fails=$((fails+1)); }

check() { # check <name> <yes|no> <pattern> <file>
  if grep -qE "$3" "$4" 2> /dev/null; then got=yes; else got=no; fi
  [ "$got" = "$2" ] || fail "$1 (expected $2, got $got, pattern: $3)"
}

check_status() { # check_status <name> <expected> <got>
  [ "$3" -eq "$2" ] || fail "$1 (expected status $2, got $3)"
}

if [ "$(uname -s)" != Darwin ]; then
  echo "report_crashes: the local controls are macOS's; nothing run here"
  exit 0
fi

name=CrashingStandIn
cat > "$tmp/standin.c" <<'STANDIN'
#include <stdlib.h>
void crashes_in_the_stand_in(void) { abort(); }
int main(void) { crashes_in_the_stand_in(); }
STANDIN
cc -g -O0 -o "$tmp/$name" "$tmp/standin.c" || { echo "FAIL: cannot compile the stand-in"; exit 1; }

inlining=InliningStandIn
cat > "$tmp/inlining.c" <<'STANDIN'
#include <stdlib.h>
static inline __attribute__((always_inline)) void aborts_when_inlined(int code) { if(code) abort(); }
__attribute__((noinline)) void calls_the_inlined_function(int code) { aborts_when_inlined(code); }
int main(int argc, char** argv) { (void)argv; calls_the_inlined_function(argc); }
STANDIN
{ cc -g1 -O2 -c -o "$tmp/inlining.o" "$tmp/inlining.c" && cc -o "$tmp/$inlining" "$tmp/inlining.o"; } \
  || { echo "FAIL: cannot compile the inlining stand-in"; exit 1; }

mkdir "$tmp/dumps" "$tmp/out"
touch "$tmp/before"
sleep 1

{ "$tmp/$name"; "$tmp/$inlining"; } 2> /dev/null

# The system writes each report a few seconds after the crash.
for stand_in in "$name" "$inlining"; do
  for attempt in $(seq 1 60); do
    find "${reports[@]}" -type f -newer "$tmp/before" -name "$stand_in*" 2> /dev/null | grep -q . && break
    sleep 1
  done
done
sleep 1
touch "$tmp/after"

# These directories stand in for the user's and the system's
# DiagnosticReports. The reports directory holds a copy of the first
# stand-in's report, and reports written here. The unreadable directory can be
# searched but not listed. The unsearchable one can be listed but not searched.
macos=$tmp/macos
mkdir -p "$macos/empty" "$macos/reports/Directory.crash" "$macos/unreadable" "$macos/unsearchable"
cp "$(find "${reports[@]}" -type f -newer "$tmp/before" -name "$name*" 2> /dev/null | head -1)" "$macos/reports/"
python3 - "$macos/reports" <<'FIXTURES'
import json, os, sys
directory = sys.argv[1]
def write(name, text):
    with open(os.path.join(directory, name), 'w') as f:
        f.write(text)
# The sparse report lacks its name, its exception, a frame's symbol, a frame's
# image and a frame's source line. Its second thread faulted.
frames = [{'symbol': 'named_frame', 'sourceFile': 'half.c', 'imageIndex': 0},
          {'imageOffset': 4096, 'imageIndex': 0},
          {'symbol': 'beyond_the_images', 'imageIndex': 1},
          {'symbol': 'without_an_image'}] \
       + [{'symbol': f'frame_{index}', 'imageIndex': 0} for index in range(4, 60)]
body = {'faultingThread': 1,
        'threads': [{'frames': [{'symbol': 'in_a_thread_that_did_not_fault', 'imageIndex': 0}]},
                    {'frames': frames}],
        'usedImages': [{'name': 'SparseImage'}]}
write('Sparse.ips',    '{"bug_type":"309"}\n' + json.dumps(body))
write('Malformed.ips', '{"bug_type":"309","name":"Malformed"}\nnot JSON\n')
write('Hang.ips',      '{"bug_type":"298","name":"Hang"}\n"bug_type":"309"\n')
write('Copy.ips.txt',  '{"bug_type":"309","name":"Copy"}\n{}\n')
write('Old.ips',       '{"bug_type":"309","name":"Old"}\n{}\n')
write('Legacy.crash',  'Process: Legacy [1]\nException Type: EXC_CRASH (SIGABRT)\n')
FIXTURES
touch -t 201901010000 "$macos/reports/Old.ips"
chmod 300 "$macos/unreadable"
chmod 600 "$macos/unsearchable"

# The Linux and Windows dumps each have a since file older than the dumps
# written now.
linux=$tmp/linux
mkdir -p "$linux/dumps/nested" "$linux/dumps/core.Directory.105" "$linux/stubs" "$linux/gdb"
touch "$linux/TestAll"
core_description() { # core_description <execfn>
  echo "ELF 64-bit LSB core file, x86-64, version 1 (SYSV), SVR4-style, from 'TestAll'," \
       "real uid: 1001, effective uid: 1001, real gid: 118, effective gid: 118, execfn: '$1', platform: 'x86_64'"
}
core_description "$linux/TestAll" > "$linux/dumps/core.TestAll.100"
core_description "$linux/Gone"    > "$linux/dumps/core.Gone.101"
echo "ELF 64-bit LSB core file, x86-64, version 1 (SYSV), SVR4-style" > "$linux/dumps/core.Unknown.102"
core_description "$linux/TestAll" > "$linux/dumps/core.Old.103"
core_description "$linux/TestAll" > "$linux/dumps/nested/core.Nested.104"
echo unlimited > "$linux/dumps/core-limit"
touch -t 201901010000 "$linux/dumps/core.Old.103"
touch -t 202001010000 "$linux/dumps/since"
printf '#!/bin/sh\necho Linux\n' > "$linux/stubs/uname"
printf '#!/bin/sh\ncat "$2"\n' > "$linux/stubs/file"
printf '#!/bin/sh\nprintf gdb:; printf " [%%s]" "$@"; echo\n' > "$linux/gdb/gdb"
chmod +x "$linux/stubs/uname" "$linux/stubs/file" "$linux/gdb/gdb"

windows=$tmp/windows
kernels="MINGW64_NT-10.0-20348 MSYS_NT-10.0-20348 CYGWIN_NT-10.0-20348"
mkdir -p "$windows/dumps/symbols" "$windows/dumps/Directory.dmp"
touch "$windows/dumps/TestAll.exe.4242.dmp" "$windows/dumps/Old.dmp" "$windows/dumps/symbols/Nested.dmp" \
      "$windows/dumps/notes.txt"
touch -t 201901010000 "$windows/dumps/Old.dmp"
touch -t 202001010000 "$windows/dumps/since"
for kernel in $kernels; do
  mkdir "$windows/$kernel"
  printf '#!/bin/sh\necho %s\n' "$kernel" > "$windows/$kernel/uname"
  chmod +x "$windows/$kernel/uname"
done

# The path holds the system's tools and not Homebrew's, so no gdb is found
# unless a stub precedes them.
system_path=/usr/bin:/bin:/usr/sbin:/sbin

run_controls() { # run_controls <directory holding the scripts>
  local scripts=$1 arguments status variable kernel
  local script=$scripts/report_crashes.sh
  local crashed=$tmp/out/crashed.txt earlier=$tmp/out/earlier.txt elsewhere=$tmp/out/elsewhere.txt
  local nowhere=$tmp/out/nowhere.txt directories=$tmp/out/directories.txt refused=$tmp/out/refused.txt
  local linux_report=$tmp/out/linux.txt no_gdb=$tmp/out/linux-no-gdb.txt windows_report=$tmp/out/windows.txt
  local gdb_batch='^gdb: \[-batch\] \[-ex\] \[thread apply all bt 50\]'
  fails=0

  "$script" "$tmp/dumps" "$tmp/before" > "$crashed" 2>&1
  check_status "a run reporting crashes succeeds" 0 $?
  check "a crash since the since file is reported"          yes "^== Crash: $name" "$crashed"
  check "the report shows the exception"                    yes "^$name: EXC_CRASH \(SIGABRT\)$" "$crashed"
  check "the report shows the function the crash was in"    yes "^  $name: crashes_in_the_stand_in \(standin\.c:2\)$" \
                                                                "$crashed"
  check "the whole report follows the summary"              yes "^\{\"app_name\":\"$name\"" "$crashed"
  check "a crash since the since file is counted"           yes "^Crashes found: [1-9]" "$crashed"
  check "an optimised crash shows the inlined function"     yes \
        "^  $inlining: aborts_when_inlined \(inlining\.c:2\) \[inlined\]$" "$crashed"
  check "the inlining caller is not marked inlined"         yes \
        "^  $inlining: calls_the_inlined_function \(inlining\.c:3\)$" "$crashed"

  "$script" "$tmp/dumps" "$tmp/after" > "$earlier" 2>&1
  check_status "a run reporting no crash succeeds" 0 $?
  check "a crash before the since file is not reported"     no  "^== Crash: $name" "$earlier"
  check "with no crash since, the count is zero"            yes "^Crashes found: 0$" "$earlier"

  REPORT_CRASHES_MACOS_DIRECTORIES="$macos/empty:$macos/reports:$macos/unreadable:$macos/unsearchable:$macos/absent" \
    "$script" "$tmp/dumps" "$tmp/before" > "$elsewhere" 2>&1
  check_status "a run with directories it cannot read succeeds" 0 $?
  check "a crash in a second directory is reported"         yes "^Report: $macos/reports/$name" "$elsewhere"
  check "every crash report there is counted, and no more"  yes "^Crashes found: 4$" "$elsewhere"
  check "a report of another kind is not reported"          no  "Hang\.ips" "$elsewhere"
  check "a file not named as a report is not reported"      no  "Copy\.ips\.txt" "$elsewhere"
  check "a directory is not reported"                       no  "Directory\.crash" "$elsewhere"
  check "a report before the since file is not reported"    no  "Old\.ips" "$elsewhere"
  check "absent fields are summarised as ?"                 yes "^\?: \? \(\?\)$" "$elsewhere"
  check "a frame with half a location shows none"           yes "^  SparseImage: named_frame$" "$elsewhere"
  check "a frame without a symbol shows its offset"         yes "^  SparseImage: 0x1000$" "$elsewhere"
  check "a frame beyond the images shows ?"                 yes "^  \?: beyond_the_images$" "$elsewhere"
  check "a frame without an image shows ?"                  yes "^  \?: without_an_image$" "$elsewhere"
  check "the summary shows the fiftieth frame"              yes "^  SparseImage: frame_49$" "$elsewhere"
  check "the summary stops at fifty frames"                 no  "^  SparseImage: frame_50$" "$elsewhere"
  check "the summary shows the faulting thread alone"       no  "^  SparseImage: in_a_thread_that_did_not_fault$" \
                                                                "$elsewhere"
  check "a report that cannot be summarised is reported"    yes "^== Crash: Malformed\.ips ==$" "$elsewhere"
  check "a report that cannot be summarised says so"        yes \
        "^The report could not be summarised; it follows whole\.$" "$elsewhere"
  check "a report that cannot be summarised is printed"     yes "^not JSON$" "$elsewhere"
  check "a .crash report is reported"                       yes "^== Crash: Legacy\.crash ==$" "$elsewhere"
  check "a .crash report is printed whole"                  yes "^Exception Type: EXC_CRASH \(SIGABRT\)$" "$elsewhere"
  check "an unreadable directory is named"                  yes "^Not read: $macos/unreadable " "$elsewhere"
  check "an unsearchable directory is named"                yes "^Not read: $macos/unsearchable " "$elsewhere"
  check "an absent directory is not named"                  no  "$macos/absent" "$elsewhere"
  if ! awk '/^== Crash: /{ crash = 1 } /^Not read: / && crash { exit 1 }' "$elsewhere"; then
    fail "every directory not read is named before the first crash"
  fi

  # The run starts in a directory holding a report: find, given no directory,
  # searches the current one.
  ( cd "$macos/reports" && REPORT_CRASHES_MACOS_DIRECTORIES="$macos/absent" \
      "$script" "$tmp/dumps" "$tmp/before" ) > "$nowhere" 2>&1
  check_status "a run with no directory to read succeeds" 0 $?
  [ "$(cat "$nowhere")" = "Crashes found: 0" ] \
    || fail "with no directory to read, the count is the only output: $(cat "$nowhere")"

  for variable in unset empty; do
    ( if [ $variable = unset ]; then unset REPORT_CRASHES_MACOS_DIRECTORIES; else REPORT_CRASHES_MACOS_DIRECTORIES=; fi
      source "$scripts/macos_crash_reports.sh"
      macos_crash_report_directories ) > "$directories"
    check "the user's directory is listed, the variable $variable"   yes "^$HOME/Library/Logs/DiagnosticReports$" \
                                                                         "$directories"
    check "the system's directory is listed, the variable $variable" yes "^/Library/Logs/DiagnosticReports$" \
                                                                         "$directories"
  done

  PATH="$linux/stubs:$linux/gdb:$system_path" "$script" "$linux/dumps" "$linux/dumps/since" > "$linux_report" 2>&1
  check_status "Linux: a run reporting crashes succeeds" 0 $?
  check "Linux: a core names its executable"                yes "^== Crash: $linux/TestAll, core\.TestAll\.100 ==$" \
                                                                "$linux_report"
  check "Linux: gdb reads a core with its executable"       yes \
        "$gdb_batch \[$linux/TestAll\] \[$linux/dumps/core\.TestAll\.100\]$" "$linux_report"
  check "Linux: a core names its absent executable"         yes "^== Crash: $linux/Gone, core\.Gone\.101 ==$" \
                                                                "$linux_report"
  check "Linux: an absent executable is reported"           yes \
        "^The executable was not found, so frames in it are unnamed\.$" "$linux_report"
  check "Linux: gdb reads a core without an absent one"     yes \
        "$gdb_batch \[-c\] \[$linux/dumps/core\.Gone\.101\]$" "$linux_report"
  check "Linux: a core naming no executable is reported"    yes \
        "^== Crash: an unknown executable, core\.Unknown\.102 ==$" "$linux_report"
  check "Linux: gdb reads a core naming no executable"      yes \
        "$gdb_batch \[-c\] \[$linux/dumps/core\.Unknown\.102\]$" "$linux_report"
  check "Linux: a core before the since file is unreported" no  "core\.Old" "$linux_report"
  check "Linux: a core in a subdirectory is not reported"   no  "core\.Nested" "$linux_report"
  check "Linux: the core limit is not reported"             no  "core-limit" "$linux_report"
  check "Linux: a directory is not reported"                no  "core\.Directory" "$linux_report"
  check "Linux: every core since is counted, and no more"   yes "^Crashes found: 3$" "$linux_report"

  PATH="$linux/stubs:$system_path" "$script" "$linux/dumps" "$linux/dumps/since" > "$no_gdb" 2>&1
  check "Linux: with no gdb, the report says so"            yes "^No stacks: gdb is not on PATH\.$" "$no_gdb"
  check "Linux: with no gdb, no gdb runs"                   no  "^gdb:" "$no_gdb"
  check "Linux: with no gdb, every core is counted"         yes "^Crashes found: 3$" "$no_gdb"

  for kernel in $kernels; do
    PATH="$windows/$kernel:$system_path" "$script" "$windows/dumps" "$windows/dumps/since" > "$windows_report" 2>&1
    check_status "Windows ($kernel): a run reporting crashes succeeds" 0 $?
    check "Windows ($kernel): a dump since is reported"        yes "^== Crash: TestAll\.exe\.4242\.dmp ==$" \
                                                                   "$windows_report"
    check "Windows ($kernel): with no cdb, the report says so" yes \
          "^No stacks: cdb\.exe is not installed where the Windows SDK puts it\.$" "$windows_report"
    check "Windows ($kernel): no other file is reported"       no \
          "Old\.dmp|Nested\.dmp|Directory\.dmp|notes\.txt|since" "$windows_report"
    check "Windows ($kernel): every dump since is counted"     yes "^Crashes found: 1$" "$windows_report"
  done

  for arguments in "" "$tmp/dumps" "$tmp/dumps $tmp/before $tmp/before" "$tmp/nowhere $tmp/before" \
                   "$tmp/before $tmp/before" "$tmp/dumps $tmp/nothing" "$tmp/dumps $tmp/dumps"; do
    # Word splitting is the point: each case is a list of arguments.
    # shellcheck disable=SC2086
    "$script" $arguments > "$refused" 2>&1
    status=$?
    [ "$status" -eq 2 ] || fail "the arguments '$arguments' were not refused (status $status)"
    check "the arguments '$arguments' report nothing" no "^Crashes found" "$refused"
  done
}

# Each mutant breaks one behaviour a control above claims. An entry is
# (description, old text, new text), and the text is in the file the array is
# named for.
report_crashes_mutations=(
  'three arguments accepted'          'if [ $# -ne 2 ]; then'                    'if [ $# -lt 2 ]; then'
  'usage refused with status 1'       $'>&2\n  exit 2\nfi\n\ndumps='              $'>&2\n  exit 1\nfi\n\ndumps='
  'any dump directory accepted'       'if [ ! -d "$dumps" ] || '                 'if '
  'any since file accepted'           ' || [ ! -f "$since" ]; then'              '; then'
  'a file as the dump directory'      '[ ! -d "$dumps" ]'                        '[ ! -e "$dumps" ]'
  'a directory as the since file'     '[ ! -f "$since" ]'                        '[ ! -e "$since" ]'
  'macOS not recognised'              'Darwin)'                                  'Darwinian)'
  'MINGW not Windows'                 'MINGW*|'                                  ''
  'MSYS not Windows'                  '|MSYS*|'                                  '|'
  'CYGWIN not Windows'                '|CYGWIN*)'                                ')'
  'a status of failure'               'echo "Crashes found: $found"'             'echo "Crashes found: $found"; false'
  'the count misspelt'                'echo "Crashes found: $found"'             'echo "Crashes: $found"'
  'fewer frames'                      'frames_per_thread=50'                     'frames_per_thread=49'
  'Windows: dumps not counted'        $'found=$((found + 1))\n    echo "== Crash: $(basename "$dump") =="'
                                      $'echo "== Crash: $(basename "$dump") =="'
  'Windows: older dumps read'         "-newer \"\$since\" -name '*.dmp'"         "-name '*.dmp'"
  'Windows: subdirectories read'      "-maxdepth 1 -type f -newer \"\$since\" -name '*.dmp'"
                                      "-type f -newer \"\$since\" -name '*.dmp'"
  'Windows: directories read'        "-maxdepth 1 -type f -newer \"\$since\" -name '*.dmp'"
                                      "-maxdepth 1 -newer \"\$since\" -name '*.dmp'"
  'Windows: any file read'            "-name '*.dmp')"                           "-name '*')"
  'Windows: no word of cdb'           '      echo "No stacks: cdb.exe'           '      : "No stacks: cdb.exe'
  'Linux: cores not counted'          $'found=$((found + 1))\n    executable='   'executable='
  'Linux: older cores read'           "-newer \"\$since\" -name 'core.*'"        "-name 'core.*'"
  'Linux: subdirectories read'        "-maxdepth 1 -type f -newer \"\$since\" -name 'core.*'"
                                      "-type f -newer \"\$since\" -name 'core.*'"
  'Linux: directories read'          "-maxdepth 1 -type f -newer \"\$since\" -name 'core.*'"
                                      "-maxdepth 1 -newer \"\$since\" -name 'core.*'"
  'Linux: the core limit read'        "-name 'core.*')"                          "-name 'core*')"
  'Linux: execfn not read'            "s/.*execfn: '"                            "s/.*execfx: '"
  'Linux: an unknown name blank'      '${executable:-an unknown executable}'     '${executable}'
  'Linux: gdb assumed present'        'if ! command -v gdb > /dev/null; then'    'if false; then'
  'Linux: the executable assumed'     '[ -e "$executable" ]'                     'true'
  'Linux: no word of an absent one'   'echo "The executable was not found'       ': "The executable was not found'
  'Linux: the core alone'             '"$executable" "$core"'                    '-c "$core"'
  'macOS: an absent directory named'  $'[ -d "$directory" ] || continue\n'       ''
  'macOS: unread named after crashes' '      echo "Not read: '
                                      $'      trap \'echo "$unread"\' RETURN; unread+="Not read: '
  'macOS: listing not checked'        '[ -r "$directory" ] && '                  ''
  'macOS: searching not checked'      ' && [ -x "$directory" ]'                  ''
  'macOS: no directory, find anyway'  $'[ ${#directories[@]} -gt 0 ] || return 0\n' ''
  'macOS: older reports read'         '-type f -newer "$since" \('               '-type f \('
  'macOS: directories read'           '"${directories[@]}" -type f '             '"${directories[@]}" '
  'macOS: .ips not read'              "-name '*.ips' -o "                        ''
  'macOS: .crash not read'            " -o -name '*.crash' "                     ' '
  'macOS: .crash not a crash'         '*.crash) return 0 ;;'                     '*.crash) return 1 ;;'
  'macOS: the bug type anywhere'      'head -1 "$1" | grep -q '"'"'"bug_type":"309"'"'"' ;;'
                                      'grep -q '"'"'"bug_type":"309"'"'"' "$1" ;;'
  'macOS: every report a crash'       'is_crash_report "$report" || continue'    'true'
  'macOS: reports not counted'        $'found=$((found + 1))\n    echo "== Crash: $(basename "$report") =="'
                                      $'echo "== Crash: $(basename "$report") =="'
  'macOS: no path'                    'echo "Report: $report"'                   ':'
  'macOS: no whole report'            '    cat "$report"'                        '    :'
  'macOS: no word of a failed summary' '|| echo "The report could not be summarised; it follows whole."'
                                      '|| true'
  'macOS: no exception'               'print(f"{json.loads(metadata)'            '(f"{json.loads(metadata)'
  'macOS: a name required'            "get('name', '?')"                         "__getitem__('name')"
  'macOS: an exception required'      'body.get("exception", {})'                'body["exception"]'
  'macOS: the first thread'           'body["threads"][body["faultingThread"]]'  'body["threads"][0]'
  'macOS: every frame'                '["frames"][:int(sys.argv[2])]'            '["frames"]'
  'macOS: no image is image 0'        'frame.get("imageIndex", -1)'              'frame.get("imageIndex", 0)'
  'macOS: negative images'            'if 0 <= index < len(images)'              'if index < len(images)'
  'macOS: images beyond the list'     'if 0 <= index < len(images)'              'if 0 <= index'
  'macOS: a symbol required'          "frame.get('symbol', hex(frame.get('imageOffset', 0)))"
                                      "frame['symbol']"
  'macOS: half a location suffices'   'if {"sourceFile", "sourceLine"} <= frame.keys()'
                                      'if "sourceFile" in frame'
  'macOS: no location'                "location = f\" ({frame['sourceFile']}:{frame['sourceLine']})\" if"
                                      'location = "" if'
  'macOS: nothing inlined'            'inlined = " [inlined]" if frame.get("inline") else ""'
                                      'inlined = ""'
  'macOS: everything inlined'         'inlined = " [inlined]" if frame.get("inline") else ""'
                                      'inlined = " [inlined]"'
)

macos_crash_reports_mutations=(
  'the variable not split'            "tr ':' '\n'"                              "tr ';' '\n'"
  'an empty variable no directories'  '-n "${REPORT_CRASHES_MACOS_DIRECTORIES:-}"'
                                      '-n "${REPORT_CRASHES_MACOS_DIRECTORIES+set}"'
  "the user's directory not listed"   '"$HOME/Library/Logs/DiagnosticReports" ' ''
  "the system's directory not listed" '" /Library/Logs/DiagnosticReports'        '"'
)

# Prints the number of controls that fail against the scripts in <directory>.
failures_against() { # failures_against <directory>
  run_controls "$1" > /dev/null
  echo "$fails"
}

if [ $mode = controls ]; then
  run_controls "$here/.."
  if [ "$fails" -eq 0 ]; then echo "report_crashes: all controls pass"; else exit 1; fi
  exit 0
fi

survivors=0

# Runs the controls against each mutant of <file>. Each mutant is three
# arguments: description, old text, new text. Sets survivors if a mutant
# survives or cannot be made.
check_mutants() { # check_mutants <file> <description> <old> <new>...
  local file=$1 mutant=$tmp/mutant description failures
  shift
  while [ $# -gt 0 ]; do
    description=$1
    rm -rf "$mutant"
    mkdir "$mutant"
    cp -p "$here"/../report_crashes.sh "$here"/../macos_crash_reports.sh "$here"/../windows_debugger.sh "$mutant/"
    if ! python3 - "$mutant/$file" "$2" "$3" <<'MUTATE'
import sys
path, old, new = sys.argv[1:]
with open(path) as f:
    text = f.read()
if text.count(old) != 1:
    print(f'the text to mutate occurs {text.count(old)} times')
    sys.exit(1)
with open(path, 'w') as f:
    f.write(text.replace(old, new))
MUTATE
    then
      echo "$description: not made"
      survivors=1
    else
      failures=$(failures_against "$mutant")
      echo "$description: $failures controls fail$([ "$failures" -ne 0 ] || echo '  <-- SURVIVED')"
      [ "$failures" -ne 0 ] || survivors=1
    fi
    shift 3
  done
}

failures=$(failures_against "$here/..")
echo "unmutated: $failures controls fail$([ "$failures" -eq 0 ] || echo '  <-- must be none')"
[ "$failures" -eq 0 ] || survivors=1
check_mutants report_crashes.sh      "${report_crashes_mutations[@]}"
check_mutants macos_crash_reports.sh "${macos_crash_reports_mutations[@]}"
exit $survivors
