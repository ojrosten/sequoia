#!/bin/bash
# Controls for snapshot_at_deadline.sh, and a mutation check of the controls.
#
#   snapshot_at_deadline_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls against the script. With
# it, the selftest runs the controls against the script, and then against each
# mutant listed below until a control fails. The script must fail no control,
# and each mutant at least one.
#
# Each control is a claim the script exists to keep:
#   - the script refuses, on standard error and before the command runs:
#       - arguments without a command after `--`;
#       - a deadline which is not a positive whole number;
#       - a snapshot file which exists, or whose directory is missing, is not
#         a directory, or is read-only;
#       - a TMPDIR in which the script cannot create its temporary directory;
#   - the script's temporary directory is under TMPDIR, and is removed;
#   - the command's exit status is the script's, zero or not, before the
#     deadline and after it;
#   - the command reads the script's standard input and writes to its standard
#     output and error;
#   - a command which ends before the deadline returns within the watcher's
#     second, leaves no snapshot and leaves no watcher behind;
#   - a watcher whose script has been killed stops, rather than taking a
#     snapshot at a deadline nobody is waiting for;
#   - at the deadline, while the command still runs, the snapshot says when it
#     began, lists every process with its parent, and shows the stack of every
#     thread of every process with the name, and of no other process;
#   - on macOS, `sample` writes its report into the snapshot and nowhere else;
#   - a snapshot begun is finished before the script returns, even when the
#     command ends while the snapshot is being taken;
#   - with no process of the name, the snapshot says so, rather than ending
#     after the listing as though the process had no threads;
#   - on Linux, the snapshot says when gdb is absent, runs gdb through sudo
#     where sudo needs no password and without it otherwise, and keeps what
#     gdb writes to standard error;
#   - on Windows, the snapshot has a section for each process PowerShell
#     names, and says when cdb is absent.
#
# The Linux and Windows dumpers' controls run on every platform, against
# stand-ins for uname, sudo, gdb and PowerShell, which print what they were
# asked. So they check what the script does with each tool's answer, and not
# the tools. The real tools run on their own platform: `sample` on macOS, and
# gdb on Linux, where gdb must be installed. Nothing here runs PowerShell or
# cdb.
#
# The stand-in for a hung suite is compiled here, blocked in two threads, in
# functions whose names the stacks must show. A system binary copied under
# another name will not do, since macOS kills a copied platform binary on
# launch. The stand-in's name is at most fifteen characters, the part of a
# process's name Linux keeps and pgrep -x compares against. The stand-in ends
# itself after two minutes, in case the selftest is killed before it can.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
original="$here/../snapshot_at_deadline.sh"
name=HungStandIn
decoy=HungStandInToo
tmp=$(mktemp -d)
started=
# Stops every process the selftest started, and removes any report `sample`
# left in /tmp, which only a mutant leaves.
clean_up() {
  [ -z "$started" ] || { kill $started; wait $started; } 2> /dev/null
  started=
  rm -f /tmp/"$name"_*.sample.txt
}
trap 'clean_up; rm -rf "$tmp"' EXIT

case "$(uname -s)" in
  Darwin) platform=macos ;;
  Linux)  platform=linux ;;
  *)      echo "snapshot_at_deadline: the controls run on macOS and Linux; nothing run here"
          exit 0 ;;
esac

fail() {
  echo "FAIL: $1"
  fails=$((fails+1))
  [ "$stop_at_first_failure" = no ] || exit 1
}

check() { # check <name> <yes|no> <pattern> <file>
  if grep -qE "$3" "$4" 2> /dev/null; then got=yes; else got=no; fi
  [ "$got" = "$2" ] || fail "$1 (expected $2, got $got, pattern: $3)"
}

check_status() { # check_status <name> <expected> <seconds> <command>...
  local description=$1 expected=$2 seconds=$3
  shift 3
  rm -f "$work/status.txt"
  bash "$script" "$seconds" "$work/status.txt" NoSuchProcess -- "$@"
  local got=$?
  [ "$got" -eq "$expected" ] || fail "$description (expected status $expected, got $got)"
}

