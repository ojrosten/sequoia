#!/usr/bin/env python3
"""Convert sequoia's headers into C++20 module interface units, in place.

    python3 scripts/modules/migrate_to_modules.py [--apply] [component ...]

One module per *component* (directory), one interface partition per former
header.  That pairing is the point: the component is the granularity a
consumer imports, established by the earlier experiments as the only one that
works, while the partition keeps the file structure a reader already knows.

A header whose stem matches its directory - Graph.hpp, Algorithms.hpp - is an
umbrella, so it becomes the primary interface unit rather than a partition.
Components without one get a primary unit generated for them.

Macros do not travel through modules, so headers whose point is a #define stay
headers and are included in the global module fragment of whatever needs them.
"""
import argparse, os, re, sys
from collections import defaultdict

ROOT      = "Source/sequoia"
MACRO_HDR = {"Source/sequoia/PlatformSpecific/Macros.hpp",
             "Source/sequoia/TestFramework/Macros.hpp"}

# A directory is a component, except where the directory is not actually one
# layer.  Core/Object holds the handler machinery that Maths/Graph is built on
# *and* Suite, which is built on Maths/Graph, so one module per directory is a
# cycle.  The header build hid this; the module graph cannot.  Splitting Suite
# out needs no file moves and changes no design - the modules simply state the
# layering that was always there.
SPLIT = {"Source/sequoia/Core/Object/Suite.hpp": "sequoia.core.object.suite"}

RE_PRAGMA  = re.compile(r'^[ \t]*#pragma once[ \t]*\n', re.M)
RE_INC_SYS = re.compile(r'^[ \t]*#include[ \t]*<([^>]+)>[ \t]*\n', re.M)
RE_INC_LOC = re.compile(r'^[ \t]*#include[ \t]*"([^"]+)"[ \t]*\n', re.M)
RE_LICENSE = re.compile(r'\A(/{60,}\n(?:.*?\n)*?/{60,}\n)', re.M)
# Namespaces opened by a macro are invisible to a textual `export namespace`
# rewrite, and the failure surfaces nowhere near the cause: nothing in
# Bitmask.hpp was exported, and what broke was an explicit specialization in
# Summary, several modules away.  Name them here rather than trying to guess.
NS_MACROS = ["NAMESPACE_SEQUOIA_AS_BITMASK"]
RE_NS_OPEN = re.compile(
    r'^(?:namespace[ \t]+[A-Za-z_][\w:]*|(?:' + "|".join(NS_MACROS) + r'))[ \t]*(?:\n|\{)', re.M)
# A quoted include naming something outside the repository is a platform header,
# not a sequoia one - Helpers.cpp's "Windows.h".  It has to reach the global
# module fragment carrying its own #ifdef, or the guard is lost and the include
# becomes unconditional.
RE_GUARDED = re.compile(
    r'^[ \t]*(#if[^\n]*)\n[ \t]*#include[ \t]*"([^"]+)"[ \t]*\n[ \t]*#endif[^\n]*\n', re.M)


def extract_foreign(src, is_sequoia):
    """Pull non-sequoia quoted includes into GMF text, keeping any guard."""
    gmf = []

    def guarded(m):
        if is_sequoia(m.group(2)):
            return m.group(0)
        gmf.append(f'{m.group(1)}\n  #include "{m.group(2)}"\n#endif')
        return ""
    src = RE_GUARDED.sub(guarded, src)

    def bare(m):
        if is_sequoia(m.group(1)):
            return m.group(0)
        gmf.append(f'#include "{m.group(1)}"')
        return ""
    src = RE_INC_LOC.sub(bare, src)
    return src, gmf


def snake(name):
    """CamelCase directory -> snake_case module component."""
    return re.sub(r'(?<!^)(?=[A-Z])', '_', name).lower()


def module_name(component):
    """Source/sequoia/Core/Meta -> sequoia.core.meta"""
    rel = os.path.relpath(component, os.path.dirname(ROOT))
    return ".".join(snake(p) for p in rel.split(os.sep))


