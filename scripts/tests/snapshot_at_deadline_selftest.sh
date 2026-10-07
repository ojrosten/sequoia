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
#       - a deadline which is not a positive whole number with no leading
#         zero;
#       - a snapshot file at which something exists, a symbolic link
#         included, or at which the script cannot create a file: an empty
#         path, a path ending in `/`, or one whose directory is missing, is
#         not a directory, or is read-only;
#       - a TMPDIR in which the script cannot create its temporary directory;
#   - the script's temporary directory is under TMPDIR, and is removed;
#   - the command's exit status is the script's, zero or not, before the
#     deadline and after it;
#   - the command reads the script's standard input and writes to its standard
#     output and error, and runs in the caller's process group;
#   - a command which ends before the deadline returns at once, leaves no
#     snapshot, and leaves neither the watcher nor its sleep behind;
#   - a command which ends while the watcher sleeps through the deadline
#     leaves no snapshot;
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
# stand-ins for uname, pgrep, sudo, gdb and PowerShell, which print what they
# were asked. So they check what the script does with each tool's answer, and
# not the tools. The real tools run on their own platform: `sample` on macOS,
# and gdb on Linux, where gdb must be installed. Nothing here runs PowerShell
# or cdb.
#
# The stand-in for a hung suite is compiled here, blocked in two threads, in
# functions whose names the stacks must show. A system binary copied under
# another name will not do, since macOS kills a copied platform binary on
# launch. The stand-in's name is at most fifteen characters, the part of a
# process's name Linux keeps and pgrep -x compares against. The stand-in ends
# itself after two minutes, in case the selftest is killed before it can.
#
# Two controls need permissions the selftest does not arrange, and each fails
# naming the permission when it is missing:
#   - On Linux, the stacks controls need gdb to attach to the stand-ins, which
#     are not gdb's descendants. That takes root, sudo without a password, or
#     a ptrace_scope of 0.
#   - The read-only directory's refusal needs a user who cannot write to a
#     read-only directory, which root can.

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

