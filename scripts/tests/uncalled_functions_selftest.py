#!/usr/bin/env python3
"""Controls for uncalled_functions.py, on synthetic tracefiles, and a mutation
check of the controls.

  uncalled_functions_selftest.py --demangler <tool>... [--mutations]

Without --mutations, the selftest runs the controls. With it, the selftest runs
the controls against the script and against each mutant of the script. The
script must fail no control, and each mutant at least one.

The demanglers are given in the order the script is to try them. The expected
keys are spelt as llvm-cxxfilt, then GNU c++filt, spell them. The mangled names
are g++-16's. gcc emits the C2 and D2 variants of a constructor and destructor
in sequoia's own tracefiles, and they are the C1 and D1 names with the variant
letter changed.
"""
import argparse, contextlib, io, os, sys, tempfile, types, unittest
from collections import Counter

HERE   = os.path.dirname(os.path.abspath(__file__))
SCRIPT = os.path.join(HERE, '..', 'uncalled_functions.py')
with open(SCRIPT, encoding='utf-8') as script:
    SOURCE = script.read()

DEMANGLERS = []
U          = None


def load(source):
    module = types.ModuleType('uncalled_functions')
    exec(compile(source, 'uncalled_functions.py', 'exec'), module.__dict__)
    return module


ROOT = '/home/runner/work/sequoia/sequoia'
F    = ROOT + '/Source/sequoia/ns.hpp'

# int ns::twice<int>(int) and double ns::twice<double>(double)
TWICE_INT       = '_ZN2ns5twiceIiEET_S1_'
TWICE_DOUBLE    = '_ZN2ns5twiceIdEET_S1_'
CONVERT_STRING  = '_ZN2ns7convertERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE'
CONVERT_VIEW    = '_ZN2ns7convertESt17basic_string_viewIcSt11char_traitsIcEE'
A_ROOT, B_ROOT  = '_ZNK2ns1a12project_rootEv', '_ZNK2ns1b12project_rootEv'
BOX_C1, BOX_C2  = '_ZN2ns3boxIiEC1Ei', '_ZN2ns3boxIiEC2Ei'
BOX_D1, BOX_D2  = '_ZN2ns3boxIiED1Ev', '_ZN2ns3boxIiED2Ev'
POLY_D0, POLY_D1, POLY_D2 = '_ZN2ns4polyD0Ev', '_ZN2ns4polyD1Ev', '_ZN2ns4polyD2Ev'
SET_INT, SET_DOUBLE       = '_ZN2ns3boxIiE3setEi', '_ZN2ns3boxIdE3setEd'
# box<int>::set(int, int): another overload of set
SET2_INT        = '_ZN2ns3boxIiE3setEii'
# ns::(anonymous namespace)::hidden(int)
HIDDEN          = '_ZN2ns12_GLOBAL__N_1L6hiddenEi'
LAMBDA_INT_1    = '_ZZN2ns12with_lambdasEiENKUliE_clEi'
LAMBDA_INT_2    = '_ZZN2ns12with_lambdasEiENKUliE0_clEi'
LAMBDA_INT_3    = '_ZZN2ns12with_lambdasEiENKUliE1_clEi'
LAMBDA_DOUBLE   = '_ZZN2ns12with_lambdasEiENKUldE_clEd'
# ns::num::operator<(ns::num const&) const
LESS            = '_ZNK2ns3numltERKS0_'
# A conversion to std::string, which llvm-cxxfilt cannot demangle
TO_STRING       = '_ZNK2ns3numcvNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEEv'
# ns::num::tag[abi:cxx11]() const
TAG             = '_ZNK2ns3num3tagB5cxx11Ev'
# Two overloads differing only in their constraints, which GNU c++filt cannot
# demangle
COND_INT        = '_ZN2ns4condIiE1fEvQ8integralIT_E'
COND_DOUBLE     = '_ZN2ns4condIdE1fEvQnt8integralIT_E'


def written(text, suffix):
    handle = tempfile.NamedTemporaryFile('w', suffix=suffix, delete=False, encoding='utf-8')
    handle.write(text)
    handle.close()
    return handle.name