def headers_of(component):
    return sorted(f for f in os.listdir(component)
                  if f.endswith(".hpp") and os.path.join(component, f) not in MACRO_HDR)


def is_umbrella(component, header):
    return os.path.splitext(header)[0] == os.path.basename(component)


def component_of(include):
    """'sequoia/Core/Meta/TypeTraits.hpp' -> ('Source/sequoia/Core/Meta', 'TypeTraits')"""
    path = os.path.join("Source", include)
    return os.path.dirname(path), os.path.splitext(os.path.basename(path))[0]


def own_module(path):
    """The module a header defines, if it is not a partition of its component."""
    return SPLIT.get(path)


# Including a header gave you everything *it* included; importing a module does
# not.  Both halves of that leak have to be repaid explicitly, or the migration
# changes what every consumer can see:
#   - standard headers, which are unioned transitively into the global module
#     fragment of whatever used to get them for free;
#   - sequoia entities, which is why a cross-module import is re-exported along
#     what used to be a real include edge.
# Narrowing either is a separate, schedulable improvement - doing it here would
# smuggle an interface change into a translation.
STD_CLOSURE = {}
# Macros leak transitively exactly as standard headers do, and for the same
# reason: a header that reached SEQUOIA_NO_UNIQUE_ADDRESS through a sibling gets
# nothing from importing that sibling as a module.
MACRO_CLOSURE = {}
# The sequoia headers a file reached transitively.  A partition that relied on a
# sibling arriving through a third one sees nothing if only direct includes
# become imports - the symptom is a two-phase-lookup failure a long way from the
# missing edge.
LOCAL_CLOSURE = {}
# A forward declaration of an entity another module owns does not declare that
# entity - it declares a *different* one, attached to this module, and the
# compiler says so.  Within a component it is still legitimate, since every
# partition attaches to the same module, so only cross-module ones are dropped.
OWNER = {}
RE_FWD = re.compile(
    r'^([ \t]*)(?:template[ \t]*<[^;{}]*>[ \t]*\n?[ \t]*)?(?:class|struct)[ \t]+(\w+)[ \t]*;[ \t]*\n',
    re.M)
RE_DEFN = re.compile(
    r'^[ \t]*(?:template[ \t]*<[^;{}]*>[ \t]*\n?[ \t]*)?(?:class|struct)[ \t]+(\w+)\b(?![ \t]*;)',
    re.M)


def build_owner_map():
    for d, _, fs in os.walk(ROOT):
        for f in fs:
            if not f.endswith(".hpp"):
                continue
            p = os.path.join(d, f)
            mod = own_module(p) or module_name(component(d))
            for name in RE_DEFN.findall(open(p).read()):
                OWNER.setdefault(name, mod)


def component(d):
    return d


def build_std_closure():
    """Transitive union of <> includes over the former include graph."""
    direct, local, macros = {}, {}, {}
    for d, _, fs in os.walk(ROOT):
        for f in fs:
            if not f.endswith((".hpp", ".cpp")):
                continue
            p = os.path.join(d, f)
            src = open(p).read()
            direct[p] = set(RE_INC_SYS.findall(src))
            loc = [os.path.join("Source", i) for i in RE_INC_LOC.findall(src)]
            local[p] = loc
            macros[p] = {i for i in loc if i in MACRO_HDR}

    seen = {}
    def close(p):
        if p in seen:
            return seen[p]
        seen[p] = (set(direct.get(p, ())), set(macros.get(p, ())), set())  # cycles
        std, mac = set(direct.get(p, ())), set(macros.get(p, ()))
        loc = set()
        for dep in local.get(p, ()):
            if dep in direct and dep not in MACRO_HDR:
                loc.add(dep)
                s2, m2, l2 = close(dep)
                std |= s2
                mac |= m2
                loc |= l2
        seen[p] = (std, mac, loc)
        return seen[p]

    for p in direct:
        STD_CLOSURE[p], MACRO_CLOSURE[p], LOCAL_CLOSURE[p] = close(p)