check_refusal() { # check_refusal <name> <TMPDIR> <argument>...
  local description=$1 temporary=$2
  shift 2
  TMPDIR=$temporary bash "$script" "$@" > "$work/refusal.out" 2> "$work/refusal.err"
  local got=$?
  [ "$got" -eq 2 ]             || fail "$description: not refused (status $got)"
  [ ! -e "$work/ran" ]         || fail "$description: refused after running the command"
  [ ! -s "$work/refusal.out" ] || fail "$description: refused on standard output"
  [ -s "$work/refusal.err" ]   || fail "$description: refused without a message"
  rm -f "$work/ran"
}

wait_for() { # wait_for <pattern> <file> <seconds>
  local polls=$(($3 * 10))
  while [ "$polls" -gt 0 ]; do
    grep -qE "$1" "$2" 2> /dev/null && return 0
    sleep 0.1
    polls=$((polls - 1))
  done
  return 1
}

# Runs the script with a command which runs until it is released, and releases
# it once the snapshot is finished. Sets `began_after` to the seconds from the
# script's start to the snapshot's, and `held` to whether the command was still
# running when the snapshot was finished.
held_snapshot() { # held_snapshot <snapshot file> <PATH> <seconds> <executable name>
  local snapshot=$1 search_path=$2 seconds=$3 executable=$4 runner start=$SECONDS
  env PATH="$search_path" "$BASH" "$script" "$seconds" "$snapshot" "$executable" \
    -- "$tmp/held" "$snapshot.release" &
  runner=$!
  started="$started $runner"
  held=no
  if wait_for '^Snapshot (taken|finished) at' "$snapshot" 60; then
    began_after=$((SECONDS - start))
    if wait_for '^Snapshot finished at' "$snapshot" 60 && kill -0 "$runner" 2> /dev/null; then
      held=yes
    fi
  else
    began_after=never
  fi
  touch "$snapshot.release"
  wait "$runner"
}

stacks_of() { # stacks_of <pid> <snapshot file>: the stacks section of process <pid>
  sed -n "/^== Stacks of '$name', process $1 ==\$/,/^Snapshot finished at/p" "$2" \
    | sed '1!{/^== /,$d;}'
}

fake_tool() { # fake_tool <directory> <tool> <body>
  mkdir -p "$1"
  printf '#!/bin/sh\n%s\n' "$3" > "$1/$2"
  chmod +x "$1/$2"
}

cat > "$tmp/standin.c" <<'STANDIN'
#include <pthread.h>
#include <unistd.h>
void* blocked_in_a_second_thread(void* unused) { (void)unused; for(;;) pause(); }
void blocked_in_the_stand_in(void) { for(;;) pause(); }
int main(void) {
  pthread_t thread;
  alarm(120);
  pthread_create(&thread, 0, blocked_in_a_second_thread, 0);
  blocked_in_the_stand_in();
}
STANDIN
for executable in "$name" "$decoy"; do
  cc -g -O0 -pthread -o "$tmp/$executable" "$tmp/standin.c" \
    || { echo "FAIL: cannot compile the stand-in"; exit 1; }
done

# The command of a snapshot's control: it runs until its release file exists or
# that file's directory has gone, for a minute at most.
cat > "$tmp/held" <<'HELD'
#!/bin/sh
polls=0
while [ ! -e "$1" ] && [ -d "${1%/*}" ] && [ "$polls" -lt 600 ]; do
  sleep 0.1
  polls=$((polls + 1))
done
HELD
chmod +x "$tmp/held"

# Stand-ins for the tools of the platforms not run here. The gdb stand-in
# prints its arguments, and says whether sudo ran it.
fake_tool "$tmp/windows" uname 'echo MINGW64_NT-10.0-26100'
fake_tool "$tmp/windows" powershell "case \"\$3\" in
  *\"Get-Process -Name '$name' \"*) printf '4242\\r\\n4343\\r\\n' ;;
  *Get-Process*)                   ;;
  *Get-CimInstance*)               printf '4242 1 20261007 $name.exe\\r\\n' ;;
  *)                               echo \"unexpected: \$*\" >&2; exit 1 ;;
