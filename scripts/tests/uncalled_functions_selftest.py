#!/usr/bin/env python3
"""Controls for uncalled_functions.py, on synthetic tracefiles, and a mutation
check of the controls.

  uncalled_functions_selftest.py --demangler <tool>... [--mutations]

Without --mutations, the selftest runs the controls. With it, the selftest runs
the controls against the script and against each mutant of the script. The
script must fail no control, and each mutant at least one.

The demanglers are given in the order the script is to try them. The expected
keys are llvm-cxxfilt's and GNU c++filt's spellings, which agree on every name
here that both can demangle. The mangled names are
those g++ 15 and g++ 16 both give. gcc emits the C2 and D2 variants of a
constructor and destructor in sequoia's own tracefiles, and they are the C1 and
D1 names with the variant letter changed.
"""
import argparse, contextlib, io, itertools, os, stat, subprocess, sys, tempfile, types, unittest
from collections import Counter

HERE        = os.path.dirname(os.path.abspath(__file__))
SCRIPT_PATH = os.path.join(HERE, '..', 'uncalled_functions.py')
with open(SCRIPT_PATH, encoding='utf-8') as script_file:
    SOURCE = script_file.read()

DEMANGLERS    = []
script        = None
script_source = None
scratch       = None
file_names    = itertools.count()


def load(source):
    module = types.ModuleType('uncalled_functions')
    exec(compile(source, 'uncalled_functions.py', 'exec'), module.__dict__)
    return module


# A repository whose Source holds no .cpp file. On a runner, the path of
# sequoia's own checkout would hold its translation units, so the repository is
# made afresh.
REPOSITORY   = tempfile.TemporaryDirectory()
ROOT         = REPOSITORY.name
os.makedirs(ROOT + '/Source/sequoia')
SOURCE_FILE  = ROOT + '/Source/sequoia/ns.hpp'
OTHER_FILE   = ROOT + '/Source/sequoia/other.hpp'
FILE_KEY     = 'Source/sequoia/ns.hpp'

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
# A hidden friend of ns::s<int>, constrained on the class template's parameter,
# which llvm-cxxfilt spells ns::s<int>::friend operator==(...) and GNU c++filt
# cannot demangle
FRIEND_EQUAL    = '_ZN2ns1sIiEFeqERKS1_S3_Q1cIT_E'
# ns::twice<ns::café>(ns::café) as lcov writes it. lcov writes a function
# name's letters from U+0080 to U+00FF as single Latin-1 bytes, so a tracefile
# need not be UTF-8; sequoia's café_free_test is such a name. Neither
# demangler names a function whose name holds such a byte.
TWICE_CAFE      = '_ZN2ns5twiceINS_4caf\udce9EEET_S2_'
UNDEMANGLED     = '_ZN2ns5brokenE'


def written(text, suffix):
    """The path of a new file in the scratch directory, holding `text`."""
    path = os.path.join(scratch, f'{next(file_names)}{suffix}')
    with open(path, 'w', encoding='utf-8', errors='surrogateescape') as file:
        file.write(text)
    return path


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


def fake_tool(name, version_line, status):
    """The path of an executable named `name` which, asked for its version,
    writes to standard error a line holding a path with a dotted number in it,
    a line holding an undotted number, and then `version_line`, and exits with
    `status`."""
    path = os.path.join(scratch, name)
    with open(path, 'w', encoding='utf-8') as file:
        file.write(f'#!/bin/sh\n{{ echo /opt/python3.12/bin/{name}; echo "{name}, build 7"; '
                   f'echo "{version_line}"; }} >&2\nexit {status}\n')
    os.chmod(path, os.stat(path).st_mode | stat.S_IXUSR)
    return path


def uncalled_keys(functions):
    found = script.keys_by_file(script.read_tracefile(tracefile(functions, SOURCE_FILE), ROOT + '/Source'),
                                DEMANGLERS, script.Selection.uncalled)
    return script.counts(found).get(FILE_KEY, Counter())


def run_main(arguments):
    """(exit status, standard output, standard error) of the script's main,
    given `arguments`."""
    out, err = io.StringIO(), io.StringIO()
    saved    = sys.argv
    sys.argv = ['uncalled_functions.py'] + arguments
    try:
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            status = script.main()
    finally:
        sys.argv = saved
    return status, out.getvalue(), err.getvalue()