def tracefile(functions, path):
    """An lcov 2.5 tracefile of one source file.

    `functions` is [(start line, [(mangled name, calls)])].
    """
    lines = ['TN:', 'SF:' + path]
    for index, (start, aliases) in enumerate(functions):
        lines.append(f'FNL:{index},{start},{start + 3}')
        lines += [f'FNA:{index},{calls},{name}' for name, calls in aliases]
    lines.append('end_of_record')
    return written('\n'.join(lines) + '\n', '.info')


def uncalled_keys(functions):
    found = U.keys_by_file(U.read_tracefile(tracefile(functions, F), ROOT + '/Source'), DEMANGLERS, uncalled=True)
    return U.counts(found).get('Source/sequoia/ns.hpp', Counter())


def run(arguments):
    """(exit status, standard output, standard error) of the script's main,
    given `arguments`."""
    out, err = io.StringIO(), io.StringIO()
    saved    = sys.argv
    sys.argv = ['uncalled_functions.py'] + arguments
    try:
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            status = U.main()
    finally:
        sys.argv = saved
    return status, out.getvalue(), err.getvalue()


def tools():
    return ([argument for demangler in DEMANGLERS for argument in ('--demangler', demangler)]
            + ['--recorded-tool', sys.executable])


class Keys(unittest.TestCase):
    def test_instantiations_collapse_into_one_function(self):
        self.assertEqual(uncalled_keys([(3, [(TWICE_INT, 0), (TWICE_DOUBLE, 0)])]), Counter({'ns::twice': 1}))

    def test_one_called_instantiation_calls_the_function(self):
        self.assertEqual(uncalled_keys([(3, [(TWICE_INT, 0), (TWICE_DOUBLE, 5)])]), Counter())

    def test_overloads_keep_their_parameters(self):
        self.assertEqual(uncalled_keys([(10, [(CONVERT_STRING, 0)]), (20, [(CONVERT_VIEW, 0)])]),
                         Counter({'ns::convert(std::__cxx11::basic_string const&)': 1,
                                  'ns::convert(std::basic_string_view)':            1}))

    def test_same_name_in_two_classes(self):
        self.assertEqual(uncalled_keys([(10, [(A_ROOT, 0)]), (20, [(B_ROOT, 0)])]),
                         Counter({'ns::a::project_root() const': 1, 'ns::b::project_root() const': 1}))

    def test_constructor_and_destructor_variants_collapse(self):
        self.assertEqual(uncalled_keys([(10, [(BOX_C1, 0), (BOX_C2, 0)]),
                                        (12, [(BOX_D1, 0), (BOX_D2, 0)]),
                                        (30, [(POLY_D0, 0), (POLY_D1, 0), (POLY_D2, 0)])]),
                         Counter({'ns::box::box': 1, 'ns::box::~box': 1, 'ns::poly::~poly()': 1}))

    def test_member_of_class_template_collapses_across_instantiations(self):
        self.assertEqual(uncalled_keys([(10, [(SET_INT, 0), (SET_DOUBLE, 0)])]), Counter({'ns::box::set': 1}))

    def test_anonymous_namespace_is_part_of_the_name(self):
        self.assertEqual(uncalled_keys([(10, [(HIDDEN, 0)])]),
                         Counter({'ns::(anonymous namespace)::hidden(int)': 1}))

    def test_lambdas_are_named_by_signature_not_discriminator(self):
        self.assertEqual(uncalled_keys([(20, [(LAMBDA_INT_2, 0)]), (24, [(LAMBDA_DOUBLE, 0)])]),
                         Counter({'ns::with_lambdas(int)::{lambda(int)}::operator()(int) const':          1,
                                  'ns::with_lambdas(int)::{lambda(double)}::operator()(double) const': 1}))

    def test_operators_and_abi_tags(self):
        self.assertEqual(uncalled_keys([(10, [(LESS, 0)]), (12, [(TO_STRING, 0)]), (14, [(TAG, 0)])]),
                         Counter({'ns::num::operator<(ns::num const&) const':   1,
                                  'ns::num::operator std::__cxx11::basic_string': 1,
                                  'ns::num::tag() const':                        1}))

    def test_overloads_differing_only_in_constraints_are_counted(self):
        self.assertEqual(uncalled_keys([(10, [(COND_INT, 0)]), (11, [(COND_DOUBLE, 0)])]), Counter({'ns::cond::f': 2}))

    def test_template_overloads_are_counted(self):
        self.assertEqual(uncalled_keys([(10, [(SET_INT, 0)]), (11, [(SET2_INT, 0)])]), Counter({'ns::box::set': 2}))

    def test_no_line_numbers(self):
        self.assertEqual(uncalled_keys([(10,  [(CONVERT_STRING, 0)]), (20,  [(LAMBDA_INT_1, 0)])]),
                         uncalled_keys([(110, [(CONVERT_STRING, 0)]), (220, [(LAMBDA_INT_1, 0)])]))

    def test_a_template_in_an_anonymous_namespace(self):
        self.assertEqual(U.key('std::vector<int, std::allocator<int> > ns::(anonymous namespace)::make<int>(int)'),
                         'ns::(anonymous namespace)::make')

    def test_a_return_type_does_not_make_the_enclosing_function_a_template(self):
        """A generic lambda's operator() returns the type its instantiation
        gives, and the enclosing g(int) is not a template."""
        generic = 'int ns::g(int)::{lambda(auto:1)#1}::operator()<int>(int) const'
        self.assertEqual(U.key('std::vector<int, std::allocator<int> > ns::g(int)::{lambda(auto:1)#1}::'
                               'operator()<std::vector<int> >(std::vector<int>) const'),
                         U.key(generic))
        self.assertEqual(U.key(generic), 'ns::g(int)::{lambda(auto:1)}::operator()')

    def test_an_operator_as_an_enclosing_scope(self):
        self.assertEqual(U.key('ns::x::operator()(int) const::{lambda()#1}::operator()() const'),
                         'ns::x::operator()(int) const::{lambda()}::operator()() const')
        self.assertEqual(U.key('ns::t<int>::operator()(int) const::{lambda()#1}::operator()() const'),
                         'ns::t::operator()::{lambda}::operator()')

    def test_hidden_friends_of_a_class_template_collapse(self):
        """A friend defined in a class template is a non-template function
        per instantiation. Only its parameter types say which, so the lists
        within a parameter list go too."""
        self.assertEqual(U.key('sequoia::operator<=>(sequoia::mem_ordered_tuple<int, int> const&, '
                               'sequoia::mem_ordered_tuple<int, int> const&)'),
                         U.key('sequoia::operator<=>(sequoia::mem_ordered_tuple<int> const&, '
                               'sequoia::mem_ordered_tuple<int> const&)'))

    def test_files_outside_the_source_tree_are_ignored(self):
        for path in ('/r/Tests/ns.cpp', ROOT + '/SourceExtra/ns.hpp', '/elsewhere' + F):
            with self.subTest(path=path):
                self.assertEqual(U.read_tracefile(tracefile([(10, [(HIDDEN, 0)])], path), ROOT + '/Source'), {})

    def test_a_file_is_named_relative_to_the_repository(self):
        self.assertEqual(list(U.read_tracefile(tracefile([(10, [(HIDDEN, 0)])], F), ROOT + '/Source')),
                         ['Source/sequoia/ns.hpp'])

    def test_a_function_no_demangler_can_name_is_refused(self):
        with self.assertRaises(U.Unkeyable):
            uncalled_keys([(10, [('_ZN2ns5brokenE', 0)])])


