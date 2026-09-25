////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DumpComparisonFreeTest.hpp"
#include "sequoia/TestFramework/DumpComparison.hpp"
#include "sequoia/Streaming/Streaming.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  template<>
  struct value_tester<dump_comparison>
  {
    template<test_mode Mode, class Advisor>
    static void test(equality_check_t,
                     test_logger<Mode>& logger,
                     const dump_comparison& obtained,
                     const dump_comparison& prediction,
                     const tutor<Advisor>& advisor)
    {
      check(equality, "Missing", logger, obtained.missing, prediction.missing, advisor);
      check(equality, "Added",   logger, obtained.added,   prediction.added,   advisor);
    }
  };

  namespace
  {
    // A dump as the logger writes one: each top-level check's message and the messages nested in
    // it, one per line, then the separator; a message may end in a newline of its own, as beta does
    [[nodiscard]]
    std::string dump_of(std::initializer_list<std::string_view> checks)
    {
      std::string dump{};
      for(const auto check : checks)
      {
        dump.append(check).append("\n").append(dump_format::check_separator);
      }

      return dump;
    }

    constexpr std::string_view
      alpha{"Tests/Alpha.cpp, Line 10\nAlpha holds\n\n[int]"},
      alphaMoved{"Tests/Alpha.cpp, Line 14\nAlpha holds\n\n[int]"},
      alphaRetyped{"Tests/Alpha.cpp, Line 10\nAlpha holds\n\n[long]"},
      beta{"Tests/Beta.cpp, Line 20\nBeta holds\n\n[bool]\n"},
      gamma{"Tests/Gamma.cpp, Line 30\nGamma holds\n\n[bool]"},
      // A check with a nested check of its own location, as a helper taking a reporter produces
      nested{"Tests/Delta.cpp, Line 40\nTests/Delta.cpp, Line 7\n\nDelta holds\n[int]"},
      nestedMoved{"Tests/Delta.cpp, Line 44\nTests/Delta.cpp, Line 9\n\nDelta holds\n[int]"},
      // A semantics check: a message ending in a newline, an empty nested message writing nothing,
      // then the nested checks' messages
      semantics{"Tests/Epsilon.cpp, Line 50\n\n[thing]\n\noperator== is inconsistent (x)\n[bool]\noperator< is inconsistent (x)\n[bool]"},
      undescribed{"Tests/Zeta.cpp, Line 60\n\n[bool]"};
  }

  [[nodiscard]]
  fs::path dump_comparison_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void dump_comparison_free_test::run_tests()
  {
    test_reading();
    test_comparison();
    test_report();
  }

  void dump_comparison_free_test::test_reading()
  {
    const auto dump{working_materials() /= "Dump.txt"};
    write_to_file(dump, dump_of({alpha, beta}), std::ios_base::out);

    check(equality, "The checks of a dump, in order, without their surrounding newlines",
          read_dump(dump), std::vector<std::string>{std::string{alpha}, "Tests/Beta.cpp, Line 20\nBeta holds\n\n[bool]"});

    write_to_file(dump, dump_of({semantics, nested}), std::ios_base::out);
    check(equality, "A check's nested messages, blank lines included, are part of it",
          read_dump(dump), std::vector<std::string>{std::string{semantics}, std::string{nested}});
    write_to_file(dump, "", std::ios_base::out);
    check(equality, "An empty dump holds no checks", read_dump(dump), std::vector<std::string>{});

    check_exception_thrown<std::runtime_error>("A dump which does not exist", [this](){
      return read_dump(working_materials() /= "Absent.txt");
    });
  }

  void dump_comparison_free_test::test_comparison()
  {
    const auto baseline{working_materials() /= "Baseline.txt"}, dump{working_materials() /= "Dump.txt"};

    auto compare{
      [&](std::string_view description,
          std::initializer_list<std::string_view> before,
          std::initializer_list<std::string_view> after,
          dump_comparison prediction) {
        write_to_file(baseline, dump_of(before), std::ios_base::out);
        write_to_file(dump,     dump_of(after),  std::ios_base::out);
        check(equality, description, compare_dumps(baseline, dump), prediction);
      }
    };

    compare("The same checks",                  {alpha, beta}, {alpha, beta}, {});
    compare("A check missing",                  {alpha, beta}, {alpha},       {.missing{"Tests/Beta.cpp, Line 20\nBeta holds\n\n[bool]"}});
    compare("A check added",                    {alpha},       {alpha, beta}, {.added{"Tests/Beta.cpp, Line 20\nBeta holds\n\n[bool]"}});
    compare("A check moved by an edit above it", {alpha, beta}, {alphaMoved, beta}, {});
    compare("A check whose types changed is one missing and one added",
            {alpha, beta}, {alphaRetyped, beta}, {.missing{std::string{alpha}}, .added{std::string{alphaRetyped}}});
    compare("A repeated check lost once is one missing check",
            {alpha, alpha, alpha}, {alpha, alpha}, {.missing{std::string{alpha}}});
    compare("Order is not identity",            {alpha, beta}, {beta, alpha}, {});
    compare("A check moved by an edit above it, with a nested location moved too, is neither",
            {nested, beta}, {nestedMoved, beta}, {});
    compare("A semantics check is one check, however many it nests",
            {semantics, alpha}, {alpha}, {.missing{std::string{semantics}}});
    compare("The surplus is attributed to the last occurrence",
            {alpha, alphaMoved}, {alpha}, {.missing{std::string{alphaMoved}}});
    compare("Missing and added checks are reported in the order of the dump holding them",
            {gamma, alpha, beta}, {beta}, {.missing{std::string{gamma}, std::string{alpha}}});
  }

  void dump_comparison_free_test::test_report()
  {
    check(equality, "Nothing missing, nothing added",
          to_string({}, "before"),
          std::string{"Dump compared with 'before': 0 checks missing, 0 checks added\n"});

    check(equality, "One of each, as location and description",
          to_string({.missing{std::string{alpha}}, .added{std::string{beta}}}, "before"),
          std::string{"Dump compared with 'before': 1 check missing, 1 check added\n"
                      "\n"
                      "Missing:\n"
                      "  Tests/Alpha.cpp, Line 10: Alpha holds\n"
                      "\n"
                      "Added:\n"
                      "  Tests/Beta.cpp, Line 20: Beta holds\n"});

    check(equality, "A check without a description shows the first line which says anything",
          to_string({.missing{std::string{undescribed}, std::string{semantics}}}, "before"),
          std::string{"Dump compared with 'before': 2 checks missing, 0 checks added\n"
                      "\n"
                      "Missing:\n"
                      "  Tests/Zeta.cpp, Line 60: [bool]\n"
                      "  Tests/Epsilon.cpp, Line 50: [thing]\n"});
  }
}
