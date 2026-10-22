#!/bin/bash
# Controls for symbol_table_provenance.py, and a mutation check of the
# controls.
#
#   symbol_table_provenance_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls against the script. With
# it, the selftest runs the controls against the script, and then against each
# mutant listed below. The script must fail no control, and each mutant at
# least one. The last line gives the verdict.
#
# Each control is a claim the script exists to keep:
#   - each tool of the record's toolchain gains the name and the version of
#     the package dpkg says owns the tool's path, which need not be the
#     path's last component; the record gains the run's id, attempt and URL;
#     and nothing else in the record changes;
#   - a package whose name carries its architecture is named in full, and a
#     line of dpkg's reporting a diversion names no owner;
#   - a path with no owner, or with two, a package with no version, and a
#     failure of dpkg-query are each refused;
#   - so are a record which is not a JSON object, has no toolchain, has a
#     tool with no path, or already has a package or a run; and a variable
#     of the run which is unset or empty, or an id or attempt which is not a
#     number.
# A refusal writes no record and says why.
#
# A stand-in for dpkg-query answers from files in the fixture, so the
# selftest needs no Debian system.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script="$here/../symbol_table_provenance.py"
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
verbose=yes

fail() {
  fails=$((fails+1))
  if [[ $verbose == yes ]]; then echo "FAIL: $*"; fi
}

# The stand-in for dpkg-query: --search <path> prints $DPKG/search/<path>,
# and --show --showformat=${Version} <package> prints $DPKG/version/<package>.
# Either fails, as dpkg-query does, when it has no answer.
mkdir -p "$tmp/bin"
cat > "$tmp/bin/dpkg-query" <<'EOF'
#!/bin/bash
case "$1 ${2-}" in
  "--search "*)
    cat "$DPKG/search$2" 2> /dev/null || { echo "no path found matching $2" >&2; exit 1; } ;;
  '--show --showformat=${Version}')
    cat "$DPKG/version/$3" 2> /dev/null || { echo "no packages found matching $3" >&2; exit 1; } ;;
  *) echo "the stand-in for dpkg-query was given: $*" >&2; exit 2 ;;
esac
EOF
chmod +x "$tmp/bin/dpkg-query"

# fixture: a record, and dpkg's answers for its three tools
fixture() {
  export DPKG="$work/dpkg"
  mkdir -p "$DPKG/search/usr/bin" "$DPKG/version"
  echo 'g++-15: /usr/bin/g++-15'                   > "$DPKG/search/usr/bin/g++-15"
  echo 'binutils-x86-64-linux-gnu: /usr/bin/ld'    > "$DPKG/search/usr/bin/ld"
  printf '%s\n' 'diversion by nm-wrapper from: /usr/bin/nm' 'diversion by nm-wrapper to: /usr/bin/nm.real' \
                'binutils:amd64: /usr/bin/nm'      > "$DPKG/search/usr/bin/nm"
  echo -n '15.2.0-16ubuntu1' > "$DPKG/version/g++-15"
  echo -n '2.46-1ubuntu2'    > "$DPKG/version/binutils-x86-64-linux-gnu"
  echo -n '2.46-1ubuntu3'    > "$DPKG/version/binutils:amd64"
  cat > "$work/record.json" <<'EOF'
{"commit": "0123456789abcdef0123456789abcdef01234567", "clean": true,
 "executable": {"symbols": 3, "symbol_name_bytes": 30},
 "toolchain": {"compiler": {"path": "/usr/bin/g++-15", "id": "GNU", "version": "15.2.0"},
               "linker":   {"path": "/usr/bin/ld", "id": "GNU", "version": "2.46"},
               "nm":       {"path": "/usr/bin/nm", "version": "GNU nm 2.46"},
               "standard_library": "libstdc++"}}
EOF
}

# run [<option of env>...]: the subject's output in $out, its errors in $err,
# and its exit status in $rc. The run's variables are set as GitHub Actions
# sets them, and then changed by the options given, such as -u <name>.
python=$(command -v python3)
run() {
  env PATH="$tmp/bin:$PATH" GITHUB_SERVER_URL=https://github.com GITHUB_REPOSITORY=owner/repo \
      GITHUB_RUN_ID=39655485487 GITHUB_RUN_ATTEMPT=2 \
    env "$@" "$python" -B "$subject" "$work/record.json" > "$out" 2> "$err"
  rc=$?
}

value() { # value <python expression over the record d>
  python3 -c 'import json, sys
d = json.load(open(sys.argv[1]))
print(eval(sys.argv[2]))' "$out" "$1" 2>&1
}

