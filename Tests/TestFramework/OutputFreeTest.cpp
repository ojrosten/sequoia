////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "OutputFreeTest.hpp"
#include "sequoia/TestFramework/Output.hpp"

namespace sequoia::testing
{
  using namespace std::string_literals;
  namespace fs = std::filesystem;

  [[nodiscard]]
  std::filesystem::path output_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void output_free_test::run_tests()
  {
    test_emphasise();
    test_display_character();
    test_tidy_name();
    test_relative_reporting_path();
    test_absolute_reporting_path();
  }

  void output_free_test::test_emphasise()
  {
    check(equality, "Emphasis", emphasise("foo"), "--foo--"s);
    check(equality, "Nothing to emphasise", emphasise(""), ""s);
  }

  void output_free_test::test_display_character()
  {
    check(equality, "", display_character('\n'), "'\\n'"s);
    check(equality, "", display_character('\t'), "'\\t'"s);
    check(equality, "", display_character('\0'), "'\\0'"s);
    check(equality, "", display_character(' '), "' '"s);
  }

  void output_free_test::test_tidy_name()
  {
    check(equality, "", tidy_name("(some enum)0", clang_type{}), "0"s);
    check(equality, "", tidy_name("<textued_2d>", clang_type{}), "<textued_2d>"s);
    check(equality, "", tidy_name("struct foo", msvc_type{}), "foo"s);
  }

  void output_free_test::test_relative_reporting_path()
  {
    check(equality, "No ..",     path_for_reporting(      "Tests/foo.cpp", ""), fs::path{"Tests/foo.cpp"});
    check(equality, "Single ..", path_for_reporting(   "../Tests/foo.cpp", ""), fs::path{"Tests/foo.cpp"});
    check(equality, "Double ..", path_for_reporting("../../Tests/foo.cpp", ""), fs::path{"Tests/foo.cpp"});
  }

  void output_free_test::test_absolute_reporting_path()
  {
    const auto root{fs::current_path().root_path()};
    const auto testRepo{root / "Tests"}, file{testRepo / "foo.cpp"};

    check(equality, "File in repo", path_for_reporting(file, testRepo), fs::path{"Tests/foo.cpp"});
  }
}
