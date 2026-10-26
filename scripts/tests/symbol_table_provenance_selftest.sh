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
#     the package dpkg says owns the tool's executable, which need not be the
#     path's last component; where the path is a symlink, the package is the
#     one owning the file it resolves to, not the one owning the symlink; the
#     record gains the run's id, attempt and URL, and the model name of the
#     CPU, which is the "Model name" line of lscpu, not the "BIOS Model name"
#     line before it, and the first of several; and nothing else in the
#     record changes;
#   - a path is resolved through a chain of symlinks, not one link;
#   - a package whose name carries its architecture is named in full, and a
#     line of dpkg's naming another path names no owner;
#   - a path with no owner, or with two, a diversion of the path by a package
#     or a local one, a package with no version, and a failure of dpkg-query
#     are each refused;
#   - dpkg-query and lscpu run in the C locale, whatever the caller's;
#   - lscpu failing, printing no "Model name" line, or printing one with no
#     name, is refused;
#   - so are a record which is not a JSON object, has no toolchain, has a
#     tool with no path, or already has a package or a run; and a variable
#     of the run which is unset or empty, or an id or attempt which is not a
#     number.
# A refusal writes no record and says why.
#
# Stand-ins for dpkg-query and lscpu answer from files in the fixture, so the
# selftest needs no Debian system. The tools are files in the fixture, so
# that what a path resolves to is the fixture's, not the system's: Ubuntu's
# /usr/bin/ld is a symlink.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script="$here/../symbol_table_provenance.py"
# Resolved, since the script resolves each tool's path: on macOS, the
# temporary directory is under /var, a symlink to /private/var.
tmp=$(cd "$(mktemp -d)" && pwd -P); trap 'rm -rf "$tmp"' EXIT
verbose=yes

fail() {
  fails=$((fails+1))
  if [[ $verbose == yes ]]; then echo "FAIL: $*"; fi
}

# The stand-in for dpkg-query: --search <path> prints $DPKG/search/<path>,
# and --show --showformat=${Version} <package> prints $DPKG/version/<package>.
# Either fails, as dpkg-query does, when it has no answer. It refuses to run
# outside the C locale, since a translated answer is not what the script
# reads.
mkdir -p "$tmp/bin"
cat > "$tmp/bin/dpkg-query" <<'EOF'
#!/bin/bash
if [ "${LC_ALL-}" != C ]; then echo "the stand-in for dpkg-query was run with LC_ALL=${LC_ALL-}" >&2; exit 2; fi
case "$1 ${2-}" in
  "--search "*)
    cat "$DPKG/search$2" 2> /dev/null || { echo "no path found matching $2" >&2; exit 1; } ;;
  '--show --showformat=${Version}')
    cat "$DPKG/version/$3" 2> /dev/null || { echo "no packages found matching $3" >&2; exit 1; } ;;
  *) echo "the stand-in for dpkg-query was given: $*" >&2; exit 2 ;;
esac
EOF
chmod +x "$tmp/bin/dpkg-query"

# The stand-in for lscpu prints $LSCPU, a file of the fixture, and fails when
# it is absent. It refuses to run outside the C locale, as the stand-in for
# dpkg-query does.
cat > "$tmp/bin/lscpu" <<'EOF'
#!/bin/bash
if [ "${LC_ALL-}" != C ]; then echo "the stand-in for lscpu was run with LC_ALL=${LC_ALL-}" >&2; exit 2; fi
cat "$LSCPU" 2> /dev/null || { echo "lscpu: cannot be run" >&2; exit 1; }
EOF
chmod +x "$tmp/bin/lscpu"