# The time in tenths of a second. $SECONDS counts whole seconds, which is too
# coarse to time a return expected within about one.
tenths() { perl -MTime::HiRes=time -e 'printf "%d\n", time * 10'; }

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
# it once the snapshot is finished. Fails if the snapshot does not begin within
# 5 s of the deadline, or does not finish within 20 s of beginning. Sets
# `began_after` to the seconds from the start of held_snapshot to the start of
# the snapshot, and `held` to whether the command was still running when the
# snapshot was finished.
held_snapshot() { # held_snapshot <snapshot file> <PATH> <seconds> <executable name>
  local snapshot=$1 search_path=$2 seconds=$3 executable=$4 runner start=$SECONDS
  env PATH="$search_path" "$BASH" "$script" "$seconds" "$snapshot" "$executable" \
    -- "$tmp/held" "$snapshot.release" &
  runner=$!
  started="$started $runner"
  held=no began_after=never
  if ! wait_for '^Snapshot (taken|finished) at' "$snapshot" $((seconds + 5)); then
    fail "a snapshot due ${seconds}s after the command began never began: $snapshot"
  else
    began_after=$((SECONDS - start))
    if ! wait_for '^Snapshot finished at' "$snapshot" 20; then
      fail "a snapshot never finished: $snapshot"
    elif kill -0 "$runner" 2> /dev/null; then
      held=yes
    fi
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

# The command of a snapshot's control. It ends once its release file exists,
# once that file's directory has gone, or after 600 polls a tenth of a second
# apart.
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
# The pgrep stand-in names one process, 4242, for the stand-in's name, so that
# these controls need no stand-in running.
pgrep_stand_in="[ \"\$*\" = \"-x $name\" ] && echo 4242"
fake_tool "$tmp/linux-sudo"   uname 'echo Linux'
fake_tool "$tmp/linux-sudo"   pgrep "$pgrep_stand_in"
fake_tool "$tmp/linux-sudo"   sudo  '[ "$1" = -n ] && shift; SUDO_STAND_IN=yes exec "$@"'
fake_tool "$tmp/linux-sudo"   gdb   "$gdb_stand_in"
fake_tool "$tmp/linux-nosudo" uname 'echo Linux'
fake_tool "$tmp/linux-nosudo" pgrep "$pgrep_stand_in"
fake_tool "$tmp/linux-nosudo" sudo  'exit 1'
fake_tool "$tmp/linux-nosudo" gdb   "$gdb_stand_in"
# Every tool the script runs, and no gdb. The script asks for sudo only once
# it has found gdb.
fake_tool "$tmp/linux-nogdb"  uname 'echo Linux'
fake_tool "$tmp/linux-nogdb"  pgrep "$pgrep_stand_in"
for tool in dirname mktemp mkdir rm touch sleep date ps; do
  ln -s "$(command -v "$tool")" "$tmp/linux-nogdb/$tool" \
    || { echo "FAIL: cannot find $tool"; exit 1; }
done

# A stand-in for sleep which holds a one-second sleep until $SLEEP_RELEASE
# exists or its directory has gone, for 600 polls at most, and passes any other
# sleep to the real one.
real_sleep=$(command -v sleep)

# A gdb stand-in which takes a second, so that a snapshot is still being taken
# when the command ends.
fake_tool "$tmp/linux-slow" uname 'echo Linux'
fake_tool "$tmp/linux-slow" pgrep "$pgrep_stand_in"
fake_tool "$tmp/linux-slow" sudo  'exit 1'
fake_tool "$tmp/linux-slow" gdb   "$real_sleep 1"
fake_tool "$tmp/sleeping" sleep "[ \"\$1\" = 1 ] || exec $real_sleep \"\$@\"
polls=0
while [ ! -e \"\$SLEEP_RELEASE\" ] && [ -d \"\${SLEEP_RELEASE%/*}\" ] && [ \"\$polls\" -lt 600 ]; do
  $real_sleep 0.1
  polls=\$((polls + 1))
done"

run_controls() {
  fails=0
  work=$(mktemp -d "$tmp/controls.XXXXXX")

  # The quick controls come first, and those which wait for a deadline or a
  # snapshot last, since --mutations stops a mutant at its first failure.

  # Refusals.
  echo "an earlier snapshot" > "$work/earlier.txt"
  mkdir "$work/read-only"
  chmod 555 "$work/read-only"
  ln -s "$work/missing/r.txt" "$work/into-missing"
  ln -s "$work/target.txt"    "$work/to-new"
  check_refusal "no command"                "$work"         1  "$work/r.txt"             "$name" --
  check_refusal "no separator"              "$work"         1  "$work/r.txt"             "$name" touch "$work/ran"
  for deadline in abc 1.5 0 -600 08 ""; do
    check_refusal "a deadline of '$deadline'" "$work" "$deadline" "$work/r.txt" "$name" -- touch "$work/ran"
  done
  check_refusal "an existing snapshot file" "$work"         1  "$work/earlier.txt"       "$name" -- touch "$work/ran"
  check_refusal "a missing directory"       "$work"         1  "$work/missing/r.txt"     "$name" -- touch "$work/ran"
  if { : > "$work/read-only/probe"; } 2> /dev/null; then
    fail "the read-only directory is writable, as it is to root, so its refusal cannot be checked"
  else
    check_refusal "a read-only directory"   "$work"         1  "$work/read-only/r.txt"   "$name" -- touch "$work/ran"
  fi
  check_refusal "a file for a directory"    "$work"         1  "$work/earlier.txt/r.txt" "$name" -- touch "$work/ran"
  check_refusal "an empty path"             "$work"         1  ""                        "$name" -- touch "$work/ran"
  check_refusal "a path ending in /"        "$work"         1  "$work/absent/"           "$name" -- touch "$work/ran"
  check_refusal "a link into a missing directory" \
                                            "$work"         1  "$work/into-missing"      "$name" -- touch "$work/ran"
  check_refusal "a link to a new file"      "$work"         1  "$work/to-new"            "$name" -- touch "$work/ran"
  check_refusal "a missing TMPDIR"          "$work/missing" 1  "$work/r.txt"             "$name" -- touch "$work/ran"
  check "an existing snapshot file is left alone" yes "^an earlier snapshot$" "$work/earlier.txt"
  [ ! -e "$work/target.txt" ] || fail "a refused link to a new file created the file"
  chmod 755 "$work/read-only"

  # A command which ends before the deadline, many times over, since a race
  # between the command's end and the watcher would show in only some runs.
  # The fastest trial shows whether the script returns at once, whatever the
  # runner's load. Every trial must return within 0.9 s, short of the
  # watcher's one-second sleep, which a script that waits for the watcher
  # would wait out. A watcher left behind has the snapshot file's path in its
  # command line.
  fastest=
  for trial in $(seq 1 "$early_return_trials"); do
    start=$(tenths)
    bash "$script" 5 "$work/early.txt" "$name" -- true
    elapsed=$(($(tenths) - start))
    [ -n "$fastest" ] && [ "$fastest" -le "$elapsed" ] || fastest=$elapsed
    if [ "$elapsed" -gt 9 ]; then
      fail "a command ending before the deadline took $((elapsed / 10)).$((elapsed % 10))s to return"
      break
    fi
  done
  [ "$fastest" -le 5 ] \
    || fail "a command ending at once took $((fastest / 10)).$((fastest % 10))s to return, at the fastest"
  [ ! -e "$work/early.txt" ] || fail "a command ending before the deadline left a snapshot"
  ! pgrep -f "$work/early.txt" > /dev/null \
    || fail "a command ending before the deadline left its watcher running"

  # The watcher's sleep ends with the script. A stand-in for sleep marks that
  # it ran, and runs the real sleep as its child, so that the stand-in's
  # command line names it. The stand-in sleeps 5 s whatever it is asked, so
  # that a sleep left behind outlives the check by seconds. The command lasts
  # long enough for the watcher to begin its sleep. The stand-in is the run's
  # own, so that no other run's sleep is taken for this one's.
  fake_tool "$work/marked" sleep "touch \"\$SLEEP_MARK\"
$real_sleep 5"
  env PATH="$work/marked:$PATH" SLEEP_MARK="$work/slept" \
    "$BASH" "$script" 5 "$work/marked.txt" "$name" -- "$real_sleep" 0.3
  if [ ! -e "$work/slept" ]; then
    fail "the watcher never slept, so the end of its sleep could not be checked"
  fi
  for poll in 1 2 3 4 5; do
    pgrep -f "$work/marked/sleep" > /dev/null || break
    sleep 0.1
  done
  ! pgrep -f "$work/marked/sleep" > /dev/null || fail "the watcher's sleep outlived the script"

  # The temporary directory, seen by the command and gone afterwards.
  mkdir "$work/temporary"
  TMPDIR=$work/temporary bash "$script" 5 "$work/temporary.txt" "$name" -- ls "$work/temporary" \
    > "$work/temporary.out"
  check "the temporary directory is under TMPDIR" yes "^snapshot_at_deadline\." "$work/temporary.out"
  [ -z "$(ls "$work/temporary")" ] || fail "the temporary directory was left behind"

  # The exit status passes through before the deadline.
  check_status "success before the deadline"  0 5  true
  check_status "failure before the deadline"  3 5  sh -c 'exit 3'

  # The command's streams.
  echo "to the command" \
    | bash "$script" 5 "$work/streams.txt" "$name" -- sh -c 'cat; echo "to error" >&2' \
        > "$work/streams.out" 2> "$work/streams.err"
  check "the command reads standard input and writes standard output" yes "^to the command$" "$work/streams.out"
  check "the command writes standard error"                            yes "^to error$"       "$work/streams.err"

  # The command runs in the caller's process group, as it would without the
  # script.
  bash "$script" 5 "$work/group.txt" "$name" -- sh -c 'ps -o pgid= -p $$' > "$work/group.out"
  check "the command runs in the caller's process group" yes \
    "^ *$(ps -o pgid= -p $$ | tr -d ' ')\$" "$work/group.out"

  # Windows, against the stand-ins for uname and PowerShell. PowerShell ends
  # its lines with a carriage return, which a process id must not keep. cdb is
  # looked for at the Windows SDK's paths, which no other platform has.
  held_snapshot "$work/windows.txt" "$tmp/windows:$PATH" 1 "$name"
  check "the snapshot says when it began" yes \
    "^Snapshot taken at [0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z, 1s after the command began\.$" \
    "$work/windows.txt"
  check "the snapshot lists the processes" yes "^== Processes ==$" "$work/windows.txt"
  check "Windows: the listing is PowerShell's"   yes "^4242 1 20261007 $name.exe" "$work/windows.txt"
  check "Windows: each process has its section"  yes "^== Stacks of '$name', process 4242 ==$" "$work/windows.txt"
  check "Windows: every process has its section" yes "^== Stacks of '$name', process 4343 ==$" "$work/windows.txt"
  check "Windows: cdb's absence is reported" yes \
    "^No stacks: cdb.exe is not installed where the Windows SDK puts it\.$" "$work/windows.txt"

  # Linux's dumper, against the stand-ins for uname, pgrep, sudo and gdb.
  held_snapshot "$work/linux-sudo.txt" "$tmp/linux-sudo:$PATH" 1 "$name"
  stacks_of 4242 "$work/linux-sudo.txt" > "$work/linux-sudo.stacks"
  check "Linux: gdb runs through sudo where sudo needs no password" yes \
    "^gdb stand-in through sudo: -p 4242 -batch -ex thread apply all bt$" "$work/linux-sudo.stacks"
  held_snapshot "$work/linux-nosudo.txt" "$tmp/linux-nosudo:$PATH" 1 "$name"
  stacks_of 4242 "$work/linux-nosudo.txt" > "$work/linux-nosudo.stacks"
  check "Linux: gdb runs without sudo where sudo needs a password" yes \
    "^gdb stand-in: -p 4242 -batch -ex thread apply all bt$" "$work/linux-nosudo.stacks"
  check "Linux: what gdb writes to standard error is kept" yes \
    "^gdb stand-in: cannot attach$" "$work/linux-nosudo.stacks"
  held_snapshot "$work/linux-nogdb.txt" "$tmp/linux-nogdb" 1 "$name"
  stacks_of 4242 "$work/linux-nogdb.txt" > "$work/linux-nogdb.stacks"
  check "Linux: gdb's absence is reported" yes "^No stacks: gdb is not on PATH\.$" "$work/linux-nogdb.stacks"

  # The command ends while the snapshot is being taken: a gdb stand-in takes a
  # second over the stacks.
  env PATH="$tmp/linux-slow:$PATH" "$BASH" "$script" 1 "$work/overlap.txt" "$name" \
    -- "$tmp/held" "$work/overlap.release" &
  runner=$!
  started="$started $runner"
  wait_for "^== Stacks of '$name'" "$work/overlap.txt" 10 \
    || fail "the overlapping snapshot never reached the stacks"
  touch "$work/overlap.release"
  wait "$runner"
  check "a snapshot begun is finished before the script returns" yes "^Snapshot finished at" "$work/overlap.txt"

  # At the deadline, with no such process.
  bash "$script" 1 "$work/absent.txt" "$name" -- sleep 2
  check "an absent process is reported"           yes "^== No process named '$name' is running ==$" "$work/absent.txt"
  check "an absent process has no stacks section" no  "^== Stacks of" "$work/absent.txt"
  check "the listing has every user's processes, pid 1 among them" yes "^ *1 +0 " "$work/absent.txt"

  # The script killed outright, as a cancelled step kills it, before a deadline
  # three seconds off. The control waits until the script has started both its
  # watcher and its command. It kills the script before the command, so that
  # the script cannot see the command end and end the watcher, and then kills
  # the command, as the step would. The watcher must be gone well before the
  # deadline. TMPDIR puts the script's temporary directory under this
  # control's, since a kill -9 leaves the directory behind.
  TMPDIR=$work bash "$script" 3 "$work/killed.txt" "$name" -- sleep 30 &
  killed=$!
  started="$started $killed"
  watcher= command=
  for poll in $(seq 1 50); do
    watcher=$(pgrep -P "$killed" -x bash) command=$(pgrep -P "$killed" -x sleep)
    [ -z "$watcher" ] || [ -z "$command" ] || break
    sleep 0.1
  done
  started="$started $watcher $command"
  if [ -z "$watcher" ] || [ -z "$command" ]; then
    fail "the killed script's watcher and command did not both start"
  else
    kill -9 "$killed"
    wait "$killed" 2> /dev/null
    kill "$command"
    for poll in $(seq 1 20); do
      kill -0 "$watcher" 2> /dev/null || break
      sleep 0.1
    done
    ! kill -0 "$watcher" 2> /dev/null || fail "a watcher whose script was killed is still running"
    [ ! -e "$work/killed.txt" ] || fail "a watcher whose script was killed took a snapshot"
  fi

  # The exit status passes through after the deadline.
  check_status "success after the deadline"   0 1  sh -c 'sleep 2'
  check_status "failure after the deadline"   3 1  sh -c 'sleep 2; exit 3'

  # A command which ends after the deadline has passed, while the watcher
  # sleeps. A stand-in for sleep holds the watcher's one-second sleep until
  # the command has ended. The watcher must not snapshot: the script either
  # ends it, or the watcher wakes to find the snapshot claimed. The deadline is
  # 2 s, so that the watcher is not already past the deadline when it first
  # looks.
  mkdir "$work/sleeping"
  env PATH="$tmp/sleeping:$PATH" TMPDIR="$work/sleeping" SLEEP_RELEASE="$work/sleeping.wake" \
    "$BASH" "$script" 2 "$work/sleeping.txt" "$name" -- "$tmp/held" "$work/sleeping.release" &
  runner=$!
  started="$started $runner"
  sleep 2.5
  touch "$work/sleeping.release"
  for poll in $(seq 1 50); do
    [ -z "$(find "$work/sleeping" -name claim)" ] || break
    sleep 0.1
  done
  touch "$work/sleeping.wake"
  wait "$runner"
  [ ! -e "$work/sleeping.txt" ] || fail "a command which ended while the watcher slept left a snapshot"

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
  check "the snapshot says how long after the command it began" yes \
    "^Snapshot taken at .*, 4s after the command began\.$" "$work/late.txt"
  parent=$(ps -o ppid= -p "$first" | tr -d ' ')
  check "the listing has the stand-in, its parent and command line" yes \
    "^ *$first +$parent .*$name" "$work/late.txt"
  check "a process whose name merely begins with the name has no stacks" no \
    "^== Stacks of '$name', process $decoy_pid ==" "$work/late.txt"
  if [ "$platform" = linux ]; then
    if ! command -v gdb > /dev/null; then
      fail "gdb is not installed, so the Linux stacks cannot be checked"
    elif [ "$(id -u)" != 0 ] && ! sudo -n true 2> /dev/null \
           && [ "$(cat /proc/sys/kernel/yama/ptrace_scope 2> /dev/null || echo 0)" != 0 ]; then
      fail "gdb cannot attach to the stand-ins: that takes root, sudo without a password, or a ptrace_scope of 0"
    fi
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
mutant any snapshot_at_deadline.sh 'the watcher not ended' \
  '{ kill -TERM -- -"$watcher" || kill -TERM "$watcher"; } 2> /dev/null' \
  ':'
mutant any snapshot_at_deadline.sh "the watcher's sleep left" \
  'kill -TERM -- -"$watcher" || ' \
  ''
mutant any snapshot_at_deadline.sh 'no process group' \
  $'set -m\nwatch_for_deadline' \
  $':\nwatch_for_deadline'
mutant any snapshot_at_deadline.sh 'job control left on' \
  'set +m' \
  ':'
mutant any snapshot_at_deadline.sh 'the script claims nothing' \
  'if mkdir "$claim" 2> /dev/null; then' \
  'if true; then'
mutant any snapshot_at_deadline.sh 'the watcher claims nothing' \
  'mkdir "$claim" 2> /dev/null && take_snapshot' \
  'take_snapshot'
mutant any snapshot_at_deadline.sh 'an orphan keeps watching' \
  'while kill -0 $$ 2> /dev/null; do' \
  'while :; do'
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
mutant any snapshot_at_deadline.sh 'a symbolic link accepted' \
  '[ -L "$snapshot" ] || ' \
  ''
mutant any snapshot_at_deadline.sh 'no trial creation' \
  ' || ! { : > "$snapshot"; } 2> /dev/null' \
  ''
mutant any snapshot_at_deadline.sh 'the trial file kept' \
  'rm -f "$snapshot"' \
  ':'
mutant any snapshot_at_deadline.sh 'TMPDIR ignored' \
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
# and prints that control's failure, or nothing. The early return is tried 3
# times, not 30. The 30 trials guard against a race which shows in only some
# runs, which no mutant introduces, and a mutant which breaks the early return
# fails the first trial.
first_failure() { # first_failure <directory>
  (
    trap clean_up EXIT
    script="$1/snapshot_at_deadline.sh" stop_at_first_failure=yes early_return_trials=3
    run_controls
  ) > "$tmp/mutation.log" 2>&1
  pkill -f "$1/snapshot_at_deadline.sh" 2> /dev/null
  rm -rf "$tmp"/controls.*
  grep -m 1 '^FAIL: ' "$tmp/mutation.log"
}

mutations() {
  local i survivors=0 file text content stripped occurrences failure start
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
    start=$SECONDS
    failure=$(first_failure "$tmp/mutant")
    if [ -n "$failure" ]; then
      echo "${mutant_description[i]}: killed in $((SECONDS - start))s by ${failure#FAIL: }"
    else
      echo "${mutant_description[i]}: SURVIVED, after $((SECONDS - start))s"
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

script=$original stop_at_first_failure=no early_return_trials=30
run_controls
if [ "$fails" -eq 0 ]; then echo "snapshot_at_deadline on $platform: all controls pass"; else exit 1; fi