field() { # field <name> <python expression over the record d> <expected>
  local got
  total=$((total+1))
  got=$(value "$2")
  if [[ $got != "$3" ]]; then fail "$1 (expected '$3', got '$got')"; fi
}

exits() { # exits <name> <status|nonzero>
  total=$((total+1))
  if [[ $2 == nonzero ]]; then
    if [[ $rc -eq 0 ]]; then fail "$1 (expected a non-zero exit, got 0)"; fi
  elif [[ $rc -ne $2 ]]; then fail "$1 (expected exit $2, got $rc: $(cat "$err"))"; fi
}

refuses() { # refuses <name> <pattern>: the run failed, saying why, and wrote no record
  exits "$1 is refused" nonzero
  total=$((total+1))
  if ! grep -qE -- "$2" "$err"; then fail "$1 says why (pattern: $2; said: $(cat "$err"))"; fi
  total=$((total+1))
  if [[ -s $out ]]; then fail "$1 wrote a record"; fi
}

edit_record() { # edit_record <python statement over the record d>
  python3 -c 'import json, sys
d = json.load(open(sys.argv[1]))
exec(sys.argv[2])
json.dump(d, open(sys.argv[1], "w"))' "$work/record.json" "$1"
}

record_controls() {
  fixture
  run
  exits "a record is written" 0
  field "the compiler's package" \
        'd["toolchain"]["compiler"]["package"]' "{'name': 'g++-15', 'version': '15.2.0-16ubuntu1'}"
  field "the linker's package is its path's owner, not its name" \
        'd["toolchain"]["linker"]["package"]' "{'name': 'binutils-x86-64-linux-gnu', 'version': '2.46-1ubuntu2'}"
  field "nm's package is named with its architecture, and past the diversion" \
        'd["toolchain"]["nm"]["package"]' "{'name': 'binutils:amd64', 'version': '2.46-1ubuntu3'}"
  field "the run" 'd["run"]' \
        "{'attempt': 2, 'id': 39655485487, 'url': 'https://github.com/owner/repo/actions/runs/39655485487/attempts/2'}"
  python3 -c 'import json, sys
d = json.load(open(sys.argv[1])); d.pop("run")
for tool in d["toolchain"].values():
    if isinstance(tool, dict): tool.pop("package")
print(d == json.load(open(sys.argv[2])))' "$out" "$work/record.json" > "$work/unchanged" 2>&1
  total=$((total+1))
  if [[ $(cat "$work/unchanged") != True ]]; then
    fail "nothing else in the record changes ($(cat "$work/unchanged"))"
  fi
}

dpkg_controls() {
  fixture
  echo 'g++-15, g++-15-multilib: /usr/bin/g++-15' > "$DPKG/search/usr/bin/g++-15"
  run
  refuses "a path with two owners" 'names 2 packages owning /usr/bin/g\+\+-15'

  fixture
  echo 'diversion by other from: /usr/bin/g++-15' > "$DPKG/search/usr/bin/g++-15"
  run
  refuses "a path whose only line is a diversion" 'names 0 packages owning /usr/bin/g\+\+-15'

  fixture
  rm "$DPKG/search/usr/bin/ld"
  run
  refuses "a path dpkg-query does not know" 'dpkg-query --search /usr/bin/ld failed'

  fixture
  : > "$DPKG/version/g++-15"
  run
  refuses "a package with no version" 'no version for g\+\+-15'

  fixture
  rm "$DPKG/version/binutils:amd64"
  run
  refuses "a package dpkg-query cannot show" 'dpkg-query --show .* binutils:amd64 failed'

  fixture
  run PATH=/nonexistent
  refuses "a missing dpkg-query" 'dpkg-query could not be run'
}

record_shape_controls() {
  fixture
  echo '{"commit": ' > "$work/record.json"
  run
  refuses "a record which is not JSON" 'cannot be read as JSON'

  fixture
  echo '[]' > "$work/record.json"
  run
  refuses "a record which is not an object" 'holds no JSON object'

  fixture
  edit_record 'del d["toolchain"]'
  run
  refuses "a record with no toolchain" 'has no toolchain'

  fixture
  edit_record 'del d["toolchain"]["linker"]["path"]'
  run
  refuses "a tool with no path" 'has a tool with no path'

  fixture
  edit_record 'd["toolchain"]["nm"]["package"] = {"name": "x", "version": "1"}'
  run
  refuses "a tool which already has a package" 'already has a package for /usr/bin/nm'

  fixture
  edit_record 'd["run"] = {"id": 1}'
  run
  refuses "a record which already has a run" 'already has a run'
}