# fixture: a record, its three tools, and dpkg's answers for them. The
# compiler's path is a symlink to $compiler_file, and one package owns the
# symlink, another the file it resolves to. The linker's path is a chain of
# two symlinks, ending at $linker_file, and dpkg has no answer for the link
# between.
fixture() {
  local bin="$work/usr/bin"
  compiler_file="$bin/x86_64-linux-gnu-g++-15"
  linker_file="$bin/x86_64-linux-gnu-ld.bfd"
  export DPKG="$work/dpkg" LSCPU="$work/lscpu"
  printf '%s\n' 'Architecture:                    x86_64' \
                'BIOS Model name:                 Virtual CPU (BIOS)' \
                'Model name:                      AMD EPYC 7763 64-Core Processor' \
                'CPU family:                      25' > "$LSCPU"
  mkdir -p "$bin" "$DPKG/search$bin" "$DPKG/version"
  touch "$compiler_file" "$linker_file" "$bin/nm"
  ln -sf x86_64-linux-gnu-g++-15 "$bin/g++-15"
  ln -sf x86_64-linux-gnu-ld.bfd "$bin/x86_64-linux-gnu-ld"
  ln -sf x86_64-linux-gnu-ld "$bin/ld"
  echo "g++-15: $bin/g++-15"                             > "$DPKG/search$bin/g++-15"
  echo "g++-15-x86-64-linux-gnu: $compiler_file"         > "$DPKG/search$compiler_file"
  echo "binutils-x86-64-linux-gnu: $linker_file"         > "$DPKG/search$linker_file"
  printf '%s\n' "binutils-doc: $bin/nm.1" "binutils:amd64: $bin/nm" > "$DPKG/search$bin/nm"
  echo -n '15.2.0-16ubuntu0' > "$DPKG/version/g++-15"
  echo -n '15.2.0-16ubuntu1' > "$DPKG/version/g++-15-x86-64-linux-gnu"
  echo -n '2.46-1ubuntu2'    > "$DPKG/version/binutils-x86-64-linux-gnu"
  echo -n '2.46-1ubuntu3'    > "$DPKG/version/binutils:amd64"
  cat > "$work/record.json" <<EOF
{"commit": "0123456789abcdef0123456789abcdef01234567", "clean": true,
 "executable": {"symbols": 3, "symbol_name_bytes": 30},
 "toolchain": {"compiler": {"path": "$bin/g++-15", "id": "GNU", "version": "15.2.0"},
               "linker":   {"path": "$bin/ld", "id": "GNU", "version": "2.46"},
               "nm":       {"path": "$bin/nm", "version": "GNU nm 2.46"},
               "standard_library": "libstdc++"}}
EOF
}

# run [<option of env>...]: the subject's output in $out, its errors in $err,
# and its exit status in $rc. GitHub Actions' variables are set as it sets
# them, and LC_ALL to a locale other than C; then the options given, such as
# -u <name>, change them.
python=$(command -v python3)
run() {
  env PATH="$tmp/bin:$PATH" GITHUB_SERVER_URL=https://github.com GITHUB_REPOSITORY=owner/repo \
      GITHUB_RUN_ID=39655485487 GITHUB_RUN_ATTEMPT=2 LC_ALL=de_DE.UTF-8 \
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
  field "the compiler's package owns its executable, not the symlink to it" \
        'd["toolchain"]["compiler"]["package"]' "{'name': 'g++-15-x86-64-linux-gnu', 'version': '15.2.0-16ubuntu1'}"
  field "the linker's package is its path's owner, not its name" \
        'd["toolchain"]["linker"]["package"]' "{'name': 'binutils-x86-64-linux-gnu', 'version': '2.46-1ubuntu2'}"
  field "nm's package is named with its architecture, past a line for another path" \
        'd["toolchain"]["nm"]["package"]' "{'name': 'binutils:amd64', 'version': '2.46-1ubuntu3'}"
  field "the run" 'd["run"]' \
        "{'attempt': 2, 'cpu_model': 'AMD EPYC 7763 64-Core Processor', 'id': 39655485487, 'url': 'https://github.com/owner/repo/actions/runs/39655485487/attempts/2'}"
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
  echo "g++-15-x86-64-linux-gnu, g++-15-multilib: $compiler_file" > "$DPKG/search$compiler_file"
  run
  refuses "a path with two owners" 'names 2 packages owning .*/usr/bin/x86_64-linux-gnu-g\+\+-15'

  fixture
  echo "binutils-doc: $compiler_file.1" > "$DPKG/search$compiler_file"
  run
  refuses "a path whose only line names another path" 'names 0 packages owning .*/usr/bin/x86_64-linux-gnu-g\+\+-15'

  fixture
  printf '%s\n' "diversion by nm-wrapper from: $work/usr/bin/nm" "diversion by nm-wrapper to: $work/usr/bin/nm.real" \
                "binutils:amd64: $work/usr/bin/nm" > "$DPKG/search$work/usr/bin/nm"
  run
  refuses "a path diverted by a package" 'reports a diversion of .*/usr/bin/nm'

  fixture
  printf '%s\n' "local diversion from: $work/usr/bin/nm" "local diversion to: $work/usr/bin/nm.distrib" \
                "binutils:amd64: $work/usr/bin/nm" > "$DPKG/search$work/usr/bin/nm"
  run
  refuses "a path diverted locally" 'reports a diversion of .*/usr/bin/nm'

  fixture
  rm "$DPKG/search$linker_file"
  run
  refuses "a path dpkg-query does not know" 'dpkg-query --search .*/usr/bin/x86_64-linux-gnu-ld.bfd failed'

  fixture
  : > "$DPKG/version/g++-15-x86-64-linux-gnu"
  run
  refuses "a package with no version" 'no version for g\+\+-15-x86-64-linux-gnu'

  fixture
  rm "$DPKG/version/binutils:amd64"
  run
  refuses "a package dpkg-query cannot show" 'dpkg-query --show .* binutils:amd64 failed'

  fixture
  run PATH=/nonexistent
  refuses "a missing dpkg-query" 'dpkg-query could not be run'
}

