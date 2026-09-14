////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "OutputFreeTest.hpp"
#include "sequoia/TestFramework/Output.hpp"

namespace sequoia::testing
{
  using namespace std::string_literals;
  namespace fs = std::filesystem;

  [[nodiscard]]
  std::filesystem::path output_free_test::source_file()
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
    check(equality, "", tidy_name("<0f>",         clang_type{}), "<0>"s);
    check(equality, "", tidy_name("<0ul>",        clang_type{}), "<0>"s);
    check(equality, "", tidy_name("<1ul, 0f>",    clang_type{}), "<1, 0>"s);
    check(equality, "", tidy_name("<2ul, 1f>",    clang_type{}), "<2, 1>"s);
    check(equality, "", tidy_name("<textued_2d>", clang_type{}), "<textued_2d>"s);
    check(equality, "", tidy_name("struct foo", msvc_type{}), "foo"s);
    check(equality, "", tidy_name("",  gcc_type{}), ""s);
    check(equality, "", tidy_name(" ", gcc_type{}), " "s);
    check(equality,
          "",
          tidy_name(std::format("<(float)[{}]>", std::format("{:X}", std::bit_cast<int>(3.14f))), gcc_type{}),
          std::format("<{:.6f}>", 3.14f)
    );

    check(equality,
          "",
          tidy_name("coordinates<my_vec_space<1ul>>",  gcc_type{}),
          "coordinates<my_vec_space<1> >"s
    );

    const std::string normalizedLong{(sizeof(unsigned long) == sizeof(unsigned long long)) ? "long long" : "long"};

    check(equality, "long",                                   tidy_name("long",             clang_type{}), normalizedLong);
    check(equality, "unsigned long",                          tidy_name("unsigned long",    clang_type{}), "unsigned " + normalizedLong);
    check(equality, "long long",                              tidy_name("long long",        clang_type{}), "long long"s);
    check(equality, "long as a template argument",            tidy_name("vector<long>",     clang_type{}), "vector<" + normalizedLong + ">");
    check(equality, "long twice",                             tidy_name("pair<long, long>", clang_type{}), "pair<" + normalizedLong + ", " + normalizedLong + ">");

    check(equality, "An identifier containing long",          tidy_name("my_long_type",     clang_type{}), "my_long_type"s);
    check(equality, "An identifier ending in long",           tidy_name("prolong",          clang_type{}), "prolong"s);
    check(equality, "An identifier beginning with long",      tidy_name("longitude",        clang_type{}), "longitude"s);
    check(equality, "An identifier containing long, beside a long", tidy_name("belongs_to<long>", clang_type{}), "belongs_to<" + normalizedLong + ">");

    // TO DO: reinstate and fix associated bug
    /*check(equality,
          "",
          tidy_name("<(float)[FF]>", gcc_type{}),
          std::format("<0.0>")
    );*/
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
