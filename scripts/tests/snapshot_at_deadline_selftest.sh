#!/bin/bash
# Controls for snapshot_at_deadline.sh, run on this machine's platform.
#
# The stand-in for a hung suite is compiled here, blocked in a function whose
# name the stacks must show; a system binary copied under another name will not
# do, since macOS kills a copied platform binary on launch. Each control is a
# claim the script exists to keep:
#
#   - the command's exit status is the script's, zero or not, before the
#     deadline and after it; the first CI edition of the watcher, a trap in
#     the workflow, turned a green suite red with the status of its own wait;
#   - a command which ends before the deadline returns within the watcher's
#     second, leaves no snapshot and leaves no watcher behind; an edition
#     which stopped its watcher by signal left the watcher's sleep behind in
#     one run and hung in another;
#   - a watcher whose script has been killed stops, rather than taking a
#     snapshot of nothing at a deadline nobody is waiting for;
#   - at the deadline, the snapshot lists the stand-in among the processes and
#     shows the function it is blocked in, which a snapshot missing its stacks
#     would not;
#   - a snapshot begun is finished before the script returns, even when the
#     command ends while it is being taken;
#   - with no process of the name, the snapshot says so, rather than ending
#     after the listing as though the process had no threads;
#   - a deadline which is not a positive whole number is refused before the
#     command runs, rather than giving no snapshot, or one at once.
#
# The stand-in's name is at most fifteen characters, the part of a process's
# name Linux keeps and pgrep -x compares against. The Windows and Linux stack
# dumpers are exercised only in CI, where their tools are.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script="$here/../snapshot_at_deadline.sh"
tmp=$(mktemp -d)
standin=
trap 'kill $standin 2> /dev/null; rm -rf "$tmp"' EXIT
fails=0

fail() { echo "FAIL: $1"; fails=$((fails+1)); }

check() { # check <name> <yes|no> <pattern> <file>
  if grep -qE "$3" "$4" 2> /dev/null; then got=yes; else got=no; fi
  [ "$got" = "$2" ] || fail "$1 (expected $2, got $got, pattern: $3)"
}

check_status() { # check_status <name> <expected> <seconds> <command>...
  local description=$1 expected=$2 seconds=$3
  shift 3
  bash "$script" "$seconds" "$tmp/status.txt" NoSuchProcess -- "$@"
  local got=$?
  [ "$got" -eq "$expected" ] || fail "$description (expected status $expected, got $got)"
}

name=HungStandIn
cat > "$tmp/standin.c" <<'STANDIN'
#include <unistd.h>
void blocked_in_the_stand_in(void) { pause(); }
int main(void) { blocked_in_the_stand_in(); }
STANDIN
cc -g -O0 -o "$tmp/$name" "$tmp/standin.c" || { echo "FAIL: cannot compile the stand-in"; exit 1; }

# The exit status passes through.
check_status "success before the deadline"  0 60 true
check_status "failure before the deadline"  3 60 sh -c 'exit 3'
check_status "success after the deadline"   0 1  sh -c 'sleep 2'
check_status "failure after the deadline"   3 1  sh -c 'sleep 2; exit 3'

# A command which ends before the deadline, many times over, since what went
# wrong before was a race. The deadline is one no other control uses, so that a
# watcher left behind can be looked for by its command line, and short, so that
# a watcher which never learns the command has ended costs one slow return
# rather than hanging this.
for trial in $(seq 1 30); do
  start=$SECONDS
  bash "$script" 20 "$tmp/early.txt" "$name" -- true
  if [ $((SECONDS - start)) -gt 2 ]; then
    fail "a command ending before the deadline took $((SECONDS - start))s to return"
    break
  fi
done
[ ! -e "$tmp/early.txt" ] || fail "a command ending before the deadline left a snapshot"
! pgrep -f "snapshot_at_deadline.sh 20 " > /dev/null \
  || fail "a command ending before the deadline left its watcher running"

# The script killed outright, as a cancelled step kills it, before a deadline
# three seconds off. It goes first, so that it cannot see its command end and
# tell the watcher; the command is then killed as the step's would be. Its
# temporary directory is put under this one, which a kill -9 leaves behind.
TMPDIR=$tmp bash "$script" 3 "$tmp/killed.txt" "$name" -- sleep 30 &
killed=$!
sleep 1
command=$(pgrep -P "$killed" -x sleep)
kill -9 "$killed"
kill "$command"
sleep 4
[ ! -e "$tmp/killed.txt" ] || fail "a watcher whose script was killed took a snapshot"
! pgrep -f "snapshot_at_deadline.sh 3 " > /dev/null || fail "a watcher whose script was killed is still running"

# At the deadline, with the stand-in running.
"$tmp/$name" &
standin=$!
bash "$script" 1 "$tmp/late.txt" "$name" -- sleep 2
sed -n "/^== Stacks of '$name', process $standin ==/,\$p" "$tmp/late.txt" > "$tmp/stacks.txt"
check "the stand-in is in the process list"        yes "^ *$standin .*$name" "$tmp/late.txt"
check "the stacks show where the stand-in blocked" yes "blocked_in_the_stand_in" "$tmp/stacks.txt"

# A command ending while the snapshot is being taken: the snapshot takes a
# second's sampling at least, so it is still going when the command ends.
bash "$script" 1 "$tmp/overlap.txt" "$name" -- sleep 1.2
check "a snapshot begun is finished before the script returns" yes "^Snapshot finished at" "$tmp/overlap.txt"
kill "$standin"
wait "$standin" 2> /dev/null
standin=

# At the deadline, with no such process.
bash "$script" 1 "$tmp/absent.txt" "$name" -- sleep 2
check "an absent process is reported"           yes "^== No process named '$name' is running ==" "$tmp/absent.txt"
check "an absent process has no stacks section" no  "^== Stacks of" "$tmp/absent.txt"

# Deadlines which are not positive whole numbers.
for deadline in abc 1.5 0 -600 ""; do
  bash "$script" "$deadline" "$tmp/refused.txt" "$name" -- touch "$tmp/ran" 2> /dev/null
  refusal=$?
  [ "$refusal" -eq 2 ] || fail "a deadline of '$deadline' was not refused (status $refusal)"
  [ ! -e "$tmp/ran" ]  || fail "a deadline of '$deadline' was refused after running the command"
  rm -f "$tmp/ran"
done

if [ "$fails" -eq 0 ]; then echo "snapshot_at_deadline: all controls pass"; else exit 1; fi
