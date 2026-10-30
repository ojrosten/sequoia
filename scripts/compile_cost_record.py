#!/usr/bin/env python3
"""Prints the compile_cost field of a symbol-table record, as JSON on standard
output: the instructions which compiling each of a few translation units
executes.

  compile_cost_record.py <build-dir> <source>...

Each source is a path relative to the repository which holds the build
directory's source tree. The build directory must be one which CMake
configured with the Ninja generator.

The script compiles each source once, by the command which ninja's
compilation database holds for it, under valgrind's cachegrind with no cache
simulation. Cachegrind follows the compiler's driver into each process the
driver starts, such as the compiler proper and the assembler. A source's
count is the instructions (cachegrind's Ir) of all these processes, summed.
The command writes its object, and its file of dependencies if it names one,
to /dev/null, so the script leaves the build directory as it was. The
compilation runs in an environment of its own, the same for every
compilation: COMPILATION_ENVIRONMENT.

The field holds:
  - boundary, which names what was counted;
  - compiler, the path, id and version of the C++ compiler, from the
    compiler record CMake wrote when it configured the build directory;
  - units, each source's count, keyed by the source as given.

The script refuses, and writes nothing, if
  - a source is given twice, or is not a file within the repository;
  - the compilation database has no compilation of a source, or more than
    one;
  - a compilation's command has no -o, or more than one, or more than one
    -MF, or ends with either;
  - the build directory's compiler record is not one which
    symbol_table_report.py reads;
  - ninja fails, or its compilation database is not JSON;
  - valgrind is not on PATH, or cannot be run, or a compilation fails under
    it;
  - valgrind leaves no file of counts for a compilation, or a file whose
    summary lines are not exactly one, or whose summary is no count;
  - the files of counts of a compilation count no instructions.
"""
import argparse, json, os, shlex, shutil, subprocess, sys, tempfile

import symbol_table_report

# The options of a compiler's command which name a file the compilation
# writes: -o its object, and -MF its dependencies. Each maps to the least
# number of times a command may hold it, and to the numbers allowed, in words.
# A command may hold neither more than once.
OUTPUT_OPTIONS = {"-o": (1, "one"), "-MF": (0, "none or one")}

# The whole environment of each compilation. The count depends on the
# environment: g++-15's count of one unit grew by 0.16% with 10 kB more of
# it, and fell by 0.013% with TMPDIR elsewhere. So every compilation sees
# these variables alone, each with the same value.
COMPILATION_ENVIRONMENT = {"PATH": "/usr/bin:/bin", "LC_ALL": "C", "TMPDIR": "/tmp"}

# The description of the count, which the field holds as its boundary. A
# change to the tool, to its options, to the processes whose instructions the
# script sums or to the outputs of the command must change it, so that a
# reader of two records can tell whether their counts compare.
BOUNDARY = ("instructions under valgrind --tool=cachegrind --cache-sim=no --trace-children=yes, summed over "
            "the processes of the compilation which ninja's compilation database holds, its outputs to /dev/null, "
            "in an environment of "
            + ", ".join(f"{name}={value}" for name, value in COMPILATION_ENVIRONMENT.items()) + " alone")


def command_with_outputs_discarded(command, source):
    """Returns `command`, split into words, with the file which each of
    OUTPUT_OPTIONS names replaced by /dev/null.

    Exits unless the command has each option as often as OUTPUT_OPTIONS
    allows, each followed by a word naming its file.
    """
    words = shlex.split(command)
    for option, (least, allowed) in OUTPUT_OPTIONS.items():
        count = words.count(option)
        if count < least or count > 1:
            sys.exit(f"the compilation of {source} has {count} {option} options, rather than {allowed}: {command}")
        if words[-1] == option:
            sys.exit(f"the compilation of {source} ends with {option}, which names no file: {command}")
    return [os.devnull if i > 0 and words[i - 1] in OUTPUT_OPTIONS else word for i, word in enumerate(words)]


def compilation_of(database, root, source):
    """Returns the one compilation of `source` in the compilation database
    `database`."""
    path = os.path.realpath(os.path.join(root, source))
    found = [compilation for compilation in database
             if os.path.realpath(os.path.join(compilation["directory"], compilation["file"])) == path]
    if len(found) != 1:
        sys.exit(f"ninja's compilation database has {len(found)} compilations of {source}, rather than one")
    return found[0]


def summed_instructions(counts_dir):
    """Returns the sum of the instructions of the files of counts which
    cachegrind left in `counts_dir`."""
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


def instructions_of_compiling(compilation, source):
    """Returns the instructions of `compilation`, run once under cachegrind."""
    words = command_with_outputs_discarded(compilation["command"], source)
    valgrind = shutil.which("valgrind")
    if valgrind is None:
        sys.exit("valgrind could not be run: there is none on PATH")
    with tempfile.TemporaryDirectory() as counts_dir:
        try:
            r = subprocess.run([valgrind, "--tool=cachegrind", "--cache-sim=no", "--trace-children=yes",
                                f"--cachegrind-out-file={counts_dir}/%p", *words],
                               cwd=compilation["directory"], env=COMPILATION_ENVIRONMENT, capture_output=True,
                               encoding="utf-8")
        except OSError as e:
            sys.exit(f"valgrind could not be run: {e}")
        if r.returncode:
            sys.exit(f"the compilation of {source} failed under valgrind with exit status {r.returncode}:\n"
                     f"{r.stderr}{r.stdout}".rstrip())
        return summed_instructions(counts_dir)


def field_of(build_dir, sources):
    """Returns the compile_cost field of `sources`, compiled as `build_dir`
    compiles them."""
    if len(set(sources)) != len(sources):
        sys.exit(f"a source is given more than once: {' '.join(sources)}")
    cache = symbol_table_report.read_cache(build_dir)
    source_dir = symbol_table_report.entry(cache, "CMAKE_HOME_DIRECTORY", "CMakeCache.txt")
    root = os.path.realpath(symbol_table_report.run(["git", "-C", source_dir, "rev-parse", "--show-toplevel"]).strip())
    for source in sources:
        path = os.path.realpath(os.path.join(root, source))
        if os.path.commonpath([path, root]) != root or not os.path.isfile(path):
            sys.exit(f"{source} is not a file within the repository {root}")
    compiler_record = symbol_table_report.read_compiler_record(
        symbol_table_report.compiler_record_path(build_dir, cache))
    try:
        database = json.loads(symbol_table_report.ninja(cache, build_dir, "-t", "compdb"))
    except json.JSONDecodeError as e:
        sys.exit(f"ninja's compilation database for {build_dir} cannot be read: {e}")
    return {"boundary": BOUNDARY,
            "compiler": symbol_table_report.compiler_of(compiler_record),
            "units": {source: instructions_of_compiling(compilation_of(database, root, source), source)
                      for source in sources}}


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("build_dir")
    ap.add_argument("sources", nargs="+", metavar="source", help="a path relative to the repository")
    a = ap.parse_args()
    json.dump(field_of(a.build_dir, a.sources), sys.stdout, indent=1, sort_keys=True)
    print()


if __name__ == "__main__":
    main()
