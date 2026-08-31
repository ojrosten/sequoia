#!/usr/bin/env python3
"""EXPERIMENT: one named module per directory - the granularity that actually works.

    python3 scripts/experiments/generate_component_modules.py <outdir> [roots...]

Three granularities were tried. A single module for everything puts every helper
namespace in scope at once, so `sets` and `wait` go ambiguous. One module per
*header* breaks on two things a header boundary is not: macros, which no module
exports, and classes whose declaration and definition are split across headers.
A directory is a component, and components are what modules are for.

Emits per component an interface and, where the directory has .cpp files, one
implementation unit each. `order.txt` lists components in dependency order;
`deps.txt` gives "name: deps" so a driver can build in parallel layers.
"""
import os, re, sys, collections

INC_Q  = re.compile(r'^\s*#include\s+"[^"]+"\s*$', re.M)
INC_A  = re.compile(r'^\s*#include\s+<[^>]+>\s*$', re.M)
PRAGMA = re.compile(r'^\s*#pragma once\s*$', re.M)
LIC    = re.compile(r'^/{60,}\n(?:.*\n)*?^/{60,}\n', re.M)
# A forward declaration of an entity another module owns declares a *different*
# entity attached to this module, which is an error. Within a component it is
# still needed, so only cross-component ones are dropped.
FWD_ANY = re.compile(r'^([ \t]*)(?:template\s*<[^;{}]*>\s*)?(?:class|struct)\s+(\w+)\s*;[ \t]*$', re.M)
DEFN    = re.compile(r'^[ \t]*(?:template\s*<[^;{}]*>[ \t]*\n?)?[ \t]*(?:class|struct)\s+(\w+)\b(?![;])', re.M)
# Headers whose point is a #define cannot be modules: nothing exports a macro.
# They are included in the global module fragment of anything that needs them.
# (path in the repo, how it is spelled on the include path)
MACRO_HDRS = [("Source/sequoia/PlatformSpecific/Macros.hpp", "sequoia/PlatformSpecific/Macros.hpp"),
              ("Tests/Maths/Graph/Static/MSVC_Workarounds.hpp", "Maths/Graph/Static/MSVC_Workarounds.hpp"),
              ("Source/sequoia/TestFramework/Macros.hpp", "sequoia/TestFramework/Macros.hpp")]
MACRO_PATHS = {p for p, _ in MACRO_HDRS}

# Core/Object and Maths/Graph are mutually dependent; they are one component.
MERGE = {"Source/sequoia/Core/Object": "Source/sequoia/Maths/Graph"}


def component(path):
    d = os.path.dirname(path)
    return MERGE.get(d, d)


def modname(c):
    return "seq.c." + c.replace("/", "_").replace("-", "_").replace(".", "_")


def strip(s, foreign=frozenset()):
    s = LIC.sub("", s, count=1)
    s = PRAGMA.sub("", s)
    s = INC_Q.sub("", s)
    s = INC_A.sub("", s)
    s = FWD_ANY.sub(lambda m: "" if m.group(2) in foreign else m.group(0), s)
    return s.strip()


def main():
    outdir, roots = sys.argv[1], (sys.argv[2:] or ["Source/sequoia", "Tests", "TestCommon"])
    os.makedirs(outdir, exist_ok=True)
    hdrs = sorted(os.path.join(d, f) for r in roots for d, _, fs in os.walk(r)
                  for f in fs if f.endswith(".hpp"))
    hdrs = [h for h in hdrs if h not in MACRO_PATHS]
    hset = set(hdrs)
    byname = collections.defaultdict(list)
    for h in hdrs:
        byname[os.path.basename(h)].append(h)

    def resolve(src, inc):
        c = os.path.normpath(os.path.join(os.path.dirname(src), inc))
        if c in hset: return c
        for r in roots:
            c2 = os.path.normpath(os.path.join(r, inc))
            if c2 in hset: return c2
        b = os.path.basename(inc)
        return byname[b][0] if len(byname.get(b, [])) == 1 else None

    hdep = collections.defaultdict(set)                 # header -> headers
    for h in hdrs:
        for inc in re.findall(r'#include "([^"]+)"', open(h).read()):
            t = resolve(h, inc)
            if t and t != h: hdep[h].add(t)

    comps = collections.defaultdict(list)
    for h in hdrs: comps[component(h)].append(h)
    cdep = collections.defaultdict(set)
    for h in hdrs:
        for t in hdep[h]:
            if component(t) != component(h): cdep[component(h)].add(component(t))

    def toposort(nodes, edges):
        out, done, active = [], set(), set()
        def visit(n):
            if n in done or n in active: return
            active.add(n)
            for d in sorted(edges.get(n, ())): visit(d)
            active.discard(n); done.add(n); out.append(n)
        for n in sorted(nodes): visit(n)
        return out

    corder = toposort(comps, cdep)
    defined = {c: {m.group(1) for h in comps[c] for m in DEFN.finditer(open(h).read())} for c in comps}
    for c in corder:
        foreign = set().union(*(defined[o] for o in comps if o != c)) - defined[c]
        inner = toposort(comps[c], {h: {t for t in hdep[h] if component(t) == c} for h in comps[c]})
        body = "\n".join(f"// ==== {h} ====\n{strip(open(h).read(), foreign)}\n" for h in inner)
        imports = "".join(f"export import {modname(d)};\n" for d in sorted(cdep[c]))
        with open(os.path.join(outdir, modname(c) + ".cppm"), "w") as f:
            gmf = "".join(f'#include "{spelling}"\n' for _, spelling in MACRO_HDRS)
            f.write(f"// component {c}\nmodule;\n\n{gmf}\n"
                    f"export module {modname(c)};\n\nimport std;\nimport std.compat;\n{imports}\n"
                    f"export {{\n{body}\n}}\n")

    nimpl = 0
    for c in corder:
        foreign_impl = set().union(*(defined[o] for o in comps if o != c)) - defined[c]
        srcdir = c if not any(v == c for v in MERGE.values()) else c
        for d in {os.path.dirname(h) for h in comps[c]}:
            for f in sorted(os.listdir(d)):
                if not f.endswith(".cpp"): continue
                p = os.path.join(d, f)
                s = open(p).read()
                extra = set()
                for inc in re.findall(r'#include "([^"]+)"', s):
                    t = resolve(p, inc)
                    if t and component(t) != c: extra.add(modname(component(t)))
                imps = "".join(f"import {m};\n" for m in sorted(extra))
                out = os.path.join(outdir, modname(c) + "__" + f.replace("/", "_"))
                gmf = "".join(f'#include "{spelling}"\n' for _, spelling in MACRO_HDRS)
                if "abi::" in s: gmf += "#include <cxxabi.h>\n"
                open(out, "w").write(f"// from {p}\nmodule;\n\n{gmf}\nmodule {modname(c)};\n\n"
                                     f"{imps}\n{strip(s, foreign_impl)}\n")
                nimpl += 1

    open(os.path.join(outdir, "order.txt"), "w").write("\n".join(modname(c) for c in corder) + "\n")
    with open(os.path.join(outdir, "deps.txt"), "w") as f:
        for c in corder:
            f.write(f"{modname(c)}: {' '.join(sorted(modname(d) for d in cdep[c]))}\n")
    print(f"  {len(corder)} components, {nimpl} implementation units", file=sys.stderr)


if __name__ == "__main__":
    main()