variable_controls() {
  local name
  for name in GITHUB_SERVER_URL GITHUB_REPOSITORY GITHUB_RUN_ID GITHUB_RUN_ATTEMPT; do
    fixture
    run "$name="
    refuses "an empty $name" "$name is unset or empty"
    fixture
    run -u "$name"
    refuses "an unset $name" "$name is unset or empty"
  done
  for name in GITHUB_RUN_ID GITHUB_RUN_ATTEMPT; do
    fixture
    run "$name=12a"
    refuses "a $name which is not a number" "$name is '12a', not a number"
  done
}

run_controls() { # run_controls <script>: sets fails, and total, the controls run
  subject=$1 fails=0 total=0
  local control
  for control in record_controls dpkg_controls record_shape_controls variable_controls; do
    work=$(mktemp -d "$tmp/controls.XXXXXX"); out="$work/out"; err="$work/err"
    "$control"
  done
}

mutants=0
mutant() { # mutant <description> <text> <replacement>
  mutant_description[mutants]=$1 mutant_text[mutants]=$2 mutant_replacement[mutants]=$3
  mutants=$((mutants+1))
}
mutant 'a failed command read'          'if r.returncode:' 'if False:'
mutant 'a command not started crashes'  'except OSError as e:
        sys.exit(f"{command[0]}' 'except ValueError as e:
        sys.exit(f"{command[0]}'
mutant 'any line read as an owner'      'if line.endswith(suffix) and not line.startswith("diversion ")' 'if True'
mutant 'a diversion read as an owner'   ' and not line.startswith("diversion ")' ''
mutant 'split at the first colon'       'line[:-len(suffix)].split(", ")]' 'line.split(":")[0].split(", ")]'
mutant 'the owners not split'           'line[:-len(suffix)].split(", ")]' '[line[:-len(suffix)]]]'
mutant 'the first of several owners'    'if len(owners) != 1:' 'if not owners:'
mutant 'an empty version accepted'      'if not version:' 'if False:'
mutant 'the package named by the path'  'name = owner_of(path)' 'name = owner_of(path) and os.path.basename(path)'
mutant 'an empty variable accepted'     'if not value:' 'if value is None:'
mutant 'any id accepted'                'if not value.isdecimal():' 'if False:'
mutant 'the attempt left out of the URL' '/actions/runs/{run_id}/attempts/{attempt}"' '/actions/runs/{run_id}"'
mutant 'an unreadable record crashes'   'except (OSError, ValueError) as e:' 'except OSError as e:'
mutant 'any JSON accepted'              'if not isinstance(record, dict):' 'if False:'
mutant 'a run overwritten'              'if "run" in record:' 'if False:'
mutant 'no toolchain accepted'          'if not tools:' 'if False:'
mutant 'a tool with no path skipped'    'if "path" not in tool:
            sys.exit' 'if "path" not in tool:
            continue
            sys.exit'
mutant 'a package overwritten'          'if "package" in tool:' 'if False:'
mutant 'the first tool alone'           'tools = [tool for' 'tools = [tool for tool in [] for'

mutations() {
  local i content stripped occurrences survivors=0
  verbose=no
  run_controls "$script"
  if ((fails)); then echo "unmutated: $fails of $total controls fail"; return 1; fi
  echo "unmutated: no control fails"
  for ((i = 0; i < mutants; i++)); do
    content=$(cat "$script"; echo x)
    content=${content%x}
    stripped=${content//"${mutant_text[i]}"/}
    occurrences=$(( (${#content} - ${#stripped}) / ${#mutant_text[i]} ))
    if [ "$occurrences" -ne 1 ]; then
      echo "${mutant_description[i]}: the text to mutate occurs $occurrences times  <-- SURVIVED"
      survivors=$((survivors+1))
      continue
    fi
    printf '%s' "${content%%"${mutant_text[i]}"*}${mutant_replacement[i]}${content#*"${mutant_text[i]}"}" \
      > "$tmp/mutant.py"
    run_controls "$tmp/mutant.py"
    if ((fails)); then
      echo "${mutant_description[i]}: $fails of $total controls fail"
    else
      echo "${mutant_description[i]}: SURVIVED"
      survivors=$((survivors+1))
    fi
  done
  if [ "$survivors" -ne 0 ]; then
    echo "symbol_table_provenance: $survivors of $mutants mutants survived"
    return 1
  fi
  echo "symbol_table_provenance: $mutants mutants, every one killed"
}

case "${1-}" in
  --mutations) mutations; exit ;;
  "")          ;;
  *)           echo "Usage: $0 [--mutations]" >&2; exit 2 ;;
esac

run_controls "$script"
if [[ $fails -eq 0 ]]; then echo "symbol_table_provenance selftest: all $total controls pass"; else
  echo "symbol_table_provenance selftest: $fails of $total controls failed"; fi
exit $((fails > 0))
