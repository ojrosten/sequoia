////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PatternsFreeTest.hpp"
#include "sequoia/TextProcessing/Patterns.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view patterns_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void patterns_free_test::run_tests()
  {
    test_find_delimiters();
  }

  void patterns_free_test::test_find_delimiters()
  {
    using size_type = std::string::size_type;
    using prediction = std::pair<size_type, size_type>;
    constexpr auto npos{std::string::npos};

    check(equality, LINE("Empty string"), find_matched_delimiters(""), prediction{npos, npos});
    check(equality, LINE("Only one ("), find_matched_delimiters("("), prediction{0, 0});
    check(equality, LINE("Only one )"), find_matched_delimiters(")"), prediction{npos, npos});
    check(equality, LINE("Mismatched parens"), find_matched_delimiters("(()"), prediction{0, 0});
    check(equality, LINE("Matched single parens"), find_matched_delimiters("()"), prediction{0, 2});
    check(equality, LINE("Matched separated parens"), find_matched_delimiters("a(b)"), prediction{1, 4});
    check(equality, LINE("Matched nested parens"), find_matched_delimiters("(())"), prediction{0, 4});
    check(equality, LINE("Matched parens offest"), find_matched_delimiters("()()", 1), prediction{2, 4});
    check(equality, LINE("Pos out of bounds"), find_matched_delimiters("()", 2), prediction{npos, npos});

    check(equality, LINE("Matched nested []"), find_matched_delimiters("[[]]", 0, '[', ']'), prediction{0, 4});
  }
}
