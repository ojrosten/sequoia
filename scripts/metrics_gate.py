#!/usr/bin/env python3
"""Judges symbol-table records, as symbol-table.yml writes them, against the
gates on metrics.

  metrics_gate.py against-baseline <measured record> <baseline record>
                                   <acceptances>
  metrics_gate.py leftover-materials <record>

`against-baseline` compares the measured record with the baseline's, series
by series. A series is one count of a record:
  -# `symbols`, the executable's symbols, and `symbol_name_bytes`, the total
     length of their names. A rise of more than 1 in 200 of the baseline's
     count fails;
  -# `prune_dependency_analysis`, the instructions of prune's analysis. A rise
     of more than 1 in 20 of the baseline's count fails.
A fall never fails. A count which either record lacks fails. A failure of
either symbol series also lists the objects whose counts of defined symbols
moved, the 20 largest moves first.

The two records must agree in what was measured, and how: their toolchain,
their build's cache, their target and, where both count prune's analysis,
the functions counted. A difference in any of these is a break in the
series: no series is compared, and the break fails. A record made from a tree
which was not clean fails too.

<acceptances> is a text, such as a pull request's description, in which the
gate looks for lines accepting what would otherwise fail. A line is an
acceptance if, without its surrounding white space, it starts with
`metrics-gate:`. It must be one of:
  -# `metrics-gate: accept <series> +<count>`: the series' threshold applies
     to its rise less <count>. <count> is a positive whole number with no
     leading zero, in the series' own unit;
  -# `metrics-gate: accept a break in the series`: a break passes, and no
     series is compared.
Any other line which starts `metrics-gate:` fails, as does a second
acceptance of one series or of a break. A failure prints the line which would
accept it.

`leftover-materials` checks the record's `leftover_materials_setup`: the
instructions which the run with leftover materials executed beyond the run
without them, for each leftover entry. More than 400 per entry fails. So does
a record which lacks the field, or whose run with leftovers found none.

Each mode prints its verdict on standard output, one finding per line, each an
`error: ` or a `notice: ` followed by the finding. It exits with status 1 if
any finding is an error, and with status 0 otherwise. A file which cannot be
read, a record which is not a JSON object, and arguments of any other form
are refused with status 2.
"""
import json, sys
from typing import NamedTuple

# Each series is (name, path into a record, denominator): a rise of more than
# the baseline's count over the denominator fails.
SERIES = (
    ("symbols",                   ("executable", "symbols"),                    200),
    ("symbol_name_bytes",         ("executable", "symbol_name_bytes"),          200),
    ("prune_dependency_analysis", ("prune_dependency_analysis", "instructions"), 20),
)

# The fields of a record which say what was measured, and how. Two records
# whose counts are compared agree in each.
MEASUREMENT_CONDITIONS = (("toolchain",), ("cache",), ("target",))

# The fields which say what a section measured, compared only where both
# records hold them, since a record may lack the section.
SECTION_CONDITIONS = (("prune_dependency_analysis", "function"),)

# The instructions per leftover entry above which leftover-materials fails.
LEFTOVER_COST_LIMIT_PER_ENTRY = 400

# The number of moves that a failure of a symbol series lists, at most.
MOVERS_LISTED_AT_MOST = 20

ACCEPTANCE_PREFIX     = "metrics-gate:"
BREAK_ACCEPTANCE_LINE = f"{ACCEPTANCE_PREFIX} accept a break in the series"


class Refusal(Exception):
    """An input the script cannot judge."""


class Acceptances(NamedTuple):
    """What the acceptance lines of a text accept, and the errors of those
    which accept nothing. A rise is keyed by its series' name."""
    accepted_rises: dict[str, int]
    accepts_break: bool
    errors: list[str]


def acceptance_line(name, count):
    """The line which accepts a rise of `count` in the series `name`."""
    return f"{ACCEPTANCE_PREFIX} accept {name} +{count}"


def read_record(path):
    """Returns the record in the file at `path`."""
    try:
        with open(path, encoding="utf-8") as f:
            record = json.load(f)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as e:
        raise Refusal(f"{path} cannot be read as JSON: {e}")
    if not isinstance(record, dict):
        raise Refusal(f"{path} holds no JSON object")
    return record


def value_at(record, path):
    """Returns the value at `path`, a sequence of keys, in `record`, or None if
    there is none."""
    value = record
    for key in path:
        value = value.get(key) if isinstance(value, dict) else None
    return value


