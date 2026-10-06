#!/usr/bin/env python3
"""The uncalled functions of a repository's Source tree, read from a tracefile.

  uncalled_functions.py baseline --tracefile <info> --repository <root>
                                 --demangler <tool>...
                                 --recorded-tool <tool>...
  uncalled_functions.py compare  --tracefile <info> --repository <root>
                                 --demangler <tool>...
                                 --recorded-tool <tool>...
                                 --baseline <file>

`baseline` prints, for each file within `<root>/Source`, the keys of its
uncalled functions. `compare` reads a baseline and exits with status 1 if the
tracefile has a key more often than the baseline does. A key the tracefile has
less often gives a notice, and does not change the status. The notice says
whether a called function in that file has the key, or none does; since
functions can share a key, the called one need not be the one listed. Either
mode exits with status 2 if it refuses its input.

A function is what lcov reports at one start line of one file. Every template
instantiation there is an alias of it, and the function is uncalled when no
alias was called. Its key is its demangled name, with these removed:
  -# every template argument list, at every level;
  -# the return type, abi tags, `[friend]` and any requires-clause;
  -# a parameter list, and its qualifiers, wherever a template argument list
     attached to a name comes before it, since the parameters are spelt with
     each instantiation's types;
  -# the discriminator of a lambda, which numbers the lambdas of one signature
     within a function in source order.

Several functions can share a key, so a baseline is a multiset: a key appears
once for each uncalled function it names.

Each demangler is tried in turn, and the first to demangle a name names it. A
baseline's header records the version of every demangler and of every
recorded tool, and `compare` refuses a baseline whose header differs.

The tracefile's counts must be gcov's. With check_data_consistency on, lcov
repairs counts whenever it reads a tracefile, and marks an uncalled lambda as
called if the lambda's first line ran.
"""
import argparse, re, subprocess, sys
from collections import Counter


class Refusal(Exception):
    pass


class Unkeyable(Exception):
    pass


# ---- The tracefile ----------------------------------------------------------

def read_tracefile(path, source_root):
    """{file: {start line: {mangled name: calls}}} for the files within
    `source_root`.

    Each file is given relative to the parent of `source_root`.
    """
    prefix                = source_root.rstrip('/') + '/'
    relative_from         = prefix[:prefix.rstrip('/').rfind('/') + 1]
    functions, current    = {}, None
    index_to_start        = {}
    with open(path, encoding='utf-8', errors='surrogateescape') as tracefile:
        for line in tracefile:
            tag, _, value = line.rstrip('\n').partition(':')
            if tag == 'SF':
                within_source  = value.startswith(prefix)
                current        = functions.setdefault(value[len(relative_from):], {}) if within_source else None
                index_to_start = {}
            elif current is None:
                continue
            elif tag == 'FNL':
                index, start, _ = value.split(',')
                index_to_start[index] = int(start)
                current.setdefault(int(start), {})
            elif tag == 'FNA':
                index, calls, name = value.split(',', 2)
                aliases       = current[index_to_start[index]]
                aliases[name] = aliases.get(name, 0) + int(calls)
    return {file: starts for file, starts in functions.items() if starts}


def demangle(names, demanglers):
    """{mangled: demangled}. A name no demangler changes maps to itself."""
    demangled, pending = {}, sorted(names)
    for demangler in demanglers:
        if not pending:
            break
        output = subprocess.run([demangler], input='\n'.join(pending) + '\n', capture_output=True, text=True,
                                errors='surrogateescape', check=True).stdout.split('\n')
        demangled.update((mangled, readable) for mangled, readable in zip(pending, output) if readable != mangled)
        pending = [mangled for mangled in pending if mangled not in demangled]
    demangled.update((mangled, mangled) for mangled in pending)
    return demangled


def version_of(tool):
    """The first line of `tool --version` which holds a version number."""
    output = subprocess.run([tool, '--version'], capture_output=True, text=True, check=True).stdout
    lines  = [' '.join(line.split()) for line in output.split('\n')]
    return next((line for line in lines if re.search(r'\d+\.\d+', line)), output.strip())


# ---- The key ----------------------------------------------------------------