esac"
gdb_stand_in='echo "gdb stand-in${SUDO_STAND_IN:+ through sudo}: $*"
echo "gdb stand-in: cannot attach" >&2
exit 1'
fake_tool "$tmp/linux-sudo"   uname 'echo Linux'
fake_tool "$tmp/linux-sudo"   sudo  '[ "$1" = -n ] && shift; SUDO_STAND_IN=yes exec "$@"'
fake_tool "$tmp/linux-sudo"   gdb   "$gdb_stand_in"
fake_tool "$tmp/linux-nosudo" uname 'echo Linux'
fake_tool "$tmp/linux-nosudo" sudo  'exit 1'
fake_tool "$tmp/linux-nosudo" gdb   "$gdb_stand_in"
# Every tool the script runs before it looks for gdb, and no gdb.
fake_tool "$tmp/linux-nogdb"  uname 'echo Linux'
for tool in dirname mktemp rm touch sleep date ps pgrep; do
  ln -s "$(command -v "$tool")" "$tmp/linux-nogdb/$tool" \
    || { echo "FAIL: cannot find $tool"; exit 1; }
done

run_controls() {
  fails=0
  work=$(mktemp -d "$tmp/controls.XXXXXX")

  # Refusals.
  echo "an earlier snapshot" > "$work/earlier.txt"
  mkdir "$work/read-only"
  chmod 555 "$work/read-only"
  check_refusal "no command"                "$work"         60 "$work/r.txt"             "$name" --
  check_refusal "no separator"              "$work"         60 "$work/r.txt"             "$name" touch "$work/ran"
  for deadline in abc 1.5 0 -600 08 ""; do
    check_refusal "a deadline of '$deadline'" "$work" "$deadline" "$work/r.txt" "$name" -- touch "$work/ran"
  done
  check_refusal "an existing snapshot file" "$work"         60 "$work/earlier.txt"       "$name" -- touch "$work/ran"
  check_refusal "a missing directory"       "$work"         60 "$work/missing/r.txt"     "$name" -- touch "$work/ran"
  check_refusal "a read-only directory"     "$work"         60 "$work/read-only/r.txt"   "$name" -- touch "$work/ran"
  check_refusal "a file for a directory"    "$work"         60 "$work/earlier.txt/r.txt" "$name" -- touch "$work/ran"
  check_refusal "a missing TMPDIR"          "$work/missing" 60 "$work/r.txt"             "$name" -- touch "$work/ran"
  check "an existing snapshot file is left alone" yes "^an earlier snapshot$" "$work/earlier.txt"
  chmod 755 "$work/read-only"

  # The temporary directory, seen by the command and gone afterwards.
  mkdir "$work/temporary"
  TMPDIR=$work/temporary bash "$script" 60 "$work/temporary.txt" "$name" -- ls "$work/temporary" \
    > "$work/temporary.out"
  check "the temporary directory is under TMPDIR" yes "^snapshot_at_deadline\." "$work/temporary.out"
  [ -z "$(ls "$work/temporary")" ] || fail "the temporary directory was left behind"

  # The exit status passes through.
  check_status "success before the deadline"  0 30 true
  check_status "failure before the deadline"  3 30 sh -c 'exit 3'
  check_status "success after the deadline"   0 1  sh -c 'sleep 2'
  check_status "failure after the deadline"   3 1  sh -c 'sleep 2; exit 3'

  # The command's streams.
  echo "to the command" \
    | bash "$script" 30 "$work/streams.txt" "$name" -- sh -c 'cat; echo "to error" >&2' \
        > "$work/streams.out" 2> "$work/streams.err"
  check "the command reads standard input and writes standard output" yes "^to the command$" "$work/streams.out"
  check "the command writes standard error"                            yes "^to error$"       "$work/streams.err"

  # At the deadline, with no such process.
  bash "$script" 1 "$work/absent.txt" "$name" -- sleep 2
  check "an absent process is reported"           yes "^== No process named '$name' is running ==$" "$work/absent.txt"
  check "an absent process has no stacks section" no  "^== Stacks of" "$work/absent.txt"

  # Windows, against the stand-ins for uname and PowerShell. PowerShell ends
  # its lines with a carriage return, which a process id must not keep. cdb is
  # looked for at the Windows SDK's paths, which no other platform has.
  held_snapshot "$work/windows.txt" "$tmp/windows:$PATH" 1 "$name"
  check "Windows: the listing is PowerShell's"   yes "^4242 1 20261007 $name.exe" "$work/windows.txt"
  check "Windows: each process has its section"  yes "^== Stacks of '$name', process 4242 ==$" "$work/windows.txt"
  check "Windows: every process has its section" yes "^== Stacks of '$name', process 4343 ==$" "$work/windows.txt"
  check "Windows: cdb's absence is reported" yes \
    "^No stacks: cdb.exe is not installed where the Windows SDK puts it\.$" "$work/windows.txt"

  # The stand-ins: two with the name, and a decoy whose name begins with it.
  "$tmp/$name" &  first=$!
  "$tmp/$name" &  second=$!
  "$tmp/$decoy" & decoy_pid=$!
  started="$started $first $second $decoy_pid"
  sleep 0.5

  # At the deadline, while the command still runs. The deadline is long enough
  # that a snapshot taken at twice the deadline is seen to be late.
  touch "$work/before-late"
  held_snapshot "$work/late.txt" "$PATH" 4 "$name"
  [ "$held" = yes ] || fail "the snapshot was not finished while the command ran"
  [ "$began_after" != never ] && [ "$began_after" -ge 2 ] && [ "$began_after" -le 6 ] \
    || fail "a snapshot due 4s after the command began began after ${began_after}s"
  check "the snapshot says when it began" yes \
    "^Snapshot taken at [0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z, 4s after the command began\.$" \
    "$work/late.txt"
  check "the snapshot lists the processes" yes "^== Processes ==$" "$work/late.txt"
  check "the listing has every user's processes, pid 1 among them" yes "^ *1 +0 " "$work/late.txt"
  parent=$(ps -o ppid= -p "$first" | tr -d ' ')
  check "the listing has the stand-in, its parent and command line" yes \
    "^ *$first +$parent .*$name" "$work/late.txt"
  check "a process whose name merely begins with the name has no stacks" no \
    "^== Stacks of '$name', process $decoy_pid ==" "$work/late.txt"
  if [ "$platform" = linux ] && ! command -v gdb > /dev/null; then
    fail "gdb is not installed, so the Linux stacks cannot be checked"
  fi
  if [ "$platform" = macos ]; then
    find /tmp/ -maxdepth 1 -name "${name}_*.sample.txt" -newer "$work/before-late" > "$work/samples.txt"
    [ ! -s "$work/samples.txt" ] || fail "sample left its report in /tmp as well"
  fi
  for pid in "$first" "$second"; do
    stacks_of "$pid" "$work/late.txt" > "$work/stacks.$pid.txt"
    check "the stacks of process $pid show its main thread" yes \
      "blocked_in_the_stand_in" "$work/stacks.$pid.txt"
    check "the stacks of process $pid show its second thread" yes \
      "blocked_in_a_second_thread" "$work/stacks.$pid.txt"
  done

  # The command ends while the snapshot is being taken.
  env PATH="$PATH" "$BASH" "$script" 1 "$work/overlap.txt" "$name" \
    -- "$tmp/held" "$work/overlap.release" &
  runner=$!
  started="$started $runner"
  wait_for "^== Stacks of '$name'" "$work/overlap.txt" 60 \
    || fail "the overlapping snapshot never reached the stacks"
  touch "$work/overlap.release"
  wait "$runner"
  check "a snapshot begun is finished before the script returns" yes "^Snapshot finished at" "$work/overlap.txt"

  # Linux's dumper, against the stand-ins for uname, sudo and gdb.
  held_snapshot "$work/linux-sudo.txt" "$tmp/linux-sudo:$PATH" 1 "$name"
  stacks_of "$first" "$work/linux-sudo.txt" > "$work/linux-sudo.stacks"
  check "Linux: gdb runs through sudo where sudo needs no password" yes \
    "^gdb stand-in through sudo: -p $first -batch -ex thread apply all bt$" "$work/linux-sudo.stacks"
  held_snapshot "$work/linux-nosudo.txt" "$tmp/linux-nosudo:$PATH" 1 "$name"
  stacks_of "$first" "$work/linux-nosudo.txt" > "$work/linux-nosudo.stacks"
  check "Linux: gdb runs without sudo where sudo needs a password" yes \
    "^gdb stand-in: -p $first -batch -ex thread apply all bt$" "$work/linux-nosudo.stacks"
  check "Linux: what gdb writes to standard error is kept" yes \
    "^gdb stand-in: cannot attach$" "$work/linux-nosudo.stacks"
  held_snapshot "$work/linux-nogdb.txt" "$tmp/linux-nogdb" 1 "$name"
  stacks_of "$first" "$work/linux-nogdb.txt" > "$work/linux-nogdb.stacks"
  check "Linux: gdb's absence is reported" yes "^No stacks: gdb is not on PATH\.$" "$work/linux-nogdb.stacks"

  clean_up

  # A command which ends before the deadline, many times over, since a race
  # between the command's end and the watcher would show in only some runs.
  # The deadline is one no other control uses, so that a watcher left behind
  # can be found by its command line. The deadline is short, so that a watcher
  # which never learns the command has ended costs one slow return rather than
  # hanging the selftest.
  for trial in $(seq 1 30); do
    start=$SECONDS
    bash "$script" 20 "$work/early.txt" "$name" -- true
    if [ $((SECONDS - start)) -gt 2 ]; then
      fail "a command ending before the deadline took $((SECONDS - start))s to return"
      break
    fi
  done
  [ ! -e "$work/early.txt" ] || fail "a command ending before the deadline left a snapshot"
  ! pgrep -f "snapshot_at_deadline.sh 20 " > /dev/null \
    || fail "a command ending before the deadline left its watcher running"

  # The script killed outright, as a cancelled step kills it, before a deadline
  # three seconds off. The script is killed before its command, so that the
  # script cannot see the command end and tell the watcher. The command is then
  # killed, as the step would kill it. TMPDIR puts the script's temporary
  # directory under this control's, since a kill -9 leaves the directory
  # behind.
  TMPDIR=$work bash "$script" 3 "$work/killed.txt" "$name" -- sleep 30 &
  killed=$!
  started="$started $killed"
  sleep 1
  command=$(pgrep -P "$killed" -x sleep)
  started="$started $command"
  kill -9 "$killed"
  wait "$killed" 2> /dev/null
  kill "$command"
  sleep 4
  [ ! -e "$work/killed.txt" ] || fail "a watcher whose script was killed took a snapshot"
  ! pgrep -f "snapshot_at_deadline.sh 3 " > /dev/null || fail "a watcher whose script was killed is still running"

  clean_up
}

