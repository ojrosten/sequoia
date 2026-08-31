#!/bin/bash
# EXPERIMENT: build sequoia as named modules, and time it against the header build.
#
#   scripts/experiments/build_modules.sh [jobs] [workdir]
#
# There are no .cppm files in this repository, which is the first thing anyone
# looking for "the modules" needs to know. They are *generated* from the headers
# by generate_component_modules.py, one module per directory, into <workdir>.
# Nothing in CMake knows about any of this: the ordinary build is still a header
# build, and this script is a parallel universe beside it.
#
# What it prints is the pair of numbers the experiment exists for: the same
# translation units compiled the ordinary way, and compiled as module
# implementation units with no #include of their own.
set -u

J=${1:-1}
W=${2:-${TMPDIR:-/tmp}/sequoia-modules}
CXX=${CXX:-/opt/homebrew/opt/llvm/bin/clang++}
STD_CPPM=${STD_CPPM:-/opt/homebrew/opt/llvm/share/libc++/v1/std.cppm}
COMPAT_CPPM=${COMPAT_CPPM:-/opt/homebrew/opt/llvm/share/libc++/v1/std.compat.cppm}
INC=(-I Source -I Tests -I TestCommon)

[[ -d Source/sequoia ]] || { echo "run from the repository root" >&2; exit 1; }
rm -rf "$W"; mkdir -p "$W"/{gen,bmi,obj}

now() { python3 -c 'import time;print(time.time())'; }

echo "generating module interfaces..."
python3 scripts/experiments/generate_component_modules.py "$W/gen"

# Components in dependency layers, so a layer can be built in parallel.
python3 - "$W" <<'PY'
import collections, sys
W = sys.argv[1]
dep = {}
for line in open(f"{W}/gen/deps.txt"):
    name, _, rest = line.partition(":")
    dep[name.strip()] = rest.split()
depth = {}
def d(n):
    if n not in depth: depth[n] = 1 + max([d(x) for x in dep[n]], default=-1)
    return depth[n]
for n in dep: d(n)
layers = collections.defaultdict(list)
for n, l in depth.items(): layers[l].append(n)
with open(f"{W}/gen/layers.txt", "w") as f:
    for l in sorted(layers): f.write(" ".join(sorted(layers[l])) + "\n")
PY

find Source/sequoia Tests -name '*.cpp' | sort > "$W/units.txt"

# ---- the header build, for comparison -------------------------------------
cat > "$W/cc-header" <<EOF
#!/bin/bash
exec "$CXX" -std=c++26 ${INC[@]} -c "\$1" -o "$W/obj/\$(echo "\$1" | tr / _).o"
EOF
chmod +x "$W/cc-header"
t0=$(now); xargs -P "$J" -n1 "$W/cc-header" < "$W/units.txt" 2>/dev/null; t1=$(now)

# ---- the module build ------------------------------------------------------
rm -rf "$W/obj"; mkdir -p "$W/obj"
t2=$(now)
"$CXX" -std=c++26 -Wno-reserved-module-identifier --precompile "$STD_CPPM" -o "$W/bmi/std.pcm"
"$CXX" -std=c++26 -Wno-reserved-module-identifier -fmodule-file=std="$W/bmi/std.pcm" \
       --precompile "$COMPAT_CPPM" -o "$W/bmi/std.compat.pcm"

cat > "$W/mapfile" <<EOF
#!/bin/bash
printf -- '-fmodule-file=std=$W/bmi/std.pcm -fmodule-file=std.compat=$W/bmi/std.compat.pcm '
for p in "$W"/bmi/seq.c.*.pcm; do [ -e "\$p" ] && printf -- '-fmodule-file=%s=%s ' "\$(basename "\$p" .pcm)" "\$p"; done
EOF
chmod +x "$W/mapfile"

cat > "$W/pc-module" <<EOF
#!/bin/bash
read -ra MF <<< "\$("$W/mapfile")"
exec "$CXX" -std=c++26 ${INC[@]} "\${MF[@]}" --precompile "$W/gen/\$1.cppm" -o "$W/bmi/\$1.pcm"
EOF
cat > "$W/cc-module" <<EOF
#!/bin/bash
read -ra MF <<< "\$("$W/mapfile")"
exec "$CXX" -std=c++26 ${INC[@]} "\${MF[@]}" -c "\$1" -o "$W/obj/\$(basename "\$1").o"
EOF
chmod +x "$W/pc-module" "$W/cc-module"

while read -r layer; do printf '%s\n' $layer | xargs -P "$J" -n1 "$W/pc-module" 2>/dev/null; done < "$W/gen/layers.txt"
t3=$(now)
ls "$W"/gen/*__*.cpp | xargs -P "$J" -n1 "$W/cc-module" 2>/dev/null
t4=$(now)

python3 - "$J" "$W" "$t0" "$t1" "$t2" "$t3" "$t4" <<'PY'
import os, sys
J, W, t0, t1, t2, t3, t4 = sys.argv[1], sys.argv[2], *map(float, sys.argv[3:])
bmis = len([f for f in os.listdir(f"{W}/bmi") if f.startswith("seq.c.")])
objs = len(os.listdir(f"{W}/obj"))
hdr, mods = t1 - t0, t4 - t2
print(f"\n  -j{J}")
print(f"    headers  {hdr:6.1f}s")
print(f"    modules  {mods:6.1f}s   ({bmis} BMIs in {t3-t2:.1f}s, then {objs} units)")
print(f"    speedup  {hdr/mods:6.2f}x")
PY