OPERATORS = ['<=>', '<<=', '>>=', '->*', '()', '[]', '<<', '>>', '<=', '>=', '->', '==', '!=', '&&', '||', '++',
             '--', '+=', '-=', '*=', '/=', '%=', '&=', '|=', '^=', '<', '>', '+', '-', '*', '/', '%', '&', '|',
             '^', '~', '!', '=', ',']

# `operator` and the symbol after it form one token, so that the brackets of
# `operator()` or `operator<` are not read as brackets. So does `->`.
TOKEN  = re.compile(r'(?<!\w)operator(?:' + '|'.join(map(re.escape, OPERATORS)) + r')?(?!\w)|->|[<>(){}\[\]]')
CLOSER = {'(': ')', '[': ']', '{': '}', '<': '>'}


def cells(text):
    """(offset, depth, token, kind) for each token of `text`.

    The kind is 'open', 'close', 'operator' or 'char'.
    """
    marks, stack, offset = {match.start(): match.group() for match in TOKEN.finditer(text)}, [], 0
    while offset < len(text):
        token = marks.get(offset)
        if token in CLOSER:
            yield offset, len(stack), token, 'open'
            stack.append(token)
        elif token in CLOSER.values():
            if not stack or CLOSER[stack.pop()] != token:
                raise Unkeyable(f'unbalanced {token!r}: {text}')
            yield offset, len(stack), token, 'close'
        elif token:
            yield offset, len(stack), token, 'operator' if token != '->' else 'char'
        else:
            yield offset, len(stack), text[offset], 'char'
        offset += len(token) if token else 1
    if stack:
        raise Unkeyable(f'unclosed {stack}: {text}')


def strip_template_arguments(text):
    """`text` without its template argument lists, and the offsets at which the
    lists attached to a name were removed.

    A list attached to a name lies outside every bracket, so a list within a
    parameter list is not one.
    """
    kept, removed, stack, resume, length = [], [], [], 0, 0
    for match in TOKEN.finditer(text):
        token = match.group()
        if token in CLOSER:
            if token == '<' and '<' not in stack:
                kept.append(text[resume:match.start()])
                length += match.start() - resume
                if not stack:
                    removed.append(length)
            stack.append(token)
        elif token in CLOSER.values():
            if not stack or CLOSER[stack.pop()] != token:
                raise Unkeyable(f'unbalanced {token!r}: {text}')
            if token == '>' and '<' not in stack:
                resume = match.end()
    if stack:
        raise Unkeyable(f'unclosed {stack}: {text}')
    kept.append(text[resume:])
    return ''.join(kept), removed


def without_requires_clause(text):
    """`text` without any requires-clause llvm-cxxfilt printed after it.

    The angle brackets of a requires-clause need not balance.
    """
    at = text.find(' requires ')
    while at >= 0:
        try:
            list(cells(text[:at]))
            return text[:at]
        except Unkeyable:
            at = text.find(' requires ', at + 1)
    return text


QUALIFIERS = re.compile(r'((?:\s*(?:const|volatile|&&|&|noexcept))*)\s*$')
LAMBDA     = re.compile(r"^(?:\{lambda\((.*)\)#(\d+)\}|'lambda(\d*)'\((.*)\))$")
UNNAMED    = re.compile(r"^(?:\{unnamed type#(\d+)\}|'unnamed(\d*)')$")
CALLABLE   = re.compile(r'(.*?)\((.*)\)((?: const| volatile| &&| &)*)$')


def operator_length(component):
    """The length of the operator name `component` begins with, or 0."""
    match = TOKEN.match(component)
    return len(match.group()) if match and match.group().startswith('operator') else 0


