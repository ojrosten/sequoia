#!/usr/bin/env python3
"""Prints a compile-cost record, as JSON on standard output: the instructions
which compiling each of a few translation units executes, and the commit and
toolchain they belong to.

  compile_cost_record.py <build-dir> <source>...

Each source is a path relative to the root of the git repository which holds
the build directory's source tree. The build directory must be one which
CMake configured with the Ninja generator; it need not be built.

The script compiles each source once, by the command which ninja's
compilation database holds for it, under valgrind's cachegrind with no cache
simulation. Cachegrind follows the compiler's driver into each process the
driver starts, such as the compiler proper and the assembler. A source's
count is the instructions (cachegrind's Ir) of all these processes, summed.
The command writes its object, and its file of dependencies if it names one,
to /dev/null, so the script leaves the build directory as it was. A compiler
is given the options COMPILER_OPTIONS names for its id first: clang reads no
configuration file. Each
compilation runs in a fixed environment, COMPILATION_ENVIRONMENT, not the
caller's.

The record holds:
  - commit, the commit at which the repository stands, and clean, whether
    its tree is clean, untracked files included;
  - toolchain:
      - compiler, the path, id and version of the C++ compiler, from the
        compiler record which CMake wrote when it configured the build
        directory;
      - standard_library, the C++ standard library which that record says
        the compiler links, and the compiler's implicit include directories;
      - valgrind, the path and version of the valgrind which counted;
      - build, the cache's build type and C++ flags;
  - compile_cost:
      - boundary, which names what was counted;
      - units, each source's count, keyed by the source as given.

The script refuses, and writes nothing, if
  - two sources resolve to one file, or a source is not a file within the
    repository;
  - the build directory's cache or compiler record is not one which
    symbol_table_report.py reads;
  - ninja fails, or its compilation database is not JSON;
  - the compilation database has no compilation of a source, or more than
    one;
  - a compilation's command does not start with the compiler which the
    compiler record names, or has no -o, or more than one, or more than one
    -MF, or ends with either;
  - valgrind is not on PATH, or cannot be run, or a compilation fails under
    it;
  - valgrind leaves no file of counts for a compilation, or a file whose
    summary lines are not exactly one, or whose summary is no count;
  - the files of counts of a compilation count no instructions.
"""
import argparse, json, os, shlex, shutil, subprocess, sys, tempfile

import symbol_table_report

# The options of a compiler's command which name a file the compilation
# writes, -o its object and -MF its dependencies, each with the least number
# of times a command may hold it. A command may hold neither more than once.
OUTPUT_OPTIONS = {"-o": 1, "-MF": 0}

# The whole environment of each compilation. The count depends on the
# environment: g++-15's count of one unit grew by 0.16% with 10 kB more of
# it, and fell by 0.013% with TMPDIR elsewhere. So every compilation sees
# these variables alone, each with the same value.
COMPILATION_ENVIRONMENT = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "TMPDIR": "/tmp"}

# The options which valgrind is given, before the command it runs.
VALGRIND_OPTIONS = ["--tool=cachegrind", "--cache-sim=no", "--trace-children=yes"]

# The options which each compiler, by its id in the compiler record, is given
# before the command's own. Clang reads configuration files, such as a
# clang++.cfg beside its binary, whatever the environment, and they may add
# options to every compilation; --no-default-config makes it read none.
COMPILER_OPTIONS = {"Clang": ["--no-default-config"]}

# The description of the count, which the record holds as its boundary. A
# change to valgrind's options, to the processes whose instructions the
# script sums, to the outputs or the options of the command or to its
# environment must change it, so that a reader of two records can tell
# whether their counts compare.
BOUNDARY = (f"instructions under valgrind {' '.join(VALGRIND_OPTIONS)}, summed over the processes of the "
            "compilation which ninja's compilation database holds, its outputs to /dev/null, "
            + "".join(f"{compiler_id} given {' '.join(options)} first, "
                      for compiler_id, options in COMPILER_OPTIONS.items())
            + "in an environment of "
            + ", ".join(f"{name}={value}" for name, value in COMPILATION_ENVIRONMENT.items()) + " alone")


def measured_command(command, source, compiler):
    """Returns the words of the command to measure: `command`, split into
    words, with the options which COMPILER_OPTIONS names for `compiler`'s id
    after its first word, and the word after each of OUTPUT_OPTIONS replaced
    by /dev/null.

    Exits unless the command's first word is `compiler`'s path, and the
    command holds each of OUTPUT_OPTIONS at least as often as OUTPUT_OPTIONS
    says and at most once, and does not end with one.
    """
    words = shlex.split(command)
    if not words or words[0] != compiler["path"]:
        sys.exit(f"the compilation of {source} does not run the compiler record's compiler, {compiler['path']}: "
                 f"{command}")
    for option, least in OUTPUT_OPTIONS.items():
        count = words.count(option)
        if count < least or count > 1:
            allowed = "one" if least == 1 else "none or one"
            sys.exit(f"the compilation of {source} has {count} {option} options, rather than {allowed}: {command}")
        if words[-1] == option:
            sys.exit(f"the compilation of {source} ends with {option}, which names no file: {command}")
    discarded = [os.devnull if i > 0 and words[i - 1] in OUTPUT_OPTIONS else word for i, word in enumerate(words)]
    return [discarded[0], *COMPILER_OPTIONS.get(compiler["id"], []), *discarded[1:]]