cpu_controls() {
  fixture
  printf '%s\n' 'Model name:                      Intel(R) Xeon(R) Platinum 8370C CPU @ 2.80GHz' \
                'Model name:                      Intel(R) Atom(TM)' > "$LSCPU"
  run
  field "the first of several model names, with its own punctuation" 'd["run"]["cpu_model"]' \
        "Intel(R) Xeon(R) Platinum 8370C CPU @ 2.80GHz"

  fixture
  printf '%s\n' 'Model name: AMD: EPYC' > "$LSCPU"
  run
  field "a name read past the first colon only" 'd["run"]["cpu_model"]' "AMD: EPYC"

  fixture
  rm "$LSCPU"
  run
  refuses "an lscpu which fails" 'lscpu failed with exit status 1'

  fixture
  printf '%s\n' 'Architecture: x86_64' 'BIOS Model name: Virtual CPU' > "$LSCPU"
  run
  refuses "an lscpu with no Model name line" 'lscpu prints no Model name'

  fixture
  printf '%s\n' 'Model name:   ' > "$LSCPU"
  run
  refuses "an lscpu with a Model name and no name" 'lscpu prints a Model name with no name'

  fixture
  run PATH=/nonexistent
  refuses "a missing lscpu" 'could not be run'
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
  refuses "a tool which already has a package" 'already has a package for .*/usr/bin/nm'

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
  for control in record_controls dpkg_controls cpu_controls record_shape_controls variable_controls; do
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
mutant 'any line read as an owner'      'if line.endswith(suffix)' 'if True'
mutant "the caller's locale kept"       ', env={**os.environ, "LC_ALL": "C"}' ''
mutant 'a diversion accepted'           'if diversions:' 'if False:'
mutant 'a local diversion accepted'     '("diversion by ", "local diversion ")' '("diversion by ",)'
mutant "a package's diversion accepted" '("diversion by ", "local diversion ")' '("local diversion ",)'
mutant 'split at the first colon'       'line[:-len(suffix)].split(", ")]' 'line.split(":")[0].split(", ")]'
mutant 'the owners not split'           'line[:-len(suffix)].split(", ")]' '[line[:-len(suffix)]]]'
mutant 'the first of several owners'    'if len(owners) != 1:' 'if not owners:'
mutant 'an empty version accepted'      'if not version:' 'if False:'
mutant 'the package named by the path'  'name = owner_of(os.path.realpath(path))' \
                                        'name = owner_of(os.path.realpath(path)) and os.path.basename(path)'
mutant "the symlink's owner named"      'owner_of(os.path.realpath(path))' 'owner_of(path)'
mutant 'one link followed'              'owner_of(os.path.realpath(path))' \
                                        'owner_of(os.path.join(os.path.dirname(path), os.readlink(path))'\
'                                         if os.path.islink(path) else path)'
mutant 'any line read as the model name' 'if key.strip() == "Model name":' 'if key.strip().endswith("Model name"):'
mutant 'the first model name skipped'   'return value.strip()' 'continue'
mutant 'the name cut at a second colon' 'key, _, value = line.partition(":")' 'key, _, value = line.rpartition(":")'
mutant 'an empty model name accepted'   'if not value.strip():' 'if False:'
mutant 'no model name recorded'         '"cpu_model": cpu_model(), ' ''
mutant 'an absent model name accepted'  'sys.exit("lscpu prints no Model name")' 'return ""'
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
mutant 'no tool read'                   'tools = [tool for' 'tools = [tool for tool in [] for'

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
