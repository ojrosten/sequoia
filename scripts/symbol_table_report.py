#!/usr/bin/env python3
"""Record the size of an executable's symbol table, as one JSON record.

  symbol_table_report.py <build-dir> <target>

Writes the record to standard output. The build directory must be one which
CMake configured with the Ninja generator. The target is the executable's path
relative to the build directory, which is also its name as a ninja target.
The record holds:
  - the commit at which the source tree stands, and whether the tree is clean;
  - the toolchain: the C++ compiler's and the linker's paths, ids and
    versions, and the standard library, as recorded by the build directory,
    and the path and version of the build directory's nm;
  - the cache's build type and flags. A target's own flags are not among them;
  - the number of symbols nm lists for the executable, and the total length
    of their names in bytes;
  - for each object among the target's inputs, as ninja lists them, the
    number of symbols the object defines, keyed by the path of its source
    relative to the repository.

The names are those nm lists by default, imported symbols among them. Each
is mangled, as the executable holds it, so on Mach-O it starts with the
underscore Mach-O prefixes to C and C++ names, and the newline ending it in
nm's listing is not counted. nm lists no debugging symbols by default, so
the total does not depend on the path of the checkout, unless a name holds
a path itself. On Mach-O, the size of the string table holding the names
would depend on it, since the table also holds the debug map's paths to the
objects.

The objects are ninja's inputs to the link. So every member of a static
library the target links is among them, and is counted whether or not the
linker takes it from the library. The executable's figures are read from
the executable itself, and are unaffected.

The objects' counts do not sum to the executable's. An inline function or a
template instantiation used in several objects is defined in each of them,
and the linker keeps one copy. The executable's count also includes the
symbols it imports.

The toolchain comes from the compiler record CMake writes when it first
configures the build directory,
CMakeFiles/<cmake version>/CMakeCXXCompiler.cmake, and not from the
environment. A compiler upgraded in place leaves the record naming the old
one. So the script refuses a record naming a compiler or an implicit include
directory which no longer exists. An upgrade that keeps those paths goes
unseen.

The script refuses, rather than write a record which may be wrong, if
  - ninja has work to do for the target, since the executable would not be
    the source tree's;
  - the compiler record cannot be found, or lacks an entry, or names a
    compiler or an implicit include directory which no longer exists;
  - the compiler record names both standard libraries or neither, or names
    the one whose inline namespace the executable's symbols do not show;
  - the executable is not a file, or cannot be read, or is neither an ELF
    file nor a thin Mach-O file, or nm lists no symbols for it;
  - ninja lists no objects for the target, or its compilation database has
    no entry for one, or two objects share a source, or a source cannot be
    named relative to the repository.
Windows executables are not supported.
"""
import argparse, concurrent.futures, json, os, string, subprocess, sys

# The characters which may start the name of an entry of CMakeCache.txt or
# of a compiler record.
NAME_INITIAL_CHARACTERS = string.ascii_letters + "_"

# The C++ standard libraries, keyed by their names in a compiler record's
# CMAKE_CXX_IMPLICIT_LINK_LIBRARIES.
STANDARD_LIBRARIES = {"c++": "libc++", "stdc++": "libstdc++"}

# libc++'s inline namespace, std::__1, as it appears in a mangled name.
LIBCXX_NAMESPACE = "St3__1"

# The first bytes of an ELF file, and of a thin Mach-O file of either width
# and byte order.
IMAGE_MAGICS = (b"\x7fELF",
                b"\xcf\xfa\xed\xfe", b"\xce\xfa\xed\xfe",
                b"\xfe\xed\xfa\xcf", b"\xfe\xed\xfa\xce")


def run(command):
    """Returns the standard output of `command`.

    Exits, naming the command, if it cannot be started or fails.
    """
    try:
        r = subprocess.run(command, capture_output=True, encoding="utf-8")
    except OSError as e:
        sys.exit(f"{command[0]} could not be run: {e}")
    if r.returncode:
        sys.exit(f"{' '.join(command)} failed with exit status {r.returncode}:\n{r.stderr}{r.stdout}".rstrip())
    return r.stdout


def entry_of_cache_line(line):
    """Returns the name and value of `line`, an entry of CMakeCache.txt.

    An entry is NAME:TYPE=VALUE. The name runs to the first colon, and
    starts with an ASCII letter or an underscore. The type runs from that
    colon to the next equals sign, and is ASCII upper-case letters. The
    value is the rest of the line. For any other line, returns None.
    """
    name, _, rest = line.partition(":")
    entry_type, equals_sign, value = rest.partition("=")
    if (name and name[0] in NAME_INITIAL_CHARACTERS and equals_sign
            and entry_type and all(c in string.ascii_uppercase for c in entry_type)):
        return name, value
    return None


def entry_of_record_line(line):
    """Returns the name and value of `line`, an entry of a compiler record.

    An entry is set(NAME "VALUE"), alone on its line and unindented: an
    indented one lies within a condition. The name starts with an ASCII
    letter or an underscore, which letters, digits and underscores of any
    script follow. The value holds no quote. For any other line, returns
    None.
    """
    if not (line.startswith("set(") and line.endswith('")')):
        return None
    name, separator, value = line.removeprefix("set(").removesuffix('")').partition(' "')
    if (separator and name and name[0] in NAME_INITIAL_CHARACTERS
            and all(c.isalnum() or c == "_" for c in name) and '"' not in value):
        return name, value
    return None