# Each mutant breaks one behaviour a control claims, and is
# <platform> <file> <description> <text> <replacement>, where <platform> is
# the one whose controls must kill it: any, macos or linux.
mutants=0
mutant() {
  mutant_platform[mutants]=$1 mutant_file[mutants]=$2 mutant_description[mutants]=$3
  mutant_text[mutants]=$4 mutant_replacement[mutants]=$5
  mutants=$((mutants + 1))
}
mutant any snapshot_at_deadline.sh 'arguments not counted' \
  '[ $# -lt 5 ] || ' \
  ''
mutant any snapshot_at_deadline.sh 'no separator demanded' \
  ' || [ "$4" != "--" ]' \
  ''
mutant any snapshot_at_deadline.sh 'a deadline of zero accepted' \
  '^[1-9][0-9]*$' \
  '^[0-9]+$'
mutant any snapshot_at_deadline.sh 'a leading zero accepted' \
  '^[1-9][0-9]*$' \
  '^[0-9]*[1-9][0-9]*$'
mutant any snapshot_at_deadline.sh 'a deadline prefix accepted' \
  '^[1-9][0-9]*$' \
  '^[1-9][0-9]*'
mutant any snapshot_at_deadline.sh 'a refusal on stdout' \
  "not '\$seconds'\" >&2" \
  "not '\$seconds'\""