def tool_options(recorded_tool):
    return ([option for demangler in DEMANGLERS for option in ('--demangler', demangler)]
            + ['--recorded-tool', recorded_tool])


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
        self.assertEqual(script.key('std::vector<int, std::allocator<int> > ns::(anonymous namespace)::make<int>(int)'),
                         'ns::(anonymous namespace)::make')

    def test_a_return_type_does_not_make_the_enclosing_function_a_template(self):
        """A generic lambda's operator() returns the type its instantiation
        gives, and the enclosing g(int) is not a template."""
        generic = 'int ns::g(int)::{lambda(auto:1)#1}::operator()<int>(int) const'
        self.assertEqual(script.key('std::vector<int, std::allocator<int> > ns::g(int)::{lambda(auto:1)#1}::'
                                    'operator()<std::vector<int> >(std::vector<int>) const'),
                         script.key(generic))
        self.assertEqual(script.key(generic), 'ns::g(int)::{lambda(auto:1)}::operator()')

    def test_an_operator_as_an_enclosing_scope(self):
        self.assertEqual(script.key('ns::x::operator()(int) const::{lambda()#1}::operator()() const'),
                         'ns::x::operator()(int) const::{lambda()}::operator()() const')
        self.assertEqual(script.key('ns::t<int>::operator()(int) const::{lambda()#1}::operator()() const'),
                         'ns::t::operator()::{lambda}::operator()')

    def test_a_const_function_as_an_enclosing_scope(self):
        """The space before the enclosing function's `const` does not begin
        the name."""
        self.assertEqual(script.key('ns::a::f() const::{lambda()#1}::operator()() const'),
                         'ns::a::f() const::{lambda()}::operator()() const')

    def test_hidden_friends_of_a_class_template_collapse(self):
        """A friend defined in a class template is a non-template function
        per instantiation. Only its parameter types say which, so the lists
        within a parameter list go too."""
        self.assertEqual(script.key('sequoia::operator<=>(sequoia::mem_ordered_tuple<int, int> const&, '
                                    'sequoia::mem_ordered_tuple<int, int> const&)'),
                         script.key('sequoia::operator<=>(sequoia::mem_ordered_tuple<int> const&, '
                                    'sequoia::mem_ordered_tuple<int> const&)'))

    def test_a_friend_marker_goes(self):
        self.assertEqual(script.key('ns::f(int) [friend]'), 'ns::f(int)')

    def test_optimiser_suffixes_go(self):
        for name in ('ns::f(int) [clone .cold]', 'ns::f(int) [clone .isra.0] [clone .cold]', 'ns::f(int) (.cold)'):
            with self.subTest(name=name):
                self.assertEqual(script.key(name), 'ns::f(int)')

    def test_an_explicit_object_parameter_list_goes(self):
        self.assertEqual(script.key('void ns::s::f(this ns::s const&)'), 'ns::s::f')

    def test_unnamed_types_are_named_without_discriminator(self):
        """GNU c++filt numbers an unnamed type; llvm-cxxfilt spells the first
        `'unnamed'`."""
        for name in ('ns::f()::{unnamed type#2}::operator()() const', "ns::f()::'unnamed'::operator()() const"):
            with self.subTest(name=name):
                self.assertEqual(script.key(name), 'ns::f()::{unnamed type}::operator()() const')

    def test_a_conversion_to_a_scoped_type(self):
        self.assertEqual(script.key('ns::num::operator ns::other::type() const'),
                         'ns::num::operator ns::other::type() const')

    def test_noexcept_is_a_qualifier(self):
        self.assertEqual(script.key('ns::f(int) noexcept'), 'ns::f(int) noexcept')

    def test_a_requires_clause_within_a_template_argument(self):
        """The first ` requires ` lies within a template argument; only the
        second ends the name. The shape is a lambda's, from an initialiser of
        a constrained constructor, as llvm-cxxfilt prints it."""
        self.assertEqual(script.key("ns::w<ns::g(int) requires c<T>::'lambda'()>::w(int) requires d<int>"),
                         'ns::w::w')

    def test_files_outside_the_source_tree_are_ignored(self):
        for path in ('/r/Tests/ns.cpp', ROOT + '/SourceExtra/ns.hpp', '/elsewhere' + SOURCE_FILE):
            with self.subTest(path=path):
                self.assertEqual(script.read_tracefile(tracefile([(10, [(HIDDEN, 0)])], path), ROOT + '/Source'), {})

    def test_a_file_is_named_relative_to_the_repository(self):
        self.assertEqual(list(script.read_tracefile(tracefile([(10, [(HIDDEN, 0)])], SOURCE_FILE), ROOT + '/Source')),
                         [FILE_KEY])

    def test_a_record_without_an_end_line_is_read(self):
        path = written(f'SF:{SOURCE_FILE}\nFNL:0,12\nFNA:0,0,{HIDDEN}\nend_of_record\n', '.info')
        self.assertEqual(script.read_tracefile(path, ROOT + '/Source'), {FILE_KEY: {12: {HIDDEN: 0}}})

    def test_a_function_no_demangler_can_name_is_refused(self):
        with self.assertRaises(script.Unkeyable):
            uncalled_keys([(10, [(UNDEMANGLED, 0)])])

    def test_an_alias_no_demangler_can_name_is_passed_over(self):
        """The other alias names the function."""
        self.assertEqual(uncalled_keys([(10, [(TWICE_INT, 0), (UNDEMANGLED, 0)])]), Counter({'ns::twice': 1}))

    def test_a_name_which_is_not_utf8_is_read(self):
        self.assertEqual(uncalled_keys([(10, [(TWICE_INT, 0), (TWICE_CAFE, 0)])]), Counter({'ns::twice': 1}))

    def test_a_function_is_listed_under_each_key_its_aliases_give(self):
        """A lambda within a lambda, written on one line, has one record."""
        self.assertEqual(uncalled_keys([(10, [(HIDDEN, 0), (LAMBDA_INT_1, 0)])]),
                         Counter({'ns::(anonymous namespace)::hidden(int)':                         1,
                                  'ns::with_lambdas(int)::{lambda(int)}::operator()(int) const': 1}))

    def test_a_hidden_friend_of_a_class_template_is_named_by_its_scope(self):
        """llvm-cxxfilt writes `friend` between the scope and the name."""
        self.assertEqual(uncalled_keys([(10, [(FRIEND_EQUAL, 0)])]), Counter({'ns::s::operator==': 1}))

    def test_two_functions_at_one_start_line_are_one_record(self):
        lines = ['SF:' + SOURCE_FILE, 'FNL:0,10,12', f'FNA:0,3,{HIDDEN}', 'FNL:1,10,15', f'FNA:1,0,{TWICE_INT}',
                 'end_of_record']
        self.assertEqual(script.read_tracefile(written('\n'.join(lines) + '\n', '.info'), ROOT + '/Source'),
                         {FILE_KEY: {10: {HIDDEN: 3, TWICE_INT: 0}}})

    def test_two_records_of_one_file_are_merged(self):
        lines = ['SF:' + SOURCE_FILE, 'FNL:0,10,12', f'FNA:0,3,{HIDDEN}', 'end_of_record',
                 'SF:' + SOURCE_FILE, 'FNL:0,10,12', f'FNA:0,0,{HIDDEN}', 'FNL:1,20,22', f'FNA:1,0,{TWICE_INT}',
                 'end_of_record']
        self.assertEqual(script.read_tracefile(written('\n'.join(lines) + '\n', '.info'), ROOT + '/Source'),
                         {FILE_KEY: {10: {HIDDEN: 3}, 20: {TWICE_INT: 0}}})


