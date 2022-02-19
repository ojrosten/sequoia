////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "PatternsFreeTest.hpp"
#include "sequoia/TextProcessing/Patterns.hpp"

namespace sequoia::testing
{
  namespace
  {
    using size_type = std::string::size_type;
    using prediction = std::pair<size_type, size_type>;
    constexpr auto npos{std::string::npos};
  }

  [[nodiscard]]
  std::string_view patterns_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void patterns_free_test::run_tests()
  {
    test_find_delimiters();
    test_find_sandwiched_text();
  }

  void patterns_free_test::test_find_delimiters()
  {
    check(equality, LINE("Empty string"),             find_matched_delimiters("", '(', ')'), prediction{npos, npos});
    check(equality, LINE("Only one ("),               find_matched_delimiters("(", '(', ')'), prediction{0, 0});
    check(equality, LINE("Only one )"),               find_matched_delimiters(")", '(', ')'), prediction{npos, npos});
    check(equality, LINE("Mismatched parens"),        find_matched_delimiters("(()", '(', ')'), prediction{0, 0});
    check(equality, LINE("Matched single parens"),    find_matched_delimiters("()", '(', ')'), prediction{0, 2});
    check(equality, LINE("Matched separated parens"), find_matched_delimiters("a(b)", '(', ')'), prediction{1, 4});
    check(equality, LINE("Matched nested parens"),    find_matched_delimiters("(())", '(', ')'), prediction{0, 4});
    check(equality, LINE("Matched parens offest"),    find_matched_delimiters("()()", '(', ')', 1), prediction{2, 4});
    check(equality, LINE("Pos out of bounds"),        find_matched_delimiters("()", '(', ')', 2), prediction{npos, npos});

    check(equality, LINE("Matched nested []"),        find_matched_delimiters("[[]]", '[', ']', 0), prediction{0, 4});
  }

  void patterns_free_test::test_find_sandwiched_text()
  {
    check(equality, LINE("Empty string"),                     find_sandwiched_text("", "foo", "bar"), prediction{npos, npos});
    check(equality, LINE("Left match"),                       find_sandwiched_text("foo ", "foo", "bar"), prediction{3, npos});
    check(equality, LINE("left match, ignored right match"),  find_sandwiched_text("fo bar", "foo", "bar"), prediction{npos, npos});
    check(equality, LINE("Saturated double match"),           find_sandwiched_text("foobar", "foo", "bar"), prediction{3, 3});
    check(equality, LINE("Double match"),                     find_sandwiched_text("foo Hello bar", "foo", "bar"), prediction{3, 10});
    check(equality, LINE("Double match away from the start"), find_sandwiched_text("Other stuff\nfoo Hello bar", "foo", "bar"), prediction{15, 22});
    check(equality, LINE("Match missed due to offset"),       find_sandwiched_text("foobar", "foo", "bar", 1), prediction{npos, npos});
    check(equality, LINE("Double match with offset"),         find_sandwiched_text("foo foo Hello bar", "foo", "bar", 1), prediction{7, 14});
    check(equality, LINE("Empty string, pos out of bounds"),  find_sandwiched_text("", "foo", "bar", 1), prediction{npos, npos});
    check(equality, LINE("Pos out of bounds"),                find_sandwiched_text("foo Hello bar", "foo", "bar", 20), prediction{npos, npos});
  }
}