mutant any snapshot_at_deadline.sh 'the status lost' \
  'exit "$status"' \
  'exit 0'
mutant any snapshot_at_deadline.sh 'stdout discarded' \
  $'"$@"\nstatus' \
  $'"$@" > /dev/null\nstatus'
mutant any snapshot_at_deadline.sh 'stdin withheld' \
  $'"$@"\nstatus' \
  $'"$@" < /dev/null\nstatus'
mutant any snapshot_at_deadline.sh 'the end not signalled' \
  'touch "$finished"' \
  ':'
mutant any snapshot_at_deadline.sh 'the end not looked for' \
  '[ -e "$finished" ] || ' \
  ''
mutant any snapshot_at_deadline.sh 'an orphan keeps watching' \
  ' || ! kill -0 $$ 2> /dev/null' \
  ''
mutant any snapshot_at_deadline.sh 'the watcher not waited for' \
  'wait "$watcher"' \
  ':'
mutant any snapshot_at_deadline.sh 'the snapshot discarded' \
  'take_snapshot > "$snapshot"' \
  'take_snapshot > /dev/null'
mutant any snapshot_at_deadline.sh 'complaints discarded' \
  '"$snapshot" 2>&1' \
  '"$snapshot" 2> /dev/null'
mutant any snapshot_at_deadline.sh 'the deadline doubled' \
  '$((SECONDS + seconds))' \
  '$((SECONDS + 2 * seconds))'