def compilation_of(database, root, source):
    """Returns the one compilation of `source`, a path relative to `root`,
    in the compilation database `database`.

    Exits unless the database holds exactly one compilation of the file
    which `source` resolves to.
    """
    path = os.path.realpath(os.path.join(root, source))
    found = [compilation for compilation in database
             if os.path.realpath(os.path.join(compilation["directory"], compilation["file"])) == path]
    if len(found) != 1:
        sys.exit(f"ninja's compilation database has {len(found)} compilations of {source}, rather than one")
    return found[0]


def summed_instructions(counts_dir):
    """Returns the sum of the instructions of the files of counts which
    cachegrind left in `counts_dir`.

    Exits unless there is at least one file, each holds exactly one summary
    line, each summary is a count, and the sum is not zero.
    """
    names = sorted(os.listdir(counts_dir))
    if not names:
        sys.exit("valgrind left no file of counts")

    def instructions_in(name):
        with open(os.path.join(counts_dir, name), encoding="utf-8") as f:
            summaries = [line.removeprefix("summary:").strip() for line in f.read().splitlines()
                         if line.startswith("summary:")]
        if len(summaries) != 1:
            sys.exit(f"cachegrind's file of counts {name} has {len(summaries)} summary lines, rather than one")
        if not summaries[0].isdecimal():
            sys.exit(f"cachegrind's file of counts {name} has a summary which is no count of instructions: "
                     f"'{summaries[0]}'")
        return int(summaries[0])

    total = sum(map(instructions_in, names))
    if total == 0:
        sys.exit("valgrind counted no instructions")
    return total


def instructions_of_compiling(compilation, source, compiler, valgrind):
    """Returns the instructions of `compilation`, run once under the
    valgrind at the path `valgrind`.

    Exits if the command is refused by measured_command, if
    valgrind cannot be run or the compilation fails under it, or if the
    counts are refused by summed_instructions.
    """
    words = measured_command(compilation["command"], source, compiler)
    with tempfile.TemporaryDirectory() as counts_dir:
        try:
            r = subprocess.run([valgrind, *VALGRIND_OPTIONS, f"--cachegrind-out-file={counts_dir}/%p", *words],
                               cwd=compilation["directory"], env=COMPILATION_ENVIRONMENT, capture_output=True,
                               encoding="utf-8")
        except OSError as e:
            sys.exit(f"valgrind could not be run: {e}")
        if r.returncode:
            sys.exit(f"the compilation of {source} failed under valgrind with exit status {r.returncode}:\n"
                     f"{r.stderr}{r.stdout}".rstrip())
        return summed_instructions(counts_dir)


def compile_cost_of(build_dir, sources):
    """Returns the compile-cost record of `sources`, compiled as `build_dir`
    compiles them."""
    cache = symbol_table_report.read_cache(build_dir)
    root = os.path.realpath(symbol_table_report.repository_root(cache))
    paths = [os.path.realpath(os.path.join(root, source)) for source in sources]
    if len(set(paths)) != len(paths):
        sys.exit(f"two of the sources resolve to one file: {' '.join(sources)}")
    for source, path in zip(sources, paths):
        if os.path.commonpath([path, root]) != root or not os.path.isfile(path):
            sys.exit(f"{source} is not a file within the repository {root}")
    record_path = symbol_table_report.compiler_record_path(build_dir, cache)
    compiler_record = symbol_table_report.read_compiler_record(record_path)
    compiler = symbol_table_report.tool_of(compiler_record, "CMAKE_CXX_COMPILER")
    valgrind = shutil.which("valgrind")
    if valgrind is None:
        sys.exit("valgrind could not be run: there is none on PATH")
    try:
        database = json.loads(symbol_table_report.ninja(cache, build_dir, "-t", "compdb"))
    except json.JSONDecodeError as e:
        sys.exit(f"ninja's compilation database for {build_dir} cannot be read: {e}")
    units = {source: instructions_of_compiling(compilation_of(database, root, source), source, compiler, valgrind)
             for source in sources}
    return {
        **symbol_table_report.tree_state(root),
        "toolchain": {
            "compiler": compiler,
            "standard_library": {
                "name": symbol_table_report.standard_library(compiler_record, record_path),
                "implicit_include_directories": compiler_record["CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES"].split(";")},
            "valgrind": {"path": valgrind, "version": symbol_table_report.run([valgrind, "--version"]).strip()},
            "build": symbol_table_report.build_settings(cache, ["CMAKE_CXX_FLAGS"]),
        },
        "compile_cost": {"boundary": BOUNDARY, "units": units},
    }


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("build_dir")
    ap.add_argument("sources", nargs="+", metavar="source", help="a path relative to the repository")
    a = ap.parse_args()
    json.dump(compile_cost_of(a.build_dir, a.sources), sys.stdout, indent=1, sort_keys=True)
    print()


if __name__ == "__main__":
    main()
