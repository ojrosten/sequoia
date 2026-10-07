#!/usr/bin/env python3
"""Check that filtering an lcov capture removed files and changed nothing else.

  check_tracefile.py --capture <captured.info> --filtered <filtered.info>
                     --summary <lcov --summary output of filtered.info>
                     --removed <pattern>...

Each file of the filtered tracefile must be a file of the capture which no
removal pattern matches. Its records must be identical to its records in the
capture, byte for byte and in order.

Each file of the capture absent from the filtered tracefile must be one of
these:
  -# A file which a removal pattern matches, as lcov matches it, whatever its
     records.
  -# A file with no coverage points. It has no record but the per-file
     totals FNF, FNH, LF, LH, BRF, BRH, MCF and MCH.
  -# A file with function records but no line records. It has no record but
     FNL, FNA and the totals. lcov deletes every such file when it reads a
     tracefile.

The figures `lcov --summary` gives for the filtered tracefile must also be
counts of its records. Lines found are its DA records, and lines hit are
those with a non-zero count. Functions found are its FNA records, and
functions hit are those with a non-zero count.

If every file and both figures meet these rules, the first line of the output
names the filtered tracefile. It counts the files of the capture, then those
kept, those removed by a pattern, those with no coverage points and those
with function records but no line records, and gives the summary's figures.
The files with no coverage points are then listed, and then those with
function records but no line records. Each list is sorted, and has a heading
giving its count. No figure drawn from the filtered tracefile covers the
files listed.

The exit status is 1, and an error message names the first problem found,
if:
  -# a file or a figure breaks the rules above;
  -# a tracefile cannot be read, has no file's record, has two records for
     one file, has a record with no end_of_record, has a DA or FNA record
     whose count is not an integer, or has a line outside any file's record
     other than a test name or a blank line;
  -# the summary cannot be read, or has no figure for lines or for
     functions.

The exit status is 2, and a usage message is printed, if the command line is
not of the form above.
"""
import argparse, re, sys
from itertools import zip_longest

TOTAL_RECORDS    = {'FNF', 'FNH', 'LF', 'LH', 'BRF', 'BRH', 'MCF', 'MCH'}
FUNCTION_RECORDS = {'FNL', 'FNA'}
COUNTED_RECORDS  = ('DA:', 'FNA:')


class Failure(Exception):
    pass


def removal_pattern(glob):
    """The expression lcov searches a path for, given a removal pattern.

    lcov turns `*` into any run of characters and `?` into exactly one. It
    takes every other character literally, and distinguishes upper from
    lower case. It searches the whole path, rather than matching from the
    path's start, so `/usr/*` removes
    `/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/stdio.h`.
    """
    return re.compile(''.join('.*' if c == '*' else '.' if c == '?' else re.escape(c) for c in glob))


def removed_by(source, patterns):
    return any(pattern.search(source) for pattern in patterns)


def read_tracefile(path):
    """Each source file's records, in the order the tracefile gives them."""
    records, source = {}, None
    # Each byte round-trips, since a tracefile need not be UTF-8. lcov writes a
    # function name's letters from U+0080 to U+00FF as single Latin-1 bytes,
    # although gcov gives them as UTF-8.
    with open(path, encoding='utf-8', errors='surrogateescape', newline='') as tracefile:
        for number, line in enumerate(tracefile, 1):
            line = line.rstrip('\n')
            if source is None:
                if line.startswith('SF:'):
                    source = line[3:]
                    if source in records:
                        raise Failure(f'{path}:{number}: {source} has a second record')
                    records[source] = []
                elif line and not line.startswith('TN:'):
                    raise Failure(f'{path}:{number}: "{line}" is outside any file\'s record')
            elif line == 'end_of_record':
                source = None
            elif line.startswith('SF:'):
                raise Failure(f'{path}:{number}: the record for {source} has no end_of_record')
            else:
                if line.startswith(COUNTED_RECORDS) and not re.fullmatch(r'[^,]*,-?\d+(,.*)?', line):
                    raise Failure(f'{path}:{number}: "{line}" has no integer count')
                records[source].append(line)
    if source is not None:
        raise Failure(f'{path}: the record for {source} has no end_of_record')
    if not records:
        raise Failure(f'{path} has no file records')
    return records


def coverage_point_tags(lines):
    return {line.split(':', 1)[0] for line in lines} - TOTAL_RECORDS


def quoted(record):
    return '(nothing)' if record is None else f'"{record}"'


def check_filtering(captured, filtered, patterns):
    """The number of files removed by a pattern, the sorted list of those
    dropped for having no coverage points, and the sorted list of those
    dropped for having function records but no line records."""
    for source, lines in filtered.items():
        if source not in captured:
            raise Failure(f'{source} is in the filtered tracefile but not the capture')
        if removed_by(source, patterns):
            raise Failure(f'{source} matches a removal pattern but was kept')
        for in_capture, in_filtered in zip_longest(captured[source], lines):
            if in_capture != in_filtered:
                raise Failure(f'{source}: the capture has {quoted(in_capture)} '
                              f'where the filtered tracefile has {quoted(in_filtered)}')

    by_pattern, without_points, function_only = 0, [], []
    for source, lines in captured.items():
        if source in filtered:
            continue
        tags = coverage_point_tags(lines)
        if removed_by(source, patterns):
            by_pattern += 1
        elif not tags:
            without_points.append(source)
        elif tags <= FUNCTION_RECORDS:
            function_only.append(source)
        else:
            raise Failure(f'{source} has records besides function records, matches no removal pattern, '
                          f'and was dropped')
    return by_pattern, sorted(without_points), sorted(function_only)


def summary_figure(summary, kind):
    """The (hit, found) pair `lcov --summary` gives for `kind`, which is
    'lines' or 'functions'."""
    match = re.search(rf'^[ \t]*{kind}\.+: .*\((\d+) of (\d+) ', summary, re.MULTILINE)
    if not match:
        raise Failure(f'the summary has no {kind} figure')
    return int(match.group(1)), int(match.group(2))


def record_figure(filtered, tag):
    """The (hit, found) pair counted from the `tag` records. Found is all of
    them, and hit is those with a non-zero count. The count is a record's
    second comma-separated field, in both DA and FNA records."""
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
    # Write escaped bytes back as themselves, since a message quotes the
    # records it compared.
    for stream in (sys.stdout, sys.stderr):
        stream.reconfigure(encoding='utf-8', errors='surrogateescape')
    try:
        captured = read_tracefile(arguments.capture)
        filtered = read_tracefile(arguments.filtered)
        with open(arguments.summary, encoding='utf-8', errors='surrogateescape') as summary:
            figures = check_summary(summary.read(), filtered)
        patterns = [removal_pattern(glob) for glob in arguments.removed]
        by_pattern, without_points, function_only = check_filtering(captured, filtered, patterns)
    except (Failure, OSError) as error:
        print(f'error: {error}', file=sys.stderr)
        return 1
    print(f'{arguments.filtered}: {len(filtered)} of {len(captured)} captured files kept unchanged; '
          f'{by_pattern} removed by pattern, {len(without_points)} with no coverage points, '
          f'{len(function_only)} with function records but no line records. '
          f'lcov --summary agrees with the records: {figures["lines"][0]} of {figures["lines"][1]} lines, '
          f'{figures["functions"][0]} of {figures["functions"][1]} functions')
    for heading, sources in (('no coverage points',                   without_points),
                             ('function records but no line records', function_only)):
        print(f'Dropped by lcov for having {heading}: {len(sources)} files')
        for source in sources:
            print(f'  {source}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