def convert(path, component, comps):
    src = open(path).read()
    header = os.path.basename(path)
    stem = os.path.splitext(header)[0]
    split = own_module(path)
    primary = bool(split) or is_umbrella(component, header)
    mod = split or module_name(component)

    licence = ""
    m = RE_LICENSE.match(src)
    if m:
        licence, src = m.group(1), src[m.end():]

    src = RE_PRAGMA.sub("", src)
    # \file directives naming the header the module replaced would now be lies.
    src = re.sub(r'\\file[ \t]+' + re.escape(stem) + r'\.hpp', r'\\file', src)

    system = list(STD_CLOSURE.get(path, ()))
    gmf_macro = sorted(os.path.relpath(h, "Source") for h in MACRO_CLOSURE.get(path, ()))
    src = RE_INC_SYS.sub("", src)

    src, foreign_gmf = extract_foreign(
        src, lambda i: os.path.exists(os.path.join("Source", i)) or i.startswith("sequoia/"))

    imports, exports, gmf_headers = set(), set(), []
    def take_loc(m):
        inc = m.group(1)
        target = os.path.join("Source", inc)
        if target in MACRO_HDR:
            pass                      # supplied from MACRO_CLOSURE below
        else:
            dep_comp, dep_stem = component_of(inc)
            dep_split = own_module(os.path.join("Source", inc))
            if dep_split:
                exports.add(dep_split)
            elif dep_comp == component and not split:
                # A partition of this module; the umbrella needs no self-import.
                if not is_umbrella(component, dep_stem + ".hpp"):
                    imports.add(":" + dep_stem)
            else:
                exports.add(module_name(dep_comp))
        return ""
    src = RE_INC_LOC.sub(take_loc, src)

    for dep in sorted(LOCAL_CLOSURE.get(path, ())):
        dep_comp = os.path.dirname(dep)
        dep_stem = os.path.splitext(os.path.basename(dep))[0]
        dep_split = own_module(dep)
        if dep_split:
            if dep_split != mod:
                exports.add(dep_split)
        elif dep_comp == component and not split:
            if not is_umbrella(component, dep_stem + ".hpp"):
                imports.add(":" + dep_stem)
        elif module_name(dep_comp) != mod:
            exports.add(module_name(dep_comp))

    # Everything a header declared was visible to its includers; exporting each
    # namespace preserves exactly that, rather than quietly narrowing the
    # interface as part of a migration.
    # Dropping a cross-module forward declaration removes the only declaration
    # the file had, so the owning module has to be imported in its place - the
    # entity stays visible, which is what the declaration was there for.
    def drop_fwd(m):
        owner = OWNER.get(m.group(2), mod)
        if owner == mod:
            return m.group(0)
        exports.add(owner)
        return ""

    src = RE_FWD.sub(drop_fwd, src)

    src = RE_NS_OPEN.sub(lambda m: "export " + m.group(0), src)

    out = [licence.rstrip("\n"), ""]
    if system or gmf_macro or foreign_gmf:
        out += ["module;", ""]
        out += foreign_gmf
        out += [f'#include "{h}"' for h in gmf_macro]
        if gmf_macro and system:
            out.append("")
        out += [f"#include <{h}>" for h in sorted(set(system))]
        out.append("")
    out.append(f"export module {mod};" if primary else f"export module {mod}:{stem};")
    out.append("")
    if imports or exports:
        out += [f"import {i};" for i in sorted(imports)]
        out += [f"export import {i};" for i in sorted(exports)]
        out.append("")
    out.append(src.strip("\n"))
    out.append("")
    return "\n".join(out), primary


