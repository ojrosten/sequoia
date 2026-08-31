#!/usr/bin/env python3
"""Repair headers that name an entity they never declared, before migrating.

    python3 scripts/modules/fix_include_what_you_use.py [--apply]

These are genuine defects, not migration artefacts, and they survived the
"every header compiles alone" pass for a precise reason: the offending use sits
inside a *template*, so two-phase lookup defers the error to instantiation.
Compiling the header alone never instantiates it, and in the header build the
including translation unit had always already supplied the name.  Importing a
module supplies nothing implicitly, so the latent error becomes a real one.
"""
import argparse, pathlib, sys

MISSING = {
    # uses demangle<T>() and specializes type_demangler, both from Output.hpp,
    # inside templates - surfaced as a two-phase-lookup failure in every
    # allocation test.
    "Source/sequoia/TestFramework/AllocationTestUtilities.hpp":
        ("sequoia/Core/Meta/TypeTraits.hpp", "sequoia/TestFramework/Output.hpp"),
}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--apply", action="store_true")
    args = ap.parse_args()
    for path, (after, add) in MISSING.items():
        p = pathlib.Path(path)
        src = p.read_text()
        if f'#include "{add}"' in src:
            print(f"  already present: {path}")
            continue
        anchor = f'#include "{after}"\n'
        if anchor not in src:
            sys.exit(f"anchor not found in {path}: {after}")
        out = src.replace(anchor, anchor + f'#include "{add}"\n', 1)
        if args.apply:
            p.write_text(out)
        print(f"  {'+' if args.apply else 'would add'} {add} -> {path}")


if __name__ == "__main__":
    main()