class Verdict(unittest.TestCase):
    def verdict(self, baseline_functions, current_functions):
        status, out, err = run(['baseline', '--tracefile', tracefile(baseline_functions, F), '--repository', ROOT]
                               + tools())
        self.assertEqual((status, err), (0, ''))
        return run(['compare', '--tracefile', tracefile(current_functions, F), '--repository', ROOT,
                    '--baseline', written(out, '.txt')] + tools())

    def test_unchanged_passes_silently(self):
        functions = [(10, [(CONVERT_STRING, 0)]), (20, [(CONVERT_VIEW, 1)])]
        self.assertEqual(self.verdict(functions, functions), (0, '', ''))

    def test_newly_uncalled_function_fails(self):
        status, out, _ = self.verdict([(10, [(CONVERT_STRING, 0)]), (20, [(CONVERT_VIEW, 1)])],
                                      [(10, [(CONVERT_STRING, 0)]), (20, [(CONVERT_VIEW, 0)])])
        self.assertEqual(status, 1)
        self.assertIn('error: uncalled, and not in the baseline: Source/sequoia/ns.hpp: '
                      'ns::convert(std::basic_string_view) (lines [20])', out)

    def test_listed_function_now_called_passes_with_notice(self):
        self.assertEqual(self.verdict([(10, [(CONVERT_STRING, 0)])], [(10, [(CONVERT_STRING, 2)])]),
                         (0, 'notice: now called, so it can leave the baseline: Source/sequoia/ns.hpp: '
                             'ns::convert(std::__cxx11::basic_string const&)\n', ''))

    def test_listed_function_gone_passes_with_its_own_notice(self):
        self.assertEqual(self.verdict([(10, [(CONVERT_STRING, 0)]), (20, [(CONVERT_VIEW, 1)])],
                                      [(20, [(CONVERT_VIEW, 1)])]),
                         (0, 'notice: no longer present, so it can leave the baseline: Source/sequoia/ns.hpp: '
                             'ns::convert(std::__cxx11::basic_string const&)\n', ''))

    def test_a_regression_cannot_hide_behind_an_improvement(self):
        status, _, _ = self.verdict([(10, [(CONVERT_STRING, 0)]), (20, [(CONVERT_VIEW, 1)])],
                                    [(10, [(CONVERT_STRING, 3)]), (20, [(CONVERT_VIEW, 0)])])
        self.assertEqual(status, 1)

    def test_second_uncalled_function_under_one_key_fails(self):
        status, out, _ = self.verdict([(10, [(SET_INT, 0)]), (11, [(SET2_INT, 1)])],
                                      [(10, [(SET_INT, 0)]), (11, [(SET2_INT, 0)])])
        self.assertEqual(status, 1)
        self.assertIn('ns::box::set (lines [10, 11])', out)

    def test_renumbered_lambda_is_not_a_change(self):
        """A lambda inserted above an uncalled one of the same signature
        changes the uncalled lambda's discriminator and its line."""
        self.assertEqual(self.verdict([(20, [(LAMBDA_INT_2, 0)])],
                                      [(21, [(LAMBDA_INT_3, 0)]), (18, [(LAMBDA_INT_1, 4)])]),
                         (0, '', ''))

    def test_a_tracefile_without_source_functions_is_refused(self):
        baseline   = written(''.join('# ' + line + '\n' for line in U.header(DEMANGLERS, [sys.executable])), '.txt')
        status, out, err = run(['compare', '--tracefile', tracefile([(10, [(HIDDEN, 0)])], '/r/Tests/ns.cpp'),
                                '--repository', ROOT, '--baseline', baseline] + tools())
        self.assertEqual((status, out), (2, ''))
        self.assertIn('has no function records within ' + ROOT + '/Source', err)

    def test_a_baseline_from_other_tools_is_refused(self):
        baseline   = written('# demangler: some other version\n', '.txt')
        status, out, err = run(['compare', '--tracefile', tracefile([(10, [(HIDDEN, 0)])], F),
                                '--repository', ROOT, '--baseline', baseline] + tools())
        self.assertEqual((status, out), (2, ''))
        self.assertIn('regenerate the baseline', err)

    def test_the_header_records_every_tool(self):
        status, out, _ = run(['baseline', '--tracefile', tracefile([(10, [(HIDDEN, 0)])], F), '--repository', ROOT]
                             + tools())
        self.assertEqual(status, 0)
        self.assertEqual([line for line in out.split('\n') if line.startswith('# ')],
                         ['# demangler: ' + U.version_of(tool) for tool in DEMANGLERS]
                         + ['# tool: ' + U.version_of(sys.executable)])

    def test_a_key_before_any_file_is_refused(self):
        baseline   = written(''.join('# ' + line + '\n' for line in U.header(DEMANGLERS, [sys.executable]))
                             + '    ns::twice\n', '.txt')
        status, out, err = run(['compare', '--tracefile', tracefile([(10, [(HIDDEN, 0)])], F),
                                '--repository', ROOT, '--baseline', baseline] + tools())
        self.assertEqual((status, out), (2, ''))
        self.assertIn('a key precedes the first file: ns::twice', err)