class Verdict(unittest.TestCase):
    def setUp(self):
        self.tool = fake_tool('faketool', 'faketool (built with 13.4; Fake 9.9.1-1fake1) 9.9.1-0 [r123]', 0)

    def make_baseline(self, functions, path):
        status, out, err = run_main(['baseline', '--tracefile', tracefile(functions, path), '--repository', ROOT]
                                    + tool_options(self.tool))
        self.assertEqual((status, err), (0, ''))
        return written(out, '.txt')

    def compare(self, baseline, tracefile_path):
        return run_main(['compare', '--tracefile', tracefile_path, '--repository', ROOT, '--baseline', baseline]
                        + tool_options(self.tool))

    def verdict(self, baseline_functions, current_functions):
        return self.compare(self.make_baseline(baseline_functions, SOURCE_FILE),
                            tracefile(current_functions, SOURCE_FILE))

    def test_unchanged_passes_silently(self):
        functions = [(10, [(CONVERT_STRING, 0)]), (20, [(CONVERT_VIEW, 1)])]
        self.assertEqual(self.verdict(functions, functions), (0, '', ''))

    def test_newly_uncalled_function_fails(self):
        status, out, _ = self.verdict([(10, [(CONVERT_STRING, 0)]), (20, [(CONVERT_VIEW, 1)])],
                                      [(10, [(CONVERT_STRING, 0)]), (20, [(CONVERT_VIEW, 0)])])
        self.assertEqual(status, 1)
        self.assertIn('error: uncalled, and not in the baseline: Source/sequoia/ns.hpp: '
                      'ns::convert(std::basic_string_view) (lines [20])', out)

    def test_newly_uncalled_function_in_a_file_the_baseline_lacks_fails(self):
        """Most files have no uncalled function, so the baseline does not list
        them."""
        baseline = self.make_baseline([(10, [(HIDDEN, 0)])], OTHER_FILE)
        status, out, _ = self.compare(baseline, tracefile([(20, [(CONVERT_VIEW, 0)])], SOURCE_FILE))
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

    def test_one_of_two_uncalled_functions_under_a_key_gone(self):
        """The other function under the key is still uncalled, so the key is
        not called."""
        self.assertEqual(self.verdict([(10, [(SET_INT, 0)]), (11, [(SET2_INT, 0)])], [(10, [(SET_INT, 0)])]),
                         (0, 'notice: no longer present, so it can leave the baseline: Source/sequoia/ns.hpp: '
                             'ns::box::set\n', ''))

    def test_a_key_listed_twice_is_counted_twice(self):
        baseline = self.make_baseline([(10, [(SET_INT, 0)]), (11, [(SET2_INT, 0)])], SOURCE_FILE)
        self.assertEqual(self.compare(baseline, tracefile([(10, [(SET_INT, 0)]), (11, [(SET2_INT, 0)])],
                                                          SOURCE_FILE)),
                         (0, '', ''))
        status, out, _ = self.compare(baseline, tracefile([(10, [(SET_INT, 0)]), (11, [(SET2_INT, 3)])],
                                                          SOURCE_FILE))
        self.assertEqual((status, out), (0, 'notice: now called, so it can leave the baseline: '
                                            'Source/sequoia/ns.hpp: ns::box::set\n'))

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

    def test_the_header_records_every_tool(self):
        """The fake tool writes its version to standard error."""
        status, out, _ = run_main(['baseline', '--tracefile', tracefile([(10, [(HIDDEN, 0)])], SOURCE_FILE),
                                   '--repository', ROOT] + tool_options(self.tool))
        self.assertEqual(status, 0)
        self.assertEqual([line for line in out.split('\n') if line.startswith('# ')],
                         ['# demangler: ' + script.version_of(demangler) for demangler in DEMANGLERS]
                         + ['# tool: faketool 9.9.1-0'])

    def test_a_rebuild_of_a_recorded_tool_is_accepted(self):
        """A package revision is not recorded, so a rebuild of one release
        compares as that release."""
        baseline  = self.make_baseline([(10, [(HIDDEN, 0)])], SOURCE_FILE)
        self.tool = fake_tool('faketool', 'faketool (built with 14.3; Fake 9.9.1-2fake1) 9.9.1-0 [r124]', 0)
        self.assertEqual(self.compare(baseline, tracefile([(10, [(HIDDEN, 0)])], SOURCE_FILE)), (0, '', ''))

    def test_a_prerelease_of_a_recorded_tool_is_refused(self):
        """A prerelease's date lies outside the groups, so it tells the
        prerelease from the release."""
        baseline  = self.make_baseline([(10, [(HIDDEN, 0)])], SOURCE_FILE)
        self.tool = fake_tool('faketool', 'faketool (built with 13.4; Fake 9.9.1-1fake1) 9.9.1-0 20260101 (prerelease)',
                              0)
        self.assert_refused(self.compare(baseline, tracefile([(10, [(HIDDEN, 0)])], SOURCE_FILE)),
                            'regenerate the baseline')

    def test_a_tool_without_a_version_number_is_refused(self):
        self.tool = fake_tool('faketool', 'faketool, no version', 0)
        self.assert_refused(run_main(['baseline', '--tracefile', tracefile([(10, [(HIDDEN, 0)])], SOURCE_FILE),
                                      '--repository', ROOT] + tool_options(self.tool)),
                            'gives no dotted version number outside a path')

    def assert_refused(self, result, message):
        status, out, err = result
        self.assertEqual((status, out), (2, ''))
        self.assertIn(message, err)

    def test_a_baseline_from_another_version_of_a_recorded_tool_is_refused(self):
        baseline  = self.make_baseline([(10, [(HIDDEN, 0)])], SOURCE_FILE)
        self.tool = fake_tool('faketool', 'faketool (built with 13.4; Fake 9.9.2-1fake1) 9.9.2-0 [r123]', 0)
        self.assert_refused(self.compare(baseline, tracefile([(10, [(HIDDEN, 0)])], SOURCE_FILE)),
                            'regenerate the baseline')

    def test_a_baseline_without_a_header_is_refused(self):
        baseline = self.make_baseline([(10, [(HIDDEN, 0)])], SOURCE_FILE)
        with open(baseline, encoding='utf-8') as file:
            entries = [line for line in file if not line.startswith('# ')]
        self.assert_refused(self.compare(written(''.join(entries), '.txt'),
                                         tracefile([(10, [(HIDDEN, 0)])], SOURCE_FILE)),
                            'regenerate the baseline')

    def test_a_key_before_any_file_is_refused(self):
        baseline = self.make_baseline([(10, [(HIDDEN, 0)])], SOURCE_FILE)
        with open(baseline, encoding='utf-8') as file:
            header_lines = [line for line in file if line.startswith('# ')]
        self.assert_refused(self.compare(written(''.join(header_lines) + '    ns::twice\n', '.txt'),
                                         tracefile([(10, [(HIDDEN, 0)])], SOURCE_FILE)),
                            'a key precedes the first file: ns::twice')

    def test_a_tracefile_without_source_functions_is_refused(self):
        baseline = self.make_baseline([(10, [(HIDDEN, 0)])], SOURCE_FILE)
        for path in (tracefile([(10, [(HIDDEN, 0)])], '/r/Tests/ns.cpp'),
                     written(f'SF:{SOURCE_FILE}\nDA:1,1\nend_of_record\n', '.info')):
            with self.subTest(path=path):
                self.assert_refused(self.compare(baseline, path),
                                    'has no function records within ' + ROOT + '/Source')

    def test_a_malformed_tracefile_is_refused(self):
        """The last case names an index which only another file's record
        gives."""
        baseline = self.make_baseline([(10, [(HIDDEN, 0)])], SOURCE_FILE)
        for lines in ([f'FNA:7,0,{HIDDEN}'],
                      ['FNL:0'],
                      ['FNL:0,10,12', 'FNL:1,20,22', 'end_of_record', 'SF:' + OTHER_FILE, 'FNL:0,20,22',
                       f'FNA:1,0,{HIDDEN}']):
            with self.subTest(lines=lines):
                path = written('\n'.join(['SF:' + SOURCE_FILE] + lines + ['end_of_record']) + '\n', '.info')
                self.assert_refused(self.compare(baseline, path),
                                    f'{path}:{len(lines) + 1}: malformed record: {lines[-1]}')

    def test_an_absent_tracefile_is_refused(self):
        baseline = self.make_baseline([(10, [(HIDDEN, 0)])], SOURCE_FILE)
        self.assert_refused(self.compare(baseline, os.path.join(scratch, 'absent.info')), 'absent.info')

    def verdict_on_units(self, sources, tracefile_text):
        """The verdict, against a baseline made from the same tracefile, in a
        new repository holding an empty file at each of `sources`.
        `tracefile_text` gives the repository's path as `{root}`."""
        root = os.path.join(scratch, f'repository{next(file_names)}')
        for source in sources:
            os.makedirs(os.path.dirname(os.path.join(root, source)), exist_ok=True)
            open(os.path.join(root, source), 'w').close()
        path     = written(tracefile_text.format(root=root), '.info')
        options  = ['--tracefile', path, '--repository', root] + tool_options(self.tool)
        status, out, err = run_main(['baseline'] + options)
        self.assertEqual((status, err), (0, ''))
        return run_main(['compare', '--baseline', written(out, '.txt')] + options)

    def test_a_translation_unit_the_tracefile_lacks_fails(self):
        """A header, and a translation unit outside Source, are not looked
        for."""
        self.assertEqual(self.verdict_on_units(['Source/lib/traced.cpp', 'Source/lib/untraced.cpp',
                                                'Source/lib/also_untraced.cpp', 'Source/lib/untraced.hpp',
                                                'Tests/untraced.cpp'],
                                               f'SF:{{root}}/Source/lib/traced.cpp\nFNL:0,10,13\nFNA:0,0,{HIDDEN}\n'
                                               'end_of_record\n'),
                         (1, 'error: not in the tracefile, so none of its functions is seen: '
                             'Source/lib/also_untraced.cpp\n'
                             'error: not in the tracefile, so none of its functions is seen: '
                             'Source/lib/untraced.cpp\n', ''))

    def test_a_repository_without_source_is_refused(self):
        """The tracefile names a repository which is not on this machine."""
        baseline = self.make_baseline([(10, [(HIDDEN, 0)])], SOURCE_FILE)
        self.assert_refused(run_main(['compare', '--tracefile',
                                      tracefile([(10, [(HIDDEN, 0)])], '/elsewhere/Source/sequoia/ns.hpp'),
                                      '--repository', '/elsewhere', '--baseline', baseline]
                                     + tool_options(self.tool)),
                            '/elsewhere/Source is not a directory')

    def test_a_translation_unit_with_line_records_alone_is_traced(self):
        self.assertEqual(self.verdict_on_units(['Source/lib/traced.cpp', 'Source/lib/lines.cpp'],
                                               f'SF:{{root}}/Source/lib/traced.cpp\nFNL:0,10,13\nFNA:0,0,{HIDDEN}\n'
                                               'end_of_record\nSF:{root}/Source/lib/lines.cpp\nDA:1,1\n'
                                               'end_of_record\n'),
                         (0, '', ''))

    def test_an_unkeyable_uncalled_function_is_refused(self):
        baseline = self.make_baseline([(10, [(HIDDEN, 0)])], SOURCE_FILE)
        self.assert_refused(self.compare(baseline, tracefile([(10, [(UNDEMANGLED, 0)])], SOURCE_FILE)),
                            'no alias of this uncalled function can be keyed')

    def test_an_unkeyable_called_function_is_passed_over(self):
        self.assertEqual(self.verdict([(10, [(CONVERT_STRING, 0)])], [(20, [(UNDEMANGLED, 4)])]),
                         (0, 'notice: no longer present, so it can leave the baseline: Source/sequoia/ns.hpp: '
                             'ns::convert(std::__cxx11::basic_string const&)\n', ''))

    def test_the_verdict_puts_notices_first_and_sorts_each_kind(self):
        """The errors' keys sort in the opposite order to their lines. The
        implementation chooses this order; the gate's verdict does not depend
        on it."""
        self.assertEqual(self.verdict([(10, [(A_ROOT, 0)]), (20, [(B_ROOT, 0)])],
                                      [(10, [(A_ROOT, 1)]), (30, [(CONVERT_VIEW, 0)]), (40, [(CONVERT_STRING, 0)])]),
                         (1, 'notice: now called, so it can leave the baseline: Source/sequoia/ns.hpp: '
                             'ns::a::project_root() const\n'
                             'notice: no longer present, so it can leave the baseline: Source/sequoia/ns.hpp: '
                             'ns::b::project_root() const\n'
                             'error: uncalled, and not in the baseline: Source/sequoia/ns.hpp: '
                             'ns::convert(std::__cxx11::basic_string const&) (lines [40])\n'
                             'error: uncalled, and not in the baseline: Source/sequoia/ns.hpp: '
                             'ns::convert(std::basic_string_view) (lines [30])\n', ''))

    def test_the_baseline_format(self):
        """The committed baseline is in this format, so a change to the format
        shows in every diff of a fresh baseline against the committed one."""
        status, out, err = run_main(['baseline', '--tracefile',
                                     written(f'SF:{OTHER_FILE}\nFNL:0,5,6\nFNA:0,0,{HIDDEN}\nend_of_record\n'
                                             f'SF:{SOURCE_FILE}\nFNL:0,10,11\nFNA:0,0,{CONVERT_VIEW}\nFNL:1,20,21\n'
                                             f'FNA:1,0,{SET_INT}\nFNL:2,30,31\nFNA:2,0,{SET2_INT}\n'
                                             'end_of_record\n', '.info'),
                                     '--repository', ROOT] + tool_options(self.tool))
        self.assertEqual((status, err), (0, ''))
        self.assertEqual(out, ''.join(f'# demangler: {script.version_of(demangler)}\n' for demangler in DEMANGLERS)
                              + '# tool: faketool 9.9.1-0\n'
                              'Source/sequoia/ns.hpp\n'
                              '    ns::box::set\n'
                              '    ns::box::set\n'
                              '    ns::convert(std::basic_string_view)\n'
                              'Source/sequoia/other.hpp\n'
                              '    ns::(anonymous namespace)::hidden(int)\n')

    def test_a_blank_line_in_a_baseline_is_passed_over(self):
        baseline = self.make_baseline([(10, [(HIDDEN, 0)])], SOURCE_FILE)
        with open(baseline, encoding='utf-8') as file:
            text = file.read().replace('\n    ', '\n\n    ')
        self.assertEqual(self.compare(written(text, '.txt'), tracefile([(10, [(HIDDEN, 0)])], SOURCE_FILE)),
                         (0, '', ''))

    def test_a_repository_named_with_a_final_slash(self):
        status, out, err = run_main(['baseline', '--tracefile', tracefile([(10, [(HIDDEN, 0)])], SOURCE_FILE),
                                     '--repository', ROOT + '/'] + tool_options(self.tool))
        self.assertEqual((status, err), (0, ''))
        self.assertIn(FILE_KEY + '\n    ns::(anonymous namespace)::hidden(int)\n', out)

    def test_a_failing_demangler_is_refused(self):
        """The demangler gives its version, and then fails to demangle."""
        failing = os.path.join(scratch, 'failing_demangler')
        with open(failing, 'w', encoding='utf-8') as file:
            file.write('#!/bin/sh\nif [ "$1" = --version ]; then echo "failing 1.2"; else exit 3; fi\n')
        os.chmod(failing, os.stat(failing).st_mode | stat.S_IXUSR)
        self.assert_refused(run_main(['baseline', '--tracefile', tracefile([(10, [(HIDDEN, 0)])], SOURCE_FILE),
                                      '--repository', ROOT, '--demangler', failing] + tool_options(self.tool)),
                            f"Command '['{failing}']' returned non-zero exit status 3")

    def test_a_tool_whose_version_query_fails_is_refused(self):
        self.tool = fake_tool('faketool', 'faketool 9.9.1', 4)
        self.assert_refused(run_main(['baseline', '--tracefile', tracefile([(10, [(HIDDEN, 0)])], SOURCE_FILE),
                                      '--repository', ROOT] + tool_options(self.tool)),
                            'returned non-zero exit status 4')

    def test_a_path_which_is_not_utf8_is_written_and_read_as_itself(self):
        """The script runs as a program, since only then does it set the
        encoding of its output. The environment asks for strict UTF-8, which
        a locale may otherwise relax."""
        traced = tracefile([(10, [(HIDDEN, 0)])], ROOT + '/Source/sequoia/caf\udce9.hpp')
        run    = subprocess.run([sys.executable, written(script_source, '.py'), 'baseline', '--tracefile', traced,
                                 '--repository', ROOT] + tool_options(self.tool),
                                capture_output=True, env=dict(os.environ, PYTHONIOENCODING='utf-8:strict'))
        self.assertEqual((run.returncode, run.stderr), (0, b''))
        self.assertIn(b'\nSource/sequoia/caf\xe9.hpp\n    ns::(anonymous namespace)::hidden(int)\n', run.stdout)
        baseline = os.path.join(scratch, 'not_utf8_baseline.txt')
        with open(baseline, 'wb') as file:
            file.write(run.stdout)
        self.assertEqual(self.compare(baseline, traced), (0, '', ''))