def split_name(text):
    """(components, parameters, qualifiers) of a demangled name, without its
    return type.

    Each component is (its text, the offset at which it begins).
    """
    all_cells  = list(cells(text))
    parameters = None
    closing    = None
    # The parameter list is the last list outside every bracket which only
    # qualifiers follow
    for offset, depth, token, kind in reversed(all_cells):
        if depth == 0 and kind == 'close' and token == ')' and QUALIFIERS.fullmatch(text[offset + 1:]):
            closing = offset
            continue
        if closing is not None and depth == 0 and kind == 'open' and token == '(':
            parameters = text[offset + 1:closing]
            qualifiers = ' '.join(text[closing + 1:].split())
            head       = text[:offset]
            break
    if parameters is None:
        raise Unkeyable(f'no parameter list: {text}')

    # The name begins after the last space outside every bracket, unless a
    # qualifier follows that space
    start = 0
    for offset, depth, token, kind in all_cells:
        if offset >= len(head) or (depth == 0 and kind == 'operator'):
            break
        if depth == 0 and token == ' ' and not re.match(r'(const|volatile|&&?)(\s|::)', head[offset + 1:]):
            start = offset + 1

    # A conversion operator's type is part of its name, `::` included
    components, begin, conversion = [], start, False
    for offset, depth, token, kind in all_cells:
        if offset < start or offset >= len(head) or depth:
            continue
        if kind == 'operator' and token == 'operator':
            conversion = True
        if kind == 'open' and token == '(':
            conversion = False
        if not conversion and head.startswith('::', offset) and offset >= begin:
            components.append((head[begin:offset], begin))
            begin = offset + 2
    components.append((head[begin:], begin))
    return components, parameters, qualifiers


def key(demangled):
    """The key of the function named `demangled`."""
    text          = re.sub(r'\[abi:[^\]]*\]|\[friend\]|(?<=::)friend ', '', demangled)
    text          = re.sub(r'( \[clone [^\]]*\]| \(\.[\w.]+\))+$', '', without_requires_clause(text))
    text, removed = strip_template_arguments(text)
    components, parameters, qualifiers = split_name(text)
    # The return type's lists say nothing of the name
    removed = [at for at in removed if at >= components[0][1]]

    def signature(parameters, qualifiers, opening):
        if any(at <= opening for at in removed) or parameters.startswith('this '):
            return ''
        return '(' + parameters + ')' + (' ' + qualifiers if qualifiers else '')

    parts = []
    for component, begin in components:
        begin    += len(component) - len(component.lstrip())
        component = component.strip()
        if lambda_match := LAMBDA.match(component):
            gnu_parameters, gnu_discriminator, _, llvm_parameters = lambda_match.groups()
            lambda_parameters = gnu_parameters if gnu_discriminator else llvm_parameters
            parts.append('{lambda' + signature(lambda_parameters, '', begin + len('{lambda')) + '}')
        elif UNNAMED.match(component):
            parts.append('{unnamed type}')
        # A callable has a name before its parameter list; `(anonymous
        # namespace)` has none
        elif (local := CALLABLE.match(component, operator_length(component))) and local.end(1):
            name = component[:local.end(1)]
            parts.append(name + signature(local.group(2), local.group(3).strip(), begin + len(name)))
        else:
            parts.append(component)
    last_name, last_begin = components[-1]
    return '::'.join(parts) + signature(parameters, qualifiers, last_begin + len(last_name))


# ---- The baseline -----------------------------------------------------------

def keys_by_file(functions, demanglers, uncalled):
    """{file: {key: [start line]}} of the uncalled functions if `uncalled` is
    true, and of the called functions otherwise.

    A function is keyed by every key its aliases give, since one start line can
    hold several functions. A called function none of whose aliases can be
    keyed is left out. An uncalled one raises `Unkeyable`.
    """
    def selected(aliases):
        return not any(aliases.values()) if uncalled else any(aliases.values())

    names     = {name for starts in functions.values() for aliases in starts.values() if selected(aliases)
                      for name in aliases}
    demangled = demangle(names, demanglers)
    result    = {}
    for file, starts in sorted(functions.items()):
        for start, aliases in sorted(starts.items()):
            if not selected(aliases):
                continue
            keys = set()
            for name in aliases:
                try:
                    keys.add(key(demangled[name]))
                except Unkeyable:
                    pass
            if not keys and uncalled:
                raise Unkeyable(f'{file}:{start}: no alias of this uncalled function can be keyed, '
                                f'e.g. {sorted(aliases)[0]}')
            for function_key in keys:
                result.setdefault(file, {}).setdefault(function_key, []).append(start)
    return result


