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
    // A dump as the logger writes one: each check's message and a newline, then two blank lines;
    // the message may end in a newline of its own, as the second here does
    [[nodiscard]]
    std::string dump_of(std::initializer_list<std::string_view> checks)
    {
      std::string dump{};
      for(const auto check : checks)
      {
        dump.append(check).append("\n").append("\n\n");
      }

      return dump;
    }

    constexpr std::string_view
      alpha{"Tests/Alpha.cpp, Line 10\nAlpha holds\n\n[int]"},
      alphaMoved{"Tests/Alpha.cpp, Line 14\nAlpha holds\n\n[int]"},
      alphaRetyped{"Tests/Alpha.cpp, Line 10\nAlpha holds\n\n[long]"},
      beta{"Tests/Beta.cpp, Line 20\nBeta holds\n\n[bool]\n"},
      gamma{"Tests/Gamma.cpp, Line 30\nGamma holds\n\n[bool]"};
  }

  [[nodiscard]]
  std::filesystem::path dump_comparison_free_test::source_file()
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
  }
}