mutant any snapshot_at_deadline.sh 'a snapshot at once' \
  '$((SECONDS + seconds))' \
  '$SECONDS'
mutant any snapshot_at_deadline.sh 'no time of beginning' \
  'echo "Snapshot taken at' \
  ': "Snapshot taken at'
mutant any snapshot_at_deadline.sh 'no time of ending' \
  'echo "Snapshot finished at' \
  ': "Snapshot finished at'
mutant any snapshot_at_deadline.sh 'no listing heading' \
  'echo "== Processes =="' \
  ':'
mutant any snapshot_at_deadline.sh 'absence not reported' \
  'echo "== No process named' \
  ': "== No process named'
mutant any snapshot_at_deadline.sh 'the first process alone' \
  'for pid in $pids; do' \
  'for pid in ${pids%%[!0-9]*}; do'
mutant any snapshot_at_deadline.sh 'a name matched as a prefix' \
  'pgrep -x "$name"' \
  'pgrep "$name"'
mutant any snapshot_at_deadline.sh 'a name matched in arguments' \
  'pgrep -x "$name"' \
  'pgrep -f "$name"'
mutant any snapshot_at_deadline.sh 'Windows: CRs kept' \
  " | tr -d '\\r'" \
  ''
mutant any snapshot_at_deadline.sh 'Windows: MINGW not Windows' \
  'MINGW*|MSYS*|CYGWIN*)' \
  'MSYS*|CYGWIN*)'
mutant any snapshot_at_deadline.sh 'Windows: another name' \
  "Get-Process -Name '\$name'" \
  "Get-Process -Name 'TestAll'"
mutant any snapshot_at_deadline.sh 'Windows: cdb absence silent' \
  'echo "No stacks: cdb.exe' \
  ': "No stacks: cdb.exe'
mutant any windows_debugger.sh 'Windows: any cdb path given' \
  'if [ -x "$cdb" ]; then' \
  'if true; then'
mutant any snapshot_at_deadline.sh 'Linux: gdb absence silent' \
  'echo "No stacks: gdb' \
  ': "No stacks: gdb'
mutant any snapshot_at_deadline.sh 'Linux: gdb never looked for' \
  'if ! command -v gdb > /dev/null; then' \
  'if false; then'
mutant any snapshot_at_deadline.sh 'Linux: sudo never' \
  'elif sudo -n true 2> /dev/null; then' \
  'elif false; then'
mutant any snapshot_at_deadline.sh 'Linux: sudo always' \
  'elif sudo -n true 2> /dev/null; then' \
  'elif true; then'
mutant any snapshot_at_deadline.sh 'Linux: one thread, sudo' \
  'sudo -n gdb -p "$1" -batch -ex "thread apply all bt"' \
  'sudo -n gdb -p "$1" -batch -ex "bt"'