def counts(keys):
    """{file: Counter(key)}: a key counts once for each of its start lines."""
    return {file: Counter({function_key: len(starts) for function_key, starts in by_key.items()})
            for file, by_key in keys.items()}


def header(demanglers, recorded_tools):
    return ([f'demangler: {version_of(tool)}' for tool in demanglers]
            + [f'tool: {version_of(tool)}' for tool in recorded_tools])


def format_baseline(header_lines, uncalled):
    lines = ['# ' + line for line in header_lines]
    for file, keys in sorted(uncalled.items()):
        lines += [file] + ['    ' + key for key in sorted(keys.elements())]
    return '\n'.join(lines) + '\n'


def read_baseline(path):
    """(header lines, {file: Counter(key)})."""
    header_lines, uncalled, file = [], {}, None
    with open(path, encoding='utf-8', errors='surrogateescape') as baseline:
        for line in baseline:
            if line.startswith('# '):
                header_lines.append(line[2:].strip())
            elif line.startswith('    '):
                if file is None:
                    raise Refusal(f'{path}: a key precedes the first file: {line.strip()}')
                uncalled[file][line.strip()] += 1
            elif line.strip():
                file = line.strip()
                uncalled.setdefault(file, Counter())
    return header_lines, uncalled


def compare(baseline, current):
    """(risen, fallen): a (file, key) pair for each unit by which a key's count
    rose or fell."""
    risen, fallen = [], []
    for file in sorted(set(baseline) | set(current)):
        was, now = baseline.get(file, Counter()), current.get(file, Counter())
        risen  += [(file, key) for key in sorted((now - was).elements())]
        fallen += [(file, key) for key in sorted((was - now).elements())]
    return risen, fallen


# ---- The command line -------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    modes  = parser.add_subparsers(dest='mode', required=True)
    for mode in ('baseline', 'compare'):
        options = modes.add_parser(mode)
        options.add_argument('--tracefile',     required=True)
        options.add_argument('--repository',    required=True)
        options.add_argument('--demangler',     required=True, action='append')
        options.add_argument('--recorded-tool', required=True, action='append')
        if mode == 'compare':
            options.add_argument('--baseline',  required=True)
    arguments = parser.parse_args()

    try:
        functions = read_tracefile(arguments.tracefile, arguments.repository.rstrip('/') + '/Source')
        if not functions:
            raise Refusal(f'{arguments.tracefile} has no function records within {arguments.repository}/Source')
        current_header = header(arguments.demangler, arguments.recorded_tool)
        uncalled       = keys_by_file(functions, arguments.demangler, uncalled=True)
        if arguments.mode == 'baseline':
            sys.stdout.write(format_baseline(current_header, counts(uncalled)))
            return 0

        baseline_header, baseline = read_baseline(arguments.baseline)
        if baseline_header != current_header:
            raise Refusal(f'{arguments.baseline} was made with {baseline_header}, and this run has '
                          f'{current_header}: regenerate the baseline')
        risen, fallen = compare(baseline, counts(uncalled))
        called        = keys_by_file({file: functions[file] for file, _ in fallen if file in functions},
                                     arguments.demangler, uncalled=False)
        for file, fallen_key in fallen:
            if fallen_key in called.get(file, Counter()):
                print(f'notice: now called, so it can leave the baseline: {file}: {fallen_key}')
            else:
                print(f'notice: no longer present, so it can leave the baseline: {file}: {fallen_key}')
        for file, risen_key in risen:
            print(f'error: uncalled, and not in the baseline: {file}: {risen_key} '
                  f'(lines {uncalled[file][risen_key]})')
        return 1 if risen else 0
    except (Refusal, Unkeyable, OSError, subprocess.CalledProcessError) as error:
        print(f'error: {error}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    # Write escaped bytes back as themselves, since a key or a path may hold
    # bytes which are not UTF-8
    for stream in (sys.stdout, sys.stderr):
        stream.reconfigure(encoding='utf-8', errors='surrogateescape')
    sys.exit(main())
