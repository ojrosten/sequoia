#!/usr/bin/env python3
"""Check that filtering a captured lcov tracefile removed files and changed nothing else.

  check_tracefile.py --capture <captured.info> --filtered <filtered.info>
                     --summary <lcov --summary output of filtered.info>
                     --removed <pattern>...

Every file of the capture must satisfy one of these:
  -# It is in the filtered tracefile, with its records identical, line for line.
  -# It matches one of the removal patterns, as lcov matches them, and is absent.
  -# It has no coverage points - no record but the per-file totals FNF, FNH, LF, LH, BRF, BRH,
     MCF and MCH - and is absent.
  -# It has function records but no line records - no record but FNL, FNA and the totals - and
     is absent. lcov deletes every file without line records when it reads a tracefile, and
     llvm-cov writes such files for functions it instruments without attributing lines to
     them. Oliver ruled (2026-09-26) that these files are accepted as removed, and listed on
     every run rather than kept, so that the loss stays visible and a newly dropped file shows.

And the figures `lcov --summary` gives for the filtered tracefile must be the counts of its
records: lines found and hit are the DA records and those with a non-zero count, functions
found and hit the FNA records and those with a non-zero count.

Either check failing means lcov changed the measurement while reading it, so the first
difference found is named and the exit status is 1.
"""
import argparse, re, sys

TOTAL_RECORDS = {'FNF', 'FNH', 'LF', 'LH', 'BRF', 'BRH', 'MCF', 'MCH'}


class Failure(Exception):
    pass


def removal_pattern(glob):
    """The expression lcov searches a path for, given one of its removal patterns.

    lcov turns `*` into any run of characters and `?` into any one, takes everything else
    literally, and searches the whole path rather than matching from its start: `/usr/*`
    removes `/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/stdio.h`.
    """
    return re.compile(''.join('.*' if c == '*' else '.' if c == '?' else re.escape(c) for c in glob))


def removed_by(source, patterns):
    return any(pattern.search(source) for pattern in patterns)


def read_tracefile(path):
    """Each source file's records, in the order the tracefile gives them."""
    records, source = {}, None
    with open(path) as tracefile:
        for number, line in enumerate(tracefile, 1):
            line = line.rstrip('\n')
            if line.startswith('SF:'):
                source = line[3:]
                if source in records:
                    raise Failure(f'{path}:{number}: {source} has a second record')
                records[source] = []
            elif line == 'end_of_record':
                source = None
            elif source is not None:
                records[source].append(line)
            elif line and not line.startswith('TN:'):
                raise Failure(f'{path}:{number}: "{line}" is outside any file\'s record')
    if source is not None:
        raise Failure(f'{path}: the record for {source} has no end_of_record')
    if not records:
        raise Failure(f'{path} has no file records')
    return records


FUNCTION_RECORDS = {'FNL', 'FNA'}


def record_tags(lines):
    return {line.split(':', 1)[0] for line in lines} - TOTAL_RECORDS


def has_coverage_points(lines):
    return bool(record_tags(lines))


def has_only_function_records(lines):
    tags = record_tags(lines)
    return bool(tags) and tags <= FUNCTION_RECORDS


def check_filtering(captured, filtered, patterns):
    """The number of files removed by a pattern, the number removed for having no coverage points,
    and the sorted list of those removed for having function records but no line records."""
    for source, lines in filtered.items():
        if source not in captured:
            raise Failure(f'{source} is in the filtered tracefile but not the capture')
        if removed_by(source, patterns):
            raise Failure(f'{source} matches a removal pattern but was kept')
        captured_lines = captured[source]
        for position in range(max(len(captured_lines), len(lines))):
            in_capture  = captured_lines[position] if position < len(captured_lines) else '(nothing)'
            in_filtered = lines[position]          if position < len(lines)          else '(nothing)'
            if in_capture != in_filtered:
                raise Failure(f'{source}: the capture has "{in_capture}" '
                              f'where the filtered tracefile has "{in_filtered}"')

    by_pattern, without_points, function_only = 0, 0, []
    for source, lines in captured.items():
        if source in filtered:
            continue
        if removed_by(source, patterns):
            by_pattern += 1
        elif not has_coverage_points(lines):
            without_points += 1
        elif has_only_function_records(lines):
            function_only.append(source)
        else:
            raise Failure(f'{source} has records besides function records, matches no removal pattern, '
                          f'and was dropped')
    return by_pattern, without_points, sorted(function_only)


def summary_figure(summary, kind):
    """The (hit, found) pair `lcov --summary` gives for `kind`, which is 'lines' or 'functions'."""
    match = re.search(rf'^[ \t]*{kind}\.+: .*\((\d+) of (\d+) ', summary, re.MULTILINE)
    if not match:
        raise Failure(f'the summary has no {kind} figure')
    return int(match.group(1)), int(match.group(2))


def record_figure(filtered, tag):
    """The (hit, found) pair counted from the `tag` records: found is all of them, hit those with a
    non-zero count, which is a record's second comma-separated field for both DA and FNA."""
    hit, found = 0, 0
    for lines in filtered.values():
        for line in lines:
            if line.startswith(tag + ':'):
                found += 1
                hit   += int(line.split(',')[1]) != 0
    return hit, found


def check_summary(summary, filtered):
    figures = {}
    for kind, tag in (('lines', 'DA'), ('functions', 'FNA')):
        reported = summary_figure(summary, kind)
        counted  = record_figure(filtered, tag)
        if reported != counted:
            raise Failure(f'lcov --summary reports {reported[0]} of {reported[1]} {kind} hit, but the filtered '
                          f'tracefile has {counted[0]} of {counted[1]} {tag} records with a non-zero count')
        figures[kind] = reported
    return figures


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--capture',  required=True)
    parser.add_argument('--filtered', required=True)
    parser.add_argument('--summary',  required=True)
    parser.add_argument('--removed',  required=True, nargs='+')
    arguments = parser.parse_args()
    try:
        captured = read_tracefile(arguments.capture)
        filtered = read_tracefile(arguments.filtered)
        with open(arguments.summary) as summary:
            figures = check_summary(summary.read(), filtered)
        patterns = [removal_pattern(glob) for glob in arguments.removed]
        by_pattern, without_points, function_only = check_filtering(captured, filtered, patterns)
    except (Failure, OSError) as error:
        print(f'error: {error}', file=sys.stderr)
        return 1
    print(f'{arguments.filtered}: {len(filtered)} of {len(captured)} captured files kept unchanged; '
          f'{by_pattern} removed by pattern, {without_points} with no coverage points. '
          f'lcov --summary agrees with the records: {figures["lines"][0]} of {figures["lines"][1]} lines, '
          f'{figures["functions"][0]} of {figures["functions"][1]} functions')
    print(f'Dropped by lcov for having function records but no line records: {len(function_only)} files')
    for source in function_only:
        print(f'  {source}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