# Each mutant breaks the key or the verdict in a way that matters, and is
# (description, old text, new text). One mutant is left out as equivalent:
# dropping the guard that keeps `(anonymous namespace)` from being read as a
# nameless call. The guard's text comes back unchanged without it, since no
# template list can precede a namespace.
MUTATIONS = [
    ('keep template argument lists',     "if token == '<' and '<' not in stack:",  "if False:"),
    ('keep lists within parameters',     "if token == '<' and '<' not in stack:",  "if token == '<' and not stack:"),
    ('drop every parameter list',        "if any(at <= opening for at in removed) or", "if True or"),
    ('keep every parameter list',        "if any(at <= opening for at in removed) or", "if False or"),
    ('keep lambda discriminators',       "begin + len('{lambda')) + '}')",
                                         "begin + len('{lambda')) + '#' + (gnu_discriminator or '1') + '}')"),
    ('keep the return type',             "            start = offset + 1\n",    "            start = 0\n"),
    ('keep abi tags',                    r"re.sub(r'\[abi:[^\]]*\]|",          r"re.sub(r'"),
    ('uncalled if any alias is',         "return not any(aliases.values()) if",  "return not all(aliases.values()) if"),
    ('a set, not a multiset',            "Counter({function_key: len(starts)",    "Counter({function_key: 1"),
    ('fail on improvements too',         "return 1 if risen else 0",              "return 1 if risen or fallen else 0"),
    ('ignore FNA counts',                "aliases.get(name, 0) + int(calls)",     "aliases.get(name, 0)"),
    ('skip an unkeyable function',       "if not keys and uncalled:",             "if False:"),
    ('one demangler only',               "        if not pending:\n",             "        if demangled:\n"),
    ('empty tracefile accepted',         "if not functions:\n",                  "if False:\n"),
    ('baseline header not checked',      "if baseline_header != current_header:", "if False:"),
    ('return type counts as a template', "removed = [at for at in removed if at >= components[0][1]]", "pass"),
    ('an operator scope as a callable',  "CALLABLE.match(component, operator_length(component))",
                                         "CALLABLE.match(component)"),
    ('files outside Source read',        "if within_source else None",           "if True else None"),
    ('a prefix, not a directory',        "source_root.rstrip('/') + '/'",         "source_root.rstrip('/')"),
    ('every notice says now called',     "if fallen_key in called.get(file, Counter()):", "if True:"),
    ('every notice says gone',           "if fallen_key in called.get(file, Counter()):", "if False:"),
    ('recorded tools not recorded',      "[f'tool: {version_of(tool)}' for tool in recorded_tools]", "[]"),
]