# Each mutant breaks the key, the reader, the header or the verdict, and is
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
    ('a const scope begins the name',    "not re.match(r'(const|volatile|&&?)(\\s|::)', head[offset + 1:])", "True"),
    ('keep abi tags',                    r"re.sub(r'\[abi:[^\]]*\]|",          r"re.sub(r'"),
    ('keep the friend marker',           r"|\[friend\]|(?<=::)friend '",       r"|(?<=::)friend '"),
    ('keep the friend before a name',    r"|\[friend\]|(?<=::)friend '",       r"|\[friend\]'"),
    ('keep optimiser suffixes',          "text          = re.sub(r'( \\[clone",
                                         "text          = (lambda *arguments: arguments[2])(r'( \\[clone"),
    ('keep a clone suffix',              r"( \[clone [^\]]*\]| \(\.[\w.]+\))+$",   r"( \(\.[\w.]+\))+$"),
    ('keep a cold suffix',               r"( \[clone [^\]]*\]| \(\.[\w.]+\))+$",   r"( \[clone [^\]]*\])+$"),
    ('keep the requires-clause',         "'', without_requires_clause(text))",  "'', text)"),
    ('search for requires once',         "at = text.find(' requires ', at + 1)", "at = -1"),
    ('keep an explicit object list',     " or parameters.startswith('this '):", ":"),
    ('keep unnamed type discriminators', "parts.append('{unnamed type}')",       "parts.append(component)"),
    ('noexcept not a qualifier',         "|&&|&|noexcept))*)",                  "|&&|&))*)"),
    ('drop qualifiers',                  "+ (' ' + qualifiers if qualifiers else '')", ""),
    ('uncalled if any alias is',         "else not called",                       "else not all(aliases.values())"),
    ('the called pass takes all',        "return called if selection == Selection.called else not called",
                                         "return True if selection == Selection.called else not called"),
    ('a set, not a multiset',            "Counter({function_key: len(starts)",    "Counter({function_key: 1"),
    ('the baseline read as a set',       "uncalled[file][line.strip()] += 1",     "uncalled[file][line.strip()] = 1"),
    ('fail on improvements too',         "return 1 if risen or untraced else 0",
                                         "return 1 if risen or untraced or fallen else 0"),
    ('an untraced unit passes',          "return 1 if risen or untraced else 0",  "return 1 if risen else 0"),
    ('an untraced unit unnamed',         "for file in untraced:",                 "for file in []:"),
    ('headers looked for too',           ".rglob('*.cpp')",                       ".rglob('*.*')"),
    ('only the top of Source looked at', ".rglob('*.cpp')",                       ".glob('*.cpp')"),
    ('units outside Source looked for',  "(root / 'Source').rglob",               "root.rglob"),
    ('a missing Source accepted',        "if not (root / 'Source').is_dir():",   "if False:"),
    ('a unit needs function records',    "untraced_translation_units(arguments.repository, traced)",
                                         "untraced_translation_units(arguments.repository, functions)"),
    ('files the baseline lacks skipped', "for file in sorted(set(baseline) | set(current)):",
                                         "for file in sorted(set(baseline)):"),
    ('ignore FNA counts',                "aliases.get(name, 0) + int(calls)",     "aliases.get(name, 0)"),
    ('require an end line',              "index, start = value.split(',')[:2]",  "index, start, _ = value.split(',')"),
    ('a malformed record crashes',       "            except (KeyError, ValueError):", "            except ():"),
    ('keep files without functions',     "for file, starts in traced.items() if starts}",
                                         "for file, starts in traced.items()}"),
    ('skip an unkeyable function',       "if not keys and selection == Selection.uncalled:", "if False:"),
    ('an unkeyable function crashes',    "except (Refusal, Unkeyable, OSError,",  "except (Refusal, OSError,"),
    ('an absent file crashes',           "except (Refusal, Unkeyable, OSError,",  "except (Refusal, Unkeyable,"),
    ('one demangler only',               "        if not pending:\n",             "        if demangled:\n"),
    ('empty tracefile accepted',         "if not functions:\n",                  "if False:\n"),
    ('baseline header not checked',      "if baseline_header != current_header:", "if False:"),
    ('only the first header line',       "if baseline_header != current_header:",
                                         "if baseline_header[:1] != current_header[:1]:"),
    ('a headerless baseline accepted',   "if baseline_header != current_header:",
                                         "if baseline_header and baseline_header != current_header:"),
    ('versions from stdout only',        "stderr=subprocess.STDOUT",              "stderr=subprocess.DEVNULL"),
    ('a constant version',               "            return ' '.join(words)",  "            return 'v'"),
    ('the whole line kept',              "words = GROUP.sub(' ', line).split()",   "words = line.split()"),
    ('brackets kept',                    r"|\[[^\[\]]*\]')",                     "')"),
    ('a path may hold the version',      " and '/' not in word for word",          " for word in word"),
    ('an undotted number is a version',  r"r'\d+(?:\.\d+)+'",                       r"r'\d+(?:\.\d+)*'"),
    ('no version accepted',              "raise Refusal(f'{tool} --version gives no dotted",
                                         "return (f'{tool} --version gives no dotted"),
    ('a key before any file accepted',   "raise Refusal(f'{path}: a key precedes the first file: {line.strip()}')",
                                         "continue"),
    ('return type counts as a template', "removed = [at for at in removed if at >= components[0][1]]", "pass"),
    ('an operator scope as a callable',  "CALLABLE.match(component, operator_length(component))",
                                         "CALLABLE.match(component)"),
    ('files outside Source read',        "if within_source else None",           "if True else None"),
    ('a prefix, not a directory',        "source_root.rstrip('/') + '/'",         "source_root.rstrip('/')"),
    ('every notice says now called',     "if fallen_key in called.get(file, {}):", "if True:"),
    ('every notice says gone',           "if fallen_key in called.get(file, {}):", "if False:"),
    ('recorded tools not recorded',      "[f'tool: {version_of(tool)}' for tool in recorded_tools]", "[]"),
    ('demanglers not recorded',          "[f'demangler: {version_of(tool)}' for tool in demanglers]", "[]"),
    ('files named by absolute path',     "value[len(relative_from):]",             "value"),
    ('a final slash kept',               "arguments.repository.rstrip('/') + '/Source'",
                                         "arguments.repository + '/Source'"),
    ('indices kept across files',        "                    index_to_start = {}\n", ""),
    ('a start line holds one function',  "current.setdefault(int(start), {})",
                                         "current.__setitem__(int(start), {})"),
    ('a file holds one record',          "functions.setdefault(value[len(relative_from):], {})",
                                         "(functions.pop(value[len(relative_from):], None), "
                                         "functions.setdefault(value[len(relative_from):], {}))[1]"),
    ('the last count of a name wins',    "aliases[name] = aliases.get(name, 0) + int(calls)",
                                         "aliases[name] = int(calls)"),
    ('a malformed line unnumbered',      "raise Refusal(f'{path}:{number}: malformed",
                                         "raise Refusal(f'{path}: malformed"),
    ('untraced units unsorted',          "sorted(unit for unit in units if unit not in traced)",
                                         "sorted((unit for unit in units if unit not in traced), reverse=True)"),
    ('a failing demangler passed over',  "errors='surrogateescape', check=True).stdout.split",
                                         "errors='surrogateescape', check=False).stdout.split"),
    ('a failing version query read',     "check=True).stdout\n",                 "check=False).stdout\n"),
    ('a failing tool crashes',           "OSError, subprocess.CalledProcessError)", "OSError)"),
    ('an undemangled name dropped',      "    demangled.update((mangled, mangled) for mangled in pending)\n", ""),
    ('an unkeyable alias refused',       "                except Unkeyable:\n                    pass",
                                         "                except Unkeyable:\n                    raise"),
    ('a called unkeyable one refused',   "if not keys and selection == Selection.uncalled:", "if not keys:"),
    ('one key per function',             "for function_key in keys:",
                                         "for function_key in sorted(keys)[:1]:"),
    ('a key once per alias',             "for function_key in keys:",
                                         "for function_key in list(keys) * len(aliases):"),
    ('a tracefile read as strict UTF-8', "with open(path, encoding='utf-8', errors='surrogateescape') as tracefile:",
                                         "with open(path, encoding='utf-8') as tracefile:"),
    ('a baseline read as strict UTF-8',  "with open(path, encoding='utf-8', errors='surrogateescape') as baseline:",
                                         "with open(path, encoding='utf-8') as baseline:"),
    ('names sent as strict UTF-8',       "errors='surrogateescape', check=True).stdout.split",
                                         "check=True).stdout.split"),
    ('output written as strict UTF-8',   "        stream.reconfigure(encoding='utf-8', errors='surrogateescape')",
                                         "        pass"),
    ('baseline keys unsorted',           "sorted(keys.elements())",               "list(keys.elements())"),
    ('errors in line order',             "sorted((now - was).elements())]\n        fallen",
                                         "list((now - was).elements())]\n        fallen"),
    ('a blank line names a file',        "            elif line.strip():\n",    "            else:\n"),
]


