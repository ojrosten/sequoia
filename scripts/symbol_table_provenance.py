#!/usr/bin/env python3
"""Add to a record of symbol_table_report.py the facts its build directory
does not hold.

  symbol_table_provenance.py <record>

Writes the record to standard output, with
  - for each tool of the record's toolchain, the Debian package which owns
    the tool's executable, and the package's version; and
  - the GitHub Actions run which made the record: its id, its attempt, and
    the attempt's URL.

A distribution's archive can rebuild a compiler without changing what
`--version` prints, so the package's version tells two toolchains apart
where the record's version may not. The package is the one dpkg says owns
the executable which the record's path resolves to, which is the file the
build ran, rather than the package the job asked apt for. Where the path is
a symlink, the package owning the symlink can differ from the package owning
the executable: on ubuntu-26.04, g++-15 owns /usr/bin/g++-15, and
g++-15-x86-64-linux-gnu owns the executable /usr/bin/g++-15 resolves to.

The run is read from the variables GitHub Actions sets in every job:
GITHUB_SERVER_URL, GITHUB_REPOSITORY, GITHUB_RUN_ID and GITHUB_RUN_ATTEMPT.

The script refuses, rather than write a record which may be wrong, if
  - the record cannot be read as a JSON object, or has no toolchain, or a
    tool of its toolchain has no path, or already has a package, or the
    record already has a run;
  - dpkg-query cannot be run, or names no package owning a tool's path, or
    more than one, or gives no version for the package;
  - one of the variables is unset or empty, or the id or the attempt is not
    a number.
"""
import argparse, json, os, subprocess, sys


def run(command):
    """Returns the standard output of `command`, exiting if it fails."""
    try:
        r = subprocess.run(command, capture_output=True, text=True)
    except OSError as e:
        sys.exit(f"{command[0]} could not be run: {e}")
    if r.returncode:
        sys.exit(f"{' '.join(command)} failed with exit status {r.returncode}: {r.stderr.strip()}")
    return r.stdout


def owner_of(path):
    """Returns the name of the one package dpkg says owns `path`."""
    # Each line is "<packages>: <path>", the packages separated by ", ". A
    # package's name may carry its architecture after a colon, so the line is
    # split at the path it ends with, not at its first colon. A line
    # reporting a diversion ends with the path too, and names no owner.
    suffix = f": {path}"
    owners = [package
              for line in run(["dpkg-query", "--search", path]).splitlines()
              if line.endswith(suffix) and not line.startswith("diversion ")
              for package in line[:-len(suffix)].split(", ")]
    if len(owners) != 1:
        sys.exit(f"dpkg-query names {len(owners)} packages owning {path}, not one: {owners}")
    return owners[0]


def package_of(path):
    """Returns the name and version of the package owning the executable
    which `path` resolves to."""
    name = owner_of(os.path.realpath(path))
    version = run(["dpkg-query", "--show", "--showformat=${Version}", name]).strip()
    if not version:
        sys.exit(f"dpkg-query gives no version for {name}")
    return {"name": name, "version": version}


def environment_value(name):
    value = os.environ.get(name, "")
    if not value:
        sys.exit(f"{name} is unset or empty: the run is read from GitHub Actions' variables")
    return value


def environment_number(name):
    value = environment_value(name)
    if not value.isdecimal():
        sys.exit(f"{name} is {value!r}, not a number")
    return int(value)


def this_run():
    """Returns the id, the attempt and the attempt's URL of this run."""
    run_id, attempt = environment_number("GITHUB_RUN_ID"), environment_number("GITHUB_RUN_ATTEMPT")
    repository = f"{environment_value('GITHUB_SERVER_URL')}/{environment_value('GITHUB_REPOSITORY')}"
    url = f"{repository}/actions/runs/{run_id}/attempts/{attempt}"
    return {"id": run_id, "attempt": attempt, "url": url}


def read_record(path):
    try:
        with open(path, encoding="utf-8") as f:
            record = json.load(f)
    except (OSError, ValueError) as e:
        sys.exit(f"{path} cannot be read as JSON: {e}")
    if not isinstance(record, dict):
        sys.exit(f"{path} holds no JSON object")
    return record


def with_provenance(record, record_path):
    """Returns `record`, read from `record_path`, with its packages and run."""
    if "run" in record:
        sys.exit(f"{record_path} already has a run")
    tools = [tool for tool in record.get("toolchain", {}).values() if isinstance(tool, dict)]
    if not tools:
        sys.exit(f"{record_path} has no toolchain")
    for tool in tools:
        if "path" not in tool:
            sys.exit(f"{record_path} has a tool with no path: {tool}")
        if "package" in tool:
            sys.exit(f"{record_path} already has a package for {tool['path']}")
        tool["package"] = package_of(tool["path"])
    record["run"] = this_run()
    return record


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("record", help="a record written by symbol_table_report.py")
    a = ap.parse_args()
    json.dump(with_provenance(read_record(a.record), a.record), sys.stdout, indent=1, sort_keys=True)
    print()


if __name__ == "__main__":
    main()