def read_cache(build_dir):
    """Returns the entries of the build directory's CMakeCache.txt, by name."""
    path = os.path.join(build_dir, "CMakeCache.txt")
    try:
        with open(path, encoding="utf-8") as f:
            lines = f.read().splitlines()
    except (OSError, UnicodeDecodeError) as e:
        sys.exit(f"{path} cannot be read: {e}")
    return dict(filter(None, map(entry_of_cache_line, lines)))


def entry(entries, name, file_name):
    """Returns the value of the entry `name`.

    Exits, naming `file_name`, if there is no such entry.
    """
    if name not in entries:
        sys.exit(f"{file_name} has no entry {name}")
    return entries[name]


def compiler_record_path(build_dir, cache):
    """Returns the path of the compiler record written by the CMake which
    configured the build directory.

    A build directory configured by several releases of CMake holds a record
    for each. Exits unless exactly one record matches the cache's version.
    """
    version = ".".join(entry(cache, f"CMAKE_CACHE_{part}_VERSION", "CMakeCache.txt")
                       for part in ("MAJOR", "MINOR", "PATCH"))
    files = os.path.join(build_dir, "CMakeFiles")
    matches = sorted(d for d in os.listdir(files)
                     if (d == version or d.startswith(version + "-"))
                     and os.path.isfile(os.path.join(files, d, "CMakeCXXCompiler.cmake")))
    if len(matches) != 1:
        sys.exit(f"{files} holds {len(matches)} C++ compiler records for CMake {version}, rather than one: "
                 + (", ".join(matches) or "none"))
    return os.path.join(files, matches[0], "CMakeCXXCompiler.cmake")


def read_compiler_record(path):
    """Returns the entries of the compiler record at `path`, by name.

    Exits if an entry the script reads is absent or empty, or if the record
    names a compiler or an implicit include directory which no longer
    exists.
    """
    try:
        with open(path, encoding="utf-8") as f:
            text = f.read()
    except (OSError, UnicodeDecodeError) as e:
        sys.exit(f"{path} cannot be read: {e}")
    record = dict(filter(None, map(entry_of_record_line, text.split("\n"))))
    for name in ("CMAKE_CXX_COMPILER", "CMAKE_CXX_COMPILER_ID", "CMAKE_CXX_COMPILER_VERSION",
                 "CMAKE_CXX_COMPILER_LINKER", "CMAKE_CXX_COMPILER_LINKER_ID",
                 "CMAKE_CXX_COMPILER_LINKER_VERSION", "CMAKE_CXX_IMPLICIT_LINK_LIBRARIES",
                 "CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES"):
        if not entry(record, name, path):
            sys.exit(f"{path} has an empty entry {name}")
    for named in [record["CMAKE_CXX_COMPILER"], *record["CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES"].split(";")]:
        if not os.path.exists(named):
            sys.exit(f"{path} names {named}, which no longer exists: the compiler has changed since "
                     "the build directory was configured. Configure it afresh.")
    return record


def standard_library(record, record_path):
    """Returns the standard library which the compiler record says the
    compiler links.

    Exits unless the record names exactly one.
    """
    linked = record["CMAKE_CXX_IMPLICIT_LINK_LIBRARIES"].split(";")
    named = [name for library, name in STANDARD_LIBRARIES.items() if library in linked]
    if len(named) != 1:
        sys.exit(f"{record_path} names {len(named)} standard libraries among those the compiler "
                 f"links, rather than one: {record['CMAKE_CXX_IMPLICIT_LINK_LIBRARIES']}")
    return named[0]


def ninja(cache, build_dir, *arguments):
    """Returns the output of the build directory's ninja, run with `arguments`."""
    return run([entry(cache, "CMAKE_MAKE_PROGRAM", "CMakeCache.txt"), "-C", build_dir, *arguments])


def symbols(nm, path, *options):
    """Returns the names of the symbols which nm lists for the file at
    `path`."""
    return [name for name in run([nm, "--format=just-symbols", *options, path]).split("\n") if name]


def nm_version(nm):
    """Returns the first line of `nm --version` holding a digit.

    llvm-nm's first line is "llvm-nm, compatible with GNU nm", and its
    version follows on a later line.
    """
    lines = [line for line in run([nm, "--version"]).splitlines()
             if any(c.isdecimal() for c in line)]
    if not lines:
        sys.exit(f"{nm} --version prints no version")
    return lines[0].strip()