def count_at(record, path):
    """Returns the count at `path` in `record`, or None if there is none.

    A count is a positive whole number, and not a boolean.
    """
    value = value_at(record, path)
    return value if isinstance(value, int) and not isinstance(value, bool) and value > 0 else None


def read_acceptances(path):
    """Returns the acceptances of the text at `path`."""
    try:
        with open(path, encoding="utf-8") as f:
            lines = f.read().splitlines()
    except (OSError, UnicodeDecodeError) as e:
        raise Refusal(f"{path} cannot be read: {e}")
    names = [name for name, _, _ in SERIES]
    accepted_rises, accepts_break, errors = {}, False, []
    for line in (line.strip() for line in lines):
        if not line.startswith(ACCEPTANCE_PREFIX):
            continue
        words = line.removeprefix(ACCEPTANCE_PREFIX).split(" ")
        if line == BREAK_ACCEPTANCE_LINE:
            if accepts_break:
                errors.append("a break in the series is accepted twice")
            accepts_break = True
        elif (len(words) != 4 or words[:2] != ["", "accept"] or words[2] not in names
                or not words[3].startswith("+") or not words[3][1:].isdecimal()
                or not words[3][1:].isascii() or words[3][1] == "0"):
            errors.append(f"the line {line!r} is not an acceptance this gate knows: an acceptance is "
                          f"'{acceptance_line('<series>', '<count>')}' or '{BREAK_ACCEPTANCE_LINE}'")
        elif words[2] in accepted_rises:
            errors.append(f"{words[2]} is accepted twice")
        else:
            accepted_rises[words[2]] = int(words[3][1:])
    return Acceptances(accepted_rises, accepts_break, errors)


def differences(measured, baseline, path):
    """Returns the paths, each a tuple of keys starting with `path`, at which
    the two values differ."""
    if isinstance(measured, dict) and isinstance(baseline, dict):
        return [difference for key in sorted(measured.keys() | baseline.keys())
                for difference in differences(measured.get(key), baseline.get(key), path + (key,))]
    return [] if measured == baseline else [path]


def is_error(finding):
    """Whether `finding` is an error."""
    return finding.startswith("error: ")


def findings_of_movers(measured, baseline):
    """Returns findings naming the objects whose counts of defined symbols
    moved between the two records, largest move first, with a finding
    counting them."""
    held  = baseline.get("defined_symbols_by_object") or {}
    found = measured.get("defined_symbols_by_object") or {}
    moves = sorted(((found.get(source, 0) - held.get(source, 0), source)
                    for source in held.keys() | found.keys()
                    if found.get(source) != held.get(source)),
                   key=lambda move: (-abs(move[0]), move[1]))
    added   = sum(source not in held for source in found)
    removed = sum(source not in found for source in held)
    listed  = moves[:MOVERS_LISTED_AT_MOST]
    return ([f"notice: {len(moves)} of {len(held.keys() | found.keys())} objects moved "
             f"({added} added, {removed} removed); the {len(listed)} largest moves:"]
            + [f"notice:   {move:+d} {source}" for move, source in listed])


def findings_of_series(name, path, denominator, measured, baseline, accepted):
    """Returns the findings of one series."""
    held, found = count_at(baseline, path), count_at(measured, path)
    uncounted = [f"error: {name}: the {role} record, of {record.get('commit')}, has no count: its "
                 "measurement failed, so re-run all the jobs"
                 for role, record, count in (("baseline's", baseline, held), ("measured", measured, found))
                 if count is None]
    if uncounted:
        return uncounted
    rise = found - held
    summary = (f"{name}: {held} -> {found}, {rise:+d} ({rise / held:+.2%}), "
               f"allowed +{held // denominator} (1 in {denominator} of the baseline)"
               + (f" and +{accepted} accepted" if accepted else ""))
    if rise <= held // denominator + accepted:
        return [f"notice: {summary}"]
    return [f"error: {summary}",
            f"error: {name}: to accept this rise, add this line to the pull request's description, "
            "then re-run the failed jobs:",
            f"error:   {acceptance_line(name, rise)}"]


def findings_of_all_series(measured, baseline, acceptances):
    """Returns the findings of every series, and the movers if a symbol series
    failed."""
    judged = {name: findings_of_series(name, path, denominator, measured, baseline,
                                       acceptances.accepted_rises.get(name, 0))
              for name, path, denominator in SERIES}
    symbols_failed = any(map(is_error, (finding for name, path, _ in SERIES if path[0] == "executable"
                                        for finding in judged[name])))
    return ([finding for findings in judged.values() for finding in findings]
            + (findings_of_movers(measured, baseline) if symbols_failed else []))


