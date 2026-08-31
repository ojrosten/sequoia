#!/usr/bin/env python3
"""Replace every textual standard-library include with `import std;`.

    python3 scripts/modules/adopt_import_std.py [--apply]

Two shapes of file, and the difference matters.

A *module unit* carries its standard headers in a global module fragment. Once
`import std;` supplies them the fragment is usually empty, and an empty one is
noise - so it is removed unless a macro header still needs to live there, since
macros do not travel through a module.

An *ordinary translation unit* - the tests, and anything a created project
compiles - simply loses its angled includes. The ordering rule this branch runs
on still applies: textual #includes must precede every import, so the `import`
block stays last.
"""
import argparse, os, re, sys

MODULE_ROOTS = ["Source/sequoia"]
PLAIN_ROOTS  = ["Tests", "TestCommon", "TestAll", "TestChamber",
                "TestFrameworkDiagnostics", "TestFrameworkHarness",
                "aux_files", "TestMaterials"]

RE_INC_SYS  = re.compile(r'^[ \t]*#include[ \t]*<[^>]+>[ \t]*\n', re.M)
RE_GMF      = re.compile(r'^module;[ \t]*\n(.*?)^(?=export module|module )', re.M | re.S)
RE_MODDECL  = re.compile(r'^((?:export )?module [\w.:]+;)[ \t]*\n', re.M)
RE_IMPORT   = re.compile(r'^(?:export )?import [\w.:]+;[ \t]*\n', re.M)
RE_INC_ANY  = re.compile(r'^[ \t]*#include[ \t]*[<"][^>"]+[>"][ \t]*\n', re.M)


def convert_module_unit(text):
    if "import std;" in text:
        return text, False
    had = bool(RE_INC_SYS.search(text))
    text = RE_INC_SYS.sub("", text)

    # Drop a global module fragment that now holds nothing but blank lines.
    def gmf(m):
        return "" if not m.group(1).strip() else m.group(0)
    text = RE_GMF.sub(gmf, text)

    m = RE_MODDECL.search(text)
    if not m:
        return text, False
    text = text[:m.end()] + "\nimport std;\n" + text[m.end():]
    text = re.sub(r'\n{3,}', '\n\n', text)
    return text, had


def convert_plain(text):
    if "import std;" in text:
        return text, False
    had = bool(RE_INC_SYS.search(text))
    if not had:
        return text, False
    text = RE_INC_SYS.sub("", text)

    imports = list(RE_IMPORT.finditer(text))
    if imports:                      # sit beside the imports already there
        at = imports[0].start()
    else:                            # after the last surviving include
        incs = list(RE_INC_ANY.finditer(text))
        at = incs[-1].end() + 1 if incs else 0
        if incs:
            text = text[:incs[-1].end()] + "\n" + text[incs[-1].end():]
    text = text[:at] + "import std;\n" + text[at:]
    return re.sub(r'\n{3,}', '\n\n', text), True


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--apply", action="store_true")
    args = ap.parse_args()

    n = 0
    for root, conv, exts in ((MODULE_ROOTS, convert_module_unit, (".cppm", ".cpp")),
                             (PLAIN_ROOTS,  convert_plain,       (".hpp", ".cpp"))):
        for r in root:
            for d, _, fs in os.walk(r):
                for f in fs:
                    if not f.endswith(exts):
                        continue
                    p = os.path.join(d, f)
                    try:
                        src = open(p).read()
                    except (OSError, UnicodeDecodeError):
                        continue
                    out, changed = conv(src)
                    if changed and out != src:
                        n += 1
                        if args.apply:
                            open(p, "w").write(out)
    print(f"{'converted' if args.apply else 'would convert'} {n} files")


if __name__ == "__main__":
    main()
