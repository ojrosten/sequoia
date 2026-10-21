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
  - the number of symbols nm lists for the executable, and the size in bytes
    of the string table holding their names;
  - for each object among the target's inputs, as ninja lists them, the
    number of symbols the object defines, keyed by the path of its source
    relative to the repository. The inputs include every member of a static
    library the target links, whether or not the linker takes it.

The string table also holds names nm does not list: on Mach-O, the debug
map's paths to the objects; on ELF, the source files' paths. So its size
changes with the path of the checkout, and records are comparable only
between builds at the same path.

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
  - the executable is not a 64-bit little-endian ELF or Mach-O file with one
    symbol table, or nm lists no symbols for it;
  - ninja lists no objects for the target, or its compilation database has
    no entry for one, or two objects share a source, or a source cannot be
    named relative to the repository.
Windows executables are not supported.
"""
import argparse, concurrent.futures, json, os, re, struct, subprocess, sys

# An entry of CMakeCache.txt: NAME:TYPE=VALUE.
CACHE_ENTRY = re.compile(r"(?P<name>[A-Za-z_][^:]*):(?P<type>[A-Z]+)=(?P<value>.*)")

# An entry of a compiler record: set(NAME "VALUE").
RECORD_ENTRY = re.compile(r'set\((?P<name>[A-Za-z_]\w*) "(?P<value>[^"]*)"\)')

# The C++ standard libraries, keyed by their names in a compiler record's
# CMAKE_CXX_IMPLICIT_LINK_LIBRARIES.
STANDARD_LIBRARIES = {"c++": "libc++", "stdc++": "libstdc++"}

# libc++'s inline namespace, std::__1, as it appears in a mangled name.
LIBCXX_NAMESPACE = "St3__1"

ELF_MAGIC           = b"\x7fELF"
E_SHOFF             = 0x28                 # offsets within a 64-bit ELF header
E_SHENTSIZE         = 0x3A
SHT_SYMTAB          = 2
SHT_STRTAB          = 3
MACH_O_MAGIC        = b"\xcf\xfa\xed\xfe"  # MH_MAGIC_64, little-endian
MH_NCMDS            = 16                   # an offset within a 64-bit Mach-O header
MACH_HEADER_64_SIZE = 32
LC_SYMTAB           = 0x2


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


def read_cache(build_dir):
    """Returns the entries of the build directory's CMakeCache.txt, by name."""
    path = os.path.join(build_dir, "CMakeCache.txt")
    try:
        with open(path, encoding="utf-8") as f:
            lines = f.read().splitlines()
    except (OSError, UnicodeDecodeError) as e:
        sys.exit(f"{path} cannot be read: {e}")
    return {m["name"]: m["value"] for m in map(CACHE_ENTRY.fullmatch, lines) if m}


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
    record = {m["name"]: m["value"] for m in RECORD_ENTRY.finditer(text)}
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


def format_and_string_table_size(path):
    """Returns the format of the executable at `path`, and the size in bytes
    of the string table holding the names of its symbols.

    Exits unless the executable is a 64-bit little-endian ELF or Mach-O file
    with one symbol table.
    """
    try:
        with open(path, "rb") as f:
            magic = f.read(4)
            if magic == ELF_MAGIC:
                return "ELF", elf_string_table_size(f, path)
            if magic == MACH_O_MAGIC:
                return "Mach-O", mach_o_string_table_size(f, path)
    except OSError as e:
        sys.exit(f"{path} cannot be read: {e}")
    except struct.error:
        sys.exit(f"{path} is truncated")
    sys.exit(f"{path} is neither a 64-bit little-endian ELF file nor a 64-bit little-endian Mach-O file")


def read_at(f, offset, size):
    """Returns the `size` bytes of `f` at `offset`.

    Raises struct.error if the file ends before them.
    """
    f.seek(offset)
    data = f.read(size)
    if len(data) != size:
        raise struct.error("truncated")
    return data


def elf_string_table_size(f, path):
    """Returns the size of the string table to which the ELF file's symbol
    table links."""
    elf_class, data_encoding = read_at(f, 4, 2)
    if (elf_class, data_encoding) != (2, 1):
        sys.exit(f"{path} is an ELF file, but not a 64-bit little-endian one")
    shoff, = struct.unpack("<Q", read_at(f, E_SHOFF, 8))
    shentsize, shnum = struct.unpack("<HH", read_at(f, E_SHENTSIZE, 4))
    def section(index):  # (sh_type, sh_size, sh_link)
        fields = struct.unpack("<IIQQQQIIQQ", read_at(f, shoff + index * shentsize, 64))
        return fields[1], fields[5], fields[6]
    # A file with too many sections for e_shnum holds the count in section
    # 0's sh_size.
    if shnum == 0 and shoff:
        shnum = section(0)[1]
    sections = [section(i) for i in range(shnum)]
    symbol_tables = [s for s in sections if s[0] == SHT_SYMTAB]
    if len(symbol_tables) != 1:
        sys.exit(f"{path} has {len(symbol_tables)} symbol tables, rather than one")
    link = symbol_tables[0][2]
    if link >= len(sections) or sections[link][0] != SHT_STRTAB:
        sys.exit(f"the symbol table of {path} does not link to a string table")
    return sections[link][1]


def mach_o_string_table_size(f, path):
    """Returns the size of the string table which the Mach-O file's LC_SYMTAB
    command names."""
    ncmds, sizeofcmds = struct.unpack("<II", read_at(f, MH_NCMDS, 8))
    commands = read_at(f, MACH_HEADER_64_SIZE, sizeofcmds)
    sizes, offset = [], 0
    for _ in range(ncmds):
        cmd, cmdsize = struct.unpack_from("<II", commands, offset)
        if cmdsize == 0:
            sys.exit(f"{path} has a load command of size 0")
        if cmd == LC_SYMTAB:
            sizes.append(struct.unpack_from("<IIII", commands, offset + 8)[3])
        offset += cmdsize
    if len(sizes) != 1:
        sys.exit(f"{path} has {len(sizes)} LC_SYMTAB commands, rather than one")
    return sizes[0]


def symbols(nm, path, *options):
    """Returns the symbols which nm lists for the file at `path`, one line
    each."""
    return [line for line in run([nm, "-P", *options, path]).splitlines() if line]


def nm_version(nm):
    """Returns the first line of `nm --version` holding a digit.

    llvm-nm's first line is "llvm-nm, compatible with GNU nm", and its
    version follows on a later line.
    """
    lines = [line for line in run([nm, "--version"]).splitlines() if re.search(r"\d", line)]
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
    file_format, string_table_bytes = format_and_string_table_size(executable)
    nm = entry(cache, "CMAKE_NM", "CMakeCache.txt")
    listed = symbols(nm, executable)
    if not listed:
        sys.exit(f"nm lists no symbols for {executable}")
    if any(LIBCXX_NAMESPACE in line for line in listed) != (library == "libc++"):
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
        "executable": {"format": file_format,
                       "symbols": len(listed),
                       "string_table_bytes": string_table_bytes},
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