def run_controls(module):
    global U
    U      = module
    result = unittest.TextTestRunner(stream=io.StringIO(), verbosity=0).run(
                 unittest.defaultTestLoader.loadTestsFromModule(sys.modules[__name__]))
    return len(result.failures) + len(result.errors), result.testsRun


def mutations():
    failures, total = run_controls(load(SOURCE))
    print(f'unmutated: {failures} of {total} controls fail' + ('' if failures == 0 else '  <-- must be none'))
    survivors = failures != 0
    for description, old, new in MUTATIONS:
        if SOURCE.count(old) != 1:
            print(f'{description}: the text to mutate occurs {SOURCE.count(old)} times')
            survivors = True
            continue
        failures, total = run_controls(load(SOURCE.replace(old, new)))
        print(f'{description}: {failures} of {total} controls fail' + ('' if failures else '  <-- SURVIVED'))
        survivors |= failures == 0
    return 1 if survivors else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--demangler', required=True, action='append')
    parser.add_argument('--mutations', action='store_true')
    arguments = parser.parse_args()
    DEMANGLERS.extend(arguments.demangler)
    if arguments.mutations:
        return mutations()
    failures, total = run_controls(load(SOURCE))
    if failures:
        unittest.TextTestRunner(verbosity=1).run(unittest.defaultTestLoader.loadTestsFromModule(sys.modules[__name__]))
        return 1
    print(f'uncalled_functions.py: all {total} controls pass')
    return 0


if __name__ == '__main__':
    sys.exit(main())