def convert_impl(path, component):
    """A .cpp defining entities the interface declares must attach to the module."""
    src = open(path).read()
    mod = module_name(component)
    for hdr, name in SPLIT.items():
        if os.path.dirname(hdr) == component and \
           os.path.splitext(os.path.basename(hdr))[0] == \
           os.path.splitext(os.path.basename(path))[0]:
            mod = name

    licence = ""
    m = RE_LICENSE.match(src)
    if m:
        licence, src = m.group(1), src[m.end():]

    system = list(STD_CLOSURE.get(path, ()))
    gmf_macro = sorted(os.path.relpath(h, "Source") for h in MACRO_CLOSURE.get(path, ()))
    src = RE_INC_SYS.sub("", src)

    src, foreign_gmf = extract_foreign(
        src, lambda i: os.path.exists(os.path.join("Source", i)) or i.startswith("sequoia/"))

    imports, gmf_headers = set(), []
    def take_loc(m):
        inc = m.group(1)
        target = os.path.join("Source", inc)
        if target in MACRO_HDR:
            pass                      # supplied from MACRO_CLOSURE below
        else:
            dep_comp, _ = component_of(inc)
            # `module M;` implicitly imports M's interface, partitions included.
            if dep_comp != component:
                imports.add(module_name(dep_comp))
        return ""
    src = RE_INC_LOC.sub(take_loc, src)

    out = [licence.rstrip("\n"), ""]
    if system or gmf_macro or foreign_gmf:
        out += ["module;", ""]
        out += foreign_gmf
        out += [f'#include "{h}"' for h in gmf_macro]
        if gmf_macro and system:
            out.append("")
        out += [f"#include <{h}>" for h in sorted(set(system))]
        out.append("")
    out.append(f"module {mod};")
    out.append("")
    if imports:
        out += [f"import {i};" for i in sorted(imports)]
        out.append("")
    out.append(src.strip("\n"))
    out.append("")
    return "\n".join(out)


def make_primary(component, parts):
    mod = module_name(component)
    lines = ["////////////////////////////////////////////////////////////////////",
             "//                Copyright Oliver J. Rosten 2026.                //",
             "// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //",
             "//    (See accompanying file LICENSE.md or copy at                //",
             "//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //",
             "////////////////////////////////////////////////////////////////////",
             "",
             "/** \\file */",
             "",
             f"export module {mod};",
             ""]
    lines += [f"export import :{p};" for p in parts]
    lines.append("")
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("components", nargs="*")
    args = ap.parse_args()

    build_std_closure()
    build_owner_map()
    comps = sorted({d for d, _, fs in os.walk(ROOT) if any(f.endswith(".hpp") for f in fs)})
    targets = [c for c in comps if not args.components
               or any(c.endswith(t) for t in args.components)]

    for comp in targets:
        hdrs = headers_of(comp)
        if not hdrs:
            continue
        parts, has_primary = [], False
        for h in hdrs:
            text, primary = convert(os.path.join(comp, h), comp, comps)
            dest = os.path.join(comp, os.path.splitext(h)[0] + ".cppm")
            has_primary |= primary and not own_module(os.path.join(comp, h))
            if not primary:
                parts.append(os.path.splitext(h)[0])
            if args.apply:
                open(dest, "w").write(text)
                os.remove(os.path.join(comp, h))
            else:
                print(f"  {os.path.join(comp, h)} -> {dest}"
                      f"{'  [PRIMARY]' if primary else ''}")
        for c in sorted(f for f in os.listdir(comp) if f.endswith(".cpp")):
            cpath = os.path.join(comp, c)
            if args.apply:
                # Compute before opening: `open(p,"w")` truncates, and source and
                # destination are the same file here, unlike the header path.
                text = convert_impl(cpath, comp)
                open(cpath, "w").write(text)
            else:
                print(f"  {cpath} -> module implementation unit")

        if not has_primary:
            dest = os.path.join(comp, os.path.basename(comp) + ".cppm")
            if args.apply:
                open(dest, "w").write(make_primary(comp, parts))
            else:
                print(f"  (new) {dest}  [PRIMARY over {len(parts)} partitions]")
        elif parts:
            # The umbrella is the primary; make sure it re-exports every sibling.
            dest = os.path.join(comp, os.path.basename(comp) + ".cppm")
            if args.apply:
                text = open(dest).read()
                add = "\n".join(f"export import :{p};" for p in parts)
                text = text.replace(f"export module {module_name(comp)};",
                                    f"export module {module_name(comp)};\n\n{add}", 1)
                open(dest, "w").write(text)
        print(f"{module_name(comp):40s} {len(hdrs)} headers")


if __name__ == "__main__":
    main()
