#!/usr/bin/env python3
"""Checks that the data branch holds a record of each commit named, with each
field named.

  check_metrics_records.py --data <ref> --directory <directory>
                             --field <path>... --commit <commit>...

The script runs in a clone of the repository holding <ref>, the data branch
which symbol-table.yml and baseline-tightener.yml store records on. The record
of <commit> is the file <directory>/<commit>.json at <ref>. Each <path> is a
field of a record, its keys joined by `.`, such as `executable.symbols`.

For each commit, the record must
  -# exist, and hold a JSON object;
  -# name the commit as its `commit`;
  -# say that its tree was clean, with a `clean` of true;
  -# hold, at each path, a whole number that is not a boolean.

The script prints a line for each commit: `ok: ` or `error: `, then the
commit, then what was found. It exits with status 1 if any line is an error,
and with status 0 otherwise. Arguments of any other form, and a <ref> which
does not name a commit, are refused with status 2.
"""
import argparse, json, subprocess, sys
from metrics_gate import value_at


def git(*arguments):
    """Returns the completed `git` command, its output as text."""
    return subprocess.run(["git", *arguments], capture_output=True, text=True)


def is_whole_number(value):
    """Whether `value` is a whole number, and not a boolean."""
    return isinstance(value, int) and not isinstance(value, bool)


def faults_of_record(text, commit, fields):
    """Returns what the record `text`, of `commit`, lacks."""
    try:
        record = json.loads(text)
    except json.JSONDecodeError as e:
        return [f"its record is not JSON: {e}"]
    if not isinstance(record, dict):
        return ["its record holds no JSON object"]
    return ([] if record.get("commit") == commit else [f"its record names the commit {record.get('commit')!r}"]) \
         + ([] if record.get("clean") is True else [f"its record has a clean of {record.get('clean')!r}, not true"]) \
         + [f"its record has no whole number at {field}" for field in fields
            if not is_whole_number(value_at(record, field.split(".")))]


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--data", required=True)
    parser.add_argument("--directory", required=True)
    parser.add_argument("--field", required=True, nargs="+")
    parser.add_argument("--commit", required=True, nargs="+")
    arguments = parser.parse_args()
    if git("rev-parse", "--verify", "--quiet", f"{arguments.data}^{{commit}}").returncode:
        print(f"{arguments.data} names no commit", file=sys.stderr)
        return 2
    failed = False
    for commit in arguments.commit:
        path = f"{arguments.directory}/{commit}.json"
        shown = git("show", f"{arguments.data}:{path}")
        faults = ([f"{arguments.data} holds no {path}"] if shown.returncode
                  else faults_of_record(shown.stdout, commit, arguments.field))
        failed = failed or bool(faults)
        print(f"error: {commit}: {'; '.join(faults)}" if faults
              else f"ok: {commit}: {path} holds {', '.join(arguments.field)}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
