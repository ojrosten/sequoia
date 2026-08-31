#!/usr/bin/env python3
"""EXPERIMENT: one named module per header - the shape production wants.

    python3 scripts/experiments/generate_per_header_modules.py <outdir> [roots...]

The monolithic variant (generate_tests_module.py) puts 220 test headers in one
module, so every helper namespace is in scope at once and names like `sets` and
`wait` go ambiguous. Here each header is its own module and each test imports
only what it included, so two unrelated tests can use the same helper name - the
situation that already obtains with headers.

Emits, per header H: an interface `<outdir>/<name>.cppm`, and where H has a
matching .cpp, an implementation unit `<outdir>/<name>.impl.cpp`. Also emits
`order.txt`, the interfaces in dependency order, and `deps.txt`, "name: deps",
so a driver can build BMIs in parallel layers.
"""
import os, re, sys, collections

ROOTS = sys.argv[2:] or ["Tests", "TestCommon"]   # e.g. Source/sequoia, or Tests TestCommon
INC_Q  = re.compile(r'^\s*#include\s+"[^"]+"\s*$', re.M)
INC_A  = re.compile(r'^\s*#include\s+<[^>]+>\s*$', re.M)
PRAGMA = re.compile(r'^\s*#pragma once\s*$', re.M)
LIC    = re.compile(r'^/{60,}\n(?:.*\n)*?^/{60,}\n', re.M)
FWD    = re.compile(r'^\s*class test_runner;\s*$', re.M)   # sequoia owns it; the import declares it


def modname(path):
    return "seq.m." + os.path.splitext(path)[0].replace("/", "_").replace("-", "_")


def main():
    outdir = sys.argv[1]
    os.makedirs(outdir, exist_ok=True)

    hdrs = sorted(os.path.join(d, f) for r in ROOTS for d, _, fs in os.walk(r)
                  for f in fs if f.endswith(".hpp"))
    hset = set(hdrs)
    byname = collections.defaultdict(list)
    for h in hdrs:
        byname[os.path.basename(h)].append(h)

    dep = collections.defaultdict(set)
    for h in hdrs:
        for inc in re.findall(r'#include "([^"]+)"', open(h).read()):
            cand = os.path.normpath(os.path.join(os.path.dirname(h), inc))
            if cand in hset:
                dep[h].add(cand); continue
            for r in ROOTS:
                c2 = os.path.normpath(os.path.join(r, inc))
                if c2 in hset:
                    dep[h].add(c2); break
            else:
                base = os.path.basename(inc)
                if len(byname.get(base, [])) == 1 and byname[base][0] != h:
                    dep[h].add(byname[base][0])

    order, done, active = [], set(), set()
    def visit(n):
        if n in done or n in active: return
        active.add(n)
        for d in sorted(dep[n]): visit(d)
        active.discard(n); done.add(n); order.append(n)
    for h in hdrs: visit(h)

    def strip(s):
        s = LIC.sub("", s, count=1)
        s = PRAGMA.sub("", s); s = INC_Q.sub("", s); s = INC_A.sub("", s)
        return FWD.sub("", s).strip()

    for h in order:
        name = modname(h)
        # `export import`, not `import`: including a header gives you everything it
        # included, and importing a module does not. Re-exporting along the real
        # include edges reproduces the semantics the tests were written against.
        imports = "".join(f"export import {modname(d)};\n" for d in sorted(dep[h]))
        with open(os.path.join(outdir, name + ".cppm"), "w") as f:
            f.write(f"// generated from {h}\nmodule;\n\nexport module {name};\n\n"
                    f"import std;\nimport std.compat;\n{imports}\n"
                    f"export {{\n{strip(open(h).read())}\n}}\n")
        cpp = h[:-4] + ".cpp"
        if os.path.exists(cpp):
            # A .cpp may include headers its own .hpp does not; those become imports.
            body = open(cpp).read()
            extra = set()
            for inc in re.findall(r'#include "([^"]+)"', body):
                cand = os.path.normpath(os.path.join(os.path.dirname(cpp), inc))
                if cand not in hset:
                    for r in ROOTS:
                        c2 = os.path.normpath(os.path.join(r, inc))
                        if c2 in hset: cand = c2; break
                    else:
                        bn = os.path.basename(inc)
                        cand = byname[bn][0] if len(byname.get(bn, [])) == 1 else None
                if cand and cand in hset and cand != h:
                    extra.add(modname(cand))
            imps = "".join(f"import {m};\n" for m in sorted(extra))
            with open(os.path.join(outdir, name + ".impl.cpp"), "w") as f:
                f.write(f"// generated from {cpp}\nmodule {name};\n\n{imps}\n{strip(body)}\n")

    with open(os.path.join(outdir, "order.txt"), "w") as f:
        f.write("\n".join(modname(h) for h in order) + "\n")
    with open(os.path.join(outdir, "deps.txt"), "w") as f:
        for h in order:
            f.write(f"{modname(h)}: {' '.join(sorted(modname(d) for d in dep[h]))}\n")
    print(f"  {len(order)} interfaces, "
          f"{sum(1 for h in order if os.path.exists(h[:-4]+'.cpp'))} implementation units",
          file=sys.stderr)


if __name__ == "__main__":
    main()