mutant any snapshot_at_deadline.sh 'Linux: one thread, no sudo' \
  $'else\n        gdb -p "$1" -batch -ex "thread apply all bt"' \
  $'else\n        gdb -p "$1" -batch -ex "bt"'
mutant any snapshot_at_deadline.sh 'an existing snapshot accepted' \
  '[ -e "$snapshot" ] || ' \
  ''
mutant any snapshot_at_deadline.sh 'a file accepted as a directory' \
  '[ ! -d "$snapshot_directory" ] || ' \
  ''
mutant any snapshot_at_deadline.sh 'a read-only directory accepted' \
  ' || [ ! -w "$snapshot_directory" ]' \
  ''
mutant macos snapshot_at_deadline.sh 'TMPDIR ignored' \
  'mktemp -d "${TMPDIR:-/tmp}/snapshot_at_deadline.XXXXXX"' \
  'mktemp -d'
mutant any snapshot_at_deadline.sh 'no temporary directory accepted' \
  '.XXXXXX") || exit 2' \
  '.XXXXXX")'
mutant any snapshot_at_deadline.sh 'the temporary directory kept' \
  'rm -rf "$flag_dir"' \
  ':'
mutant macos snapshot_at_deadline.sh 'macOS: sampled to a file' \
  ' -file /dev/stdout' \
  ''
mutant macos snapshot_at_deadline.sh "macOS: one user's processes" \
  'ps -axo pid' \
  'ps -xo pid'
mutant macos snapshot_at_deadline.sh 'macOS: no parents' \
  'ps -axo pid,ppid,' \
  'ps -axo pid,'
mutant linux snapshot_at_deadline.sh "Linux: one tty's processes" \
  'ps -eo pid' \
  'ps -o pid'
mutant linux snapshot_at_deadline.sh 'Linux: no parents' \
  'ps -eo pid,ppid,' \
  'ps -eo pid,'

# Runs the controls against the scripts in <directory> until a control fails,
# and prints that control's failure, or nothing.
first_failure() { # first_failure <directory>
  (
    trap clean_up EXIT
    script="$1/snapshot_at_deadline.sh" stop_at_first_failure=yes
    run_controls
  ) > "$tmp/mutation.log" 2>&1
  pkill -f "$1/snapshot_at_deadline.sh" 2> /dev/null
  rm -rf "$tmp"/controls.*
  grep -m 1 '^FAIL: ' "$tmp/mutation.log"
}

mutations() {
  local i survivors=0 file text content stripped occurrences failure
  mkdir "$tmp/unmutated"
  cp "$original" "$here/../windows_debugger.sh" "$tmp/unmutated/"
  failure=$(first_failure "$tmp/unmutated")
  echo "unmutated: ${failure:-no control fails}"
  [ -z "$failure" ] || survivors=1
  for ((i = 0; i < mutants; i++)); do
    if [ "${mutant_platform[i]}" != any ] && [ "${mutant_platform[i]}" != "$platform" ]; then
      echo "${mutant_description[i]}: not run, since its controls are ${mutant_platform[i]}'s"
      continue
    fi
    rm -rf "$tmp/mutant"
    mkdir "$tmp/mutant"
    cp "$original" "$here/../windows_debugger.sh" "$tmp/mutant/"
    file="$tmp/mutant/${mutant_file[i]}" text=${mutant_text[i]}
    content=$(cat "$file"; echo x)
    content=${content%x}
    stripped=${content//"$text"/}
    occurrences=$(( (${#content} - ${#stripped}) / ${#text} ))
    if [ "$occurrences" -ne 1 ]; then
      echo "${mutant_description[i]}: the text to mutate occurs $occurrences times  <-- must be once"
      survivors=1
      continue
    fi
    printf '%s' "${content%%"$text"*}${mutant_replacement[i]}${content#*"$text"}" > "$file"
    failure=$(first_failure "$tmp/mutant")
    if [ -n "$failure" ]; then
      echo "${mutant_description[i]}: killed by ${failure#FAIL: }"
    else
      echo "${mutant_description[i]}: SURVIVED"
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

script=$original stop_at_first_failure=no
run_controls
if [ "$fails" -eq 0 ]; then echo "snapshot_at_deadline: all controls pass on $platform"; else exit 1; fi