def linked_objects_by_source(cache, build_dir, target, root):
    """Returns the path of each object ninja links into `target`, keyed by
    the path of its source relative to the repository at `root`.

    Exits if ninja lists no objects, if ninja's compilation database has no
    entry for one, or if two objects share a source. Also exits if a source
    lies outside the repository: a path outside it differs between machines.
    """
    linked = [line for line in ninja(cache, build_dir, "-t", "inputs", target).splitlines()
              if line.endswith(".o")]
    if not linked:
        sys.exit(f"ninja lists no objects among the inputs of {target}")
    try:
        compilations = {e["output"]: e for e in json.loads(ninja(cache, build_dir, "-t", "compdb"))}
    except (json.JSONDecodeError, TypeError, KeyError) as e:
        sys.exit(f"ninja's compilation database for {build_dir} cannot be read: {e}")
    root = os.path.realpath(root)
    out = {}
    for obj in linked:
        if obj not in compilations:
            sys.exit(f"ninja's compilation database has no entry for {obj}")
        compilation = compilations[obj]
        source = os.path.realpath(os.path.join(compilation["directory"], compilation["file"]))
        if os.path.commonpath([source, root]) != root:
            sys.exit(f"the source of {obj}, {source}, lies outside the repository {root}")
        key = os.path.relpath(source, root)
        if key in out:
            sys.exit(f"{out[key]} and {obj} are both compiled from {key}")
        out[key] = obj
    return out


def record_of(build_dir, target):
    """Returns the record of the executable `target` in `build_dir`."""
    cache = read_cache(build_dir)
    generator = entry(cache, "CMAKE_GENERATOR", "CMakeCache.txt")
    if generator != "Ninja":
        sys.exit(f"{build_dir} was configured with the generator {generator!r}, not 'Ninja'")
    if "ninja: no work to do." not in ninja(cache, build_dir, "-n", target).splitlines():
        sys.exit(f"ninja has work to do for {target} in {build_dir}: build it first")

    record_path = compiler_record_path(build_dir, cache)
    record = read_compiler_record(record_path)
    library = standard_library(record, record_path)

    executable = os.path.join(build_dir, target)
    if not os.path.isfile(executable):
        sys.exit(f"{executable} is not a file")
    try:
        with open(executable, "rb") as f:
            first_bytes = f.read(max(map(len, IMAGE_MAGICS)))
    except OSError as e:
        sys.exit(f"{executable} cannot be read: {e}")
    # nm lists a static library member by member, or merged, and a universal
    # binary for one architecture alone, with no error.
    if not first_bytes.startswith(IMAGE_MAGICS):
        sys.exit(f"{executable} is neither an ELF file nor a thin Mach-O file")
    nm = entry(cache, "CMAKE_NM", "CMakeCache.txt")
    listed = symbols(nm, executable)
    if not listed:
        sys.exit(f"nm lists no symbols for {executable}")
    if any(LIBCXX_NAMESPACE in name for name in listed) != (library == "libc++"):
        sys.exit(f"{record_path} says the compiler links {library}, and the symbols of {executable} "
                 f"say otherwise: they {'do not ' if library == 'libc++' else ''}include libc++'s "
                 "inline namespace")

    source_dir = entry(cache, "CMAKE_HOME_DIRECTORY", "CMakeCache.txt")
    root = run(["git", "-C", source_dir, "rev-parse", "--show-toplevel"]).strip()
    objects_by_source = linked_objects_by_source(cache, build_dir, target, root)
    with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as pool:
        counts = dict(zip(objects_by_source, pool.map(
            lambda obj: len(symbols(nm, os.path.join(build_dir, obj), "--defined-only")),
            objects_by_source.values())))

    build_type = entry(cache, "CMAKE_BUILD_TYPE", "CMakeCache.txt")
    flags = ["CMAKE_CXX_FLAGS", "CMAKE_EXE_LINKER_FLAGS"]
    if build_type:
        flags += [f"CMAKE_CXX_FLAGS_{build_type.upper()}", f"CMAKE_EXE_LINKER_FLAGS_{build_type.upper()}"]
    return {
        "commit": run(["git", "-C", root, "rev-parse", "HEAD"]).strip(),
        "clean": run(["git", "-C", root, "status", "--porcelain", "--untracked-files=all"]) == "",
        "target": target,
        "toolchain": {
            "compiler": {"path":    record["CMAKE_CXX_COMPILER"],
                         "id":      record["CMAKE_CXX_COMPILER_ID"],
                         "version": record["CMAKE_CXX_COMPILER_VERSION"]},
            "linker":   {"path":    record["CMAKE_CXX_COMPILER_LINKER"],
                         "id":      record["CMAKE_CXX_COMPILER_LINKER_ID"],
                         "version": record["CMAKE_CXX_COMPILER_LINKER_VERSION"]},
            "standard_library": library,
            "nm": {"path": nm, "version": nm_version(nm)},
        },
        "cache": {name: entry(cache, name, "CMakeCache.txt") for name in ["CMAKE_BUILD_TYPE", *flags]},
        "executable": {"symbols":           len(listed),
                       "symbol_name_bytes": sum(len(name.encode("utf-8")) for name in listed)},
        "defined_symbols_by_object": counts,
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("build_dir")
    ap.add_argument("target", help="the executable's path relative to the build directory")
    a = ap.parse_args()
    json.dump(record_of(a.build_dir, a.target), sys.stdout, indent=1, sort_keys=True)
    print()


if __name__ == "__main__":
    main()
