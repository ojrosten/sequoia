#!/usr/bin/env python3
"""Point the test tree at sequoia's modules instead of its headers.

    python3 scripts/modules/migrate_tests_to_imports.py [--apply]

The library is modules now, so `#include "sequoia/..."` names a file that no
longer exists.  Each such include becomes an import of the module owning that
component.  Test-local headers are left alone: turning the tests themselves
into modules is a separate job, and a larger one - a single module for 220 test
headers puts every helper namespace in scope at once, which is where the
earlier experiment broke.
"""
import argparse, os, re, sys

ROOTS = ["Tests", "TestCommon", "TestAll", "TestChamber",
         "TestFrameworkDiagnostics", "TestFrameworkHarness"]
MACRO_HDR = {"Source/sequoia/PlatformSpecific/Macros.hpp",
             "Source/sequoia/TestFramework/Macros.hpp"}
SPLIT = {"Source/sequoia/Core/Object/Suite.hpp": "sequoia.core.object.suite"}

RE_INC = re.compile(r'^[ \t]*#include[ \t]*"(sequoia/[^"]+)"[ \t]*\n', re.M)
RE_INC_SYS = re.compile(r'^[ \t]*#include[ \t]*<([^>]+)>[ \t]*\n', re.M)
RE_INC_LOC = re.compile(r'^[ \t]*#include[ \t]*"([^"]+)"[ \t]*\n', re.M)

# A test that included a sequoia header also got everything that header
# included.  Importing the module gives it none of that, so the standard
# headers it was silently relying on have to be named explicitly.  The closure
# is computed from the pre-migration headers, which only git still has.
STD_CLOSURE = {}
# Macros leak the same way, and STATIC_CHECK is the one every test relies on.
MACRO_CLOSURE = {}
# A test that forward-declares a library entity now declares a *different* one,
# in the global module.  The import already makes the real one visible, so the
# declaration is simply removed.
OWNER = {}
RE_FWD = re.compile(
    r'^([ \t]*)(?:template[ \t]*<[^;{}]*>[ \t]*\n?[ \t]*)?(?:class|struct)[ \t]+(\w+)[ \t]*;[ \t]*\n',
    re.M)
RE_DEFN = re.compile(
    r'^[ \t]*(?:template[ \t]*<[^;{}]*>[ \t]*\n?[ \t]*)?(?:class|struct)[ \t]+(\w+)\b(?![ \t]*;)',
    re.M)


def build_std_closure(rev):
    import subprocess
    files = subprocess.run(["git", "ls-tree", "-r", "--name-only", rev, "Source/sequoia"],
                           capture_output=True, text=True).stdout.split()
    files = [f for f in files if f.endswith(".hpp")]
    blobs = {}
    for f in files:
        blobs[f] = subprocess.run(["git", "show", f"{rev}:{f}"],
                                  capture_output=True, text=True).stdout
    direct = {f: set(RE_INC_SYS.findall(b)) for f, b in blobs.items()}
    local = {f: [os.path.join("Source", i) for i in RE_INC_LOC.findall(b)]
             for f, b in blobs.items()}
    macro = {f: {os.path.relpath(i, "Source") for i in local[f] if i in MACRO_HDR}
             for f in files}
    seen = {}

    def close(f):
        if f in seen:
            return seen[f]
        seen[f] = (set(direct.get(f, ())), set(macro.get(f, ())))   # guards cycles
        std, mac = set(direct.get(f, ())), set(macro.get(f, ()))
        for dep in local.get(f, ()):
            if dep in direct:
                s2, m2 = close(dep)
                std |= s2
                mac |= m2
        seen[f] = (std, mac)
        return seen[f]

    for f in files:
        rel = os.path.relpath(f, "Source")
        STD_CLOSURE[rel], MACRO_CLOSURE[rel] = close(f)
        for name in RE_DEFN.findall(blobs[f]):
            OWNER.setdefault(name, module_for(rel))


def snake(n):
    return re.sub(r'(?<!^)(?=[A-Z])', '_', n).lower()


def module_for(include):
    path = os.path.join("Source", include)
    if path in SPLIT:
        return SPLIT[path]
    rel = os.path.relpath(os.path.dirname(path), "Source")
    return ".".join(snake(p) for p in rel.split(os.sep))


def convert(text):
    mods, std, mac = set(), set(), set()

    def sub(m):
        inc = m.group(1)
        if os.path.join("Source", inc) in MACRO_HDR:
            return m.group(0)
        mods.add(module_for(inc))
        std.update(STD_CLOSURE.get(inc, set()))
        mac.update(MACRO_CLOSURE.get(inc, set()))
        return ""

    text = RE_INC.sub(sub, text)
    if not mods:
        return text, 0

    std -= set(RE_INC_SYS.findall(text))          # do not duplicate what is there
    mac -= set(RE_INC_LOC.findall(text))

    text = RE_FWD.sub(lambda m: "" if m.group(2) in OWNER else m.group(0), text)

    # Imports replace the include block, so put them where it was: after the
    # last surviving #include, or after the file's doc comment.
    lines = text.split("\n")
    anchor = 0
    for i, l in enumerate(lines):
        if l.startswith("#include") or l.startswith("#pragma once"):
            anchor = i + 1
    block = ([""] + [f'#include "{h}"' for h in sorted(mac)]
             + [""] + [f"#include <{h}>" for h in sorted(std)]
             + [""] + [f"import {m};" for m in sorted(mods)])
    return "\n".join(lines[:anchor] + block + lines[anchor:]), len(mods)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--rev", default="HEAD",
                    help="revision holding the pre-migration headers")
    args = ap.parse_args()
    build_std_closure(args.rev)
    n = 0
    for root in ROOTS:
        for d, _, fs in os.walk(root):
            for f in fs:
                if not f.endswith((".hpp", ".cpp")):
                    continue
                p = os.path.join(d, f)
                src = open(p).read()
                out, k = convert(src)
                if k and out != src:
                    n += 1
                    if args.apply:
                        open(p, "w").write(out)
    print(f"{'converted' if args.apply else 'would convert'} {n} files")


if __name__ == "__main__":
    main()