def findings_of_break(broken, measured, baseline, accepts_break):
    """Returns the findings of a break in the series, at the paths `broken`."""
    kind = "notice" if accepts_break else "error"
    return ([f"{kind}: a break in the series: the records differ in "
             f"{', '.join('.'.join(path) for path in broken)}, so no series is compared"]
            + [f"{kind}:   {'.'.join(path)}: baseline {value_at(baseline, path)!r}, "
               f"measured {value_at(measured, path)!r}" for path in broken]
            + ([] if accepts_break else
               # Records which the same scripts made from trees which agree
               # in these fields differ only if the two jobs' machines or
               # packages did. A re-run of the failed jobs alone would
               # compare the same two records again.
               ["error: if the pull request changes none of these fields, the two jobs' machines or "
                "packages differed: re-run all the jobs, rather than accept the break",
                "error: to accept the break, add this line to the pull request's description, "
                "then re-run the failed jobs:", f"error:   {BREAK_ACCEPTANCE_LINE}"]))


def judge_against_baseline(measured_path, baseline_path, acceptances_path):
    """Prints the verdict of `against-baseline`, and returns its status."""
    measured, baseline = read_record(measured_path), read_record(baseline_path)
    acceptances = read_acceptances(acceptances_path)
    broken = ([difference for path in MEASUREMENT_CONDITIONS
               for difference in differences(value_at(measured, path), value_at(baseline, path), path)]
              + [path for path in SECTION_CONDITIONS
                 if None not in (value_at(measured, path), value_at(baseline, path))
                 and value_at(measured, path) != value_at(baseline, path)])
    unclean = [f"error: the {role} record, of {record.get('commit')}, was made from a tree which was not "
               "clean, so its counts are not its commit's"
               for role, record in (("measured", measured), ("baseline's", baseline))
               if record.get("clean") is not True]
    findings = ([f"error: {e}" for e in acceptances.errors]
                + [f"notice: the measured record is of {measured.get('commit')}, "
                   f"the baseline's of {baseline.get('commit')}"]
                + unclean
                + (findings_of_break(broken, measured, baseline, acceptances.accepts_break) if broken
                   else findings_of_all_series(measured, baseline, acceptances)))
    print("\n".join(findings))
    return 1 if any(map(is_error, findings)) else 0


def judge_leftover_materials(record_path):
    """Prints the verdict of `leftover-materials`, and returns its status."""
    record = read_record(record_path)
    field = ("leftover_materials_setup",)
    if value_at(record, field + ("leftover_entries",)) == 0:
        print("error: the run with leftover materials found no leftover entries, "
              "so the cost of clearing them was not measured")
        return 1
    entries = count_at(record, field + ("leftover_entries",))
    without = count_at(record, field + ("without_leftovers", "instructions"))
    with_   = count_at(record, field + ("with_leftovers", "instructions"))
    if None in (entries, without, with_):
        print(f"error: the record of {record.get('commit')} has no count of the setup cost of "
              "leftover materials, so it cannot be checked")
        return 1
    cost = with_ - without
    summary = (f"leftover materials cost the runner's setup {cost} instructions for "
               f"{entries} entries, {cost / entries:.1f} per entry, on "
               f"{record['leftover_materials_setup'].get('cpu_model', 'an unrecorded CPU')}; "
               f"the limit is {LEFTOVER_COST_LIMIT_PER_ENTRY} per entry")
    exceeded = cost > LEFTOVER_COST_LIMIT_PER_ENTRY * entries
    print(f"{'error' if exceeded else 'notice'}: {summary}")
    return 1 if exceeded else 0


def main():
    usage = __doc__.split("\n\n")[1]
    arguments = sys.argv[1:]
    modes = {"against-baseline":   (judge_against_baseline, 3),
             "leftover-materials": (judge_leftover_materials, 1)}
    if not arguments or arguments[0] not in modes or len(arguments) != modes[arguments[0]][1] + 1:
        print(f"Usage:\n{usage}", file=sys.stderr)
        return 2
    mode, _ = modes[arguments[0]]
    try:
        return mode(*arguments[1:])
    except Refusal as e:
        print(e, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