def run_controls(source):
    """(failures, controls run) of the controls against the script whose text
    is `source`, each run given a scratch directory which is removed
    afterwards."""
    global script, script_source, scratch
    script, script_source = load(source), source
    with tempfile.TemporaryDirectory() as directory:
        scratch = directory
        result  = unittest.TextTestRunner(stream=io.StringIO(), verbosity=0).run(
                      unittest.defaultTestLoader.loadTestsFromModule(sys.modules[__name__]))
    return len(result.failures) + len(result.errors), result.testsRun


def mutations():
    failures, total = run_controls(SOURCE)
    print(f'unmutated: {failures} of {total} controls fail' + ('' if failures == 0 else '  <-- must be none'))
    flawed = failures != 0 or total == 0
    for description, old, new in MUTATIONS:
        if SOURCE.count(old) != 1:
            print(f'{description}: the text to mutate occurs {SOURCE.count(old)} times')
            flawed = True
            continue
        failures, total = run_controls(SOURCE.replace(old, new))
        print(f'{description}: {failures} of {total} controls fail' + ('' if failures else '  <-- SURVIVED'))
        flawed |= failures == 0
    if flawed or not MUTATIONS:
        return 1
    print(f'uncalled_functions.py: {len(MUTATIONS)} mutants, every one killed')
    return 0


def main():
    global script, script_source, scratch
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0], allow_abbrev=False)
    parser.add_argument('--demangler', required=True, action='append')
    parser.add_argument('--mutations', action='store_true')
    arguments = parser.parse_args()
    DEMANGLERS.extend(arguments.demangler)
    if arguments.mutations:
        return mutations()
    failures, total = run_controls(SOURCE)
    if total == 0:
        print('uncalled_functions_selftest.py: no controls were found')
        return 1
    if failures:
        script, script_source = load(SOURCE), SOURCE
        with tempfile.TemporaryDirectory() as directory:
            scratch = directory
            unittest.TextTestRunner(verbosity=1).run(
                unittest.defaultTestLoader.loadTestsFromModule(sys.modules[__name__]))
        return 1
    print(f'uncalled_functions.py: all {total} controls pass')
    return 0


if __name__ == '__main__':
    sys.exit(main())
