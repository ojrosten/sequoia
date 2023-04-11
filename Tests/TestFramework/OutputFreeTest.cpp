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
  [[nodiscard]]
  std::string_view output_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void output_free_test::run_tests()
  {
    test_emphasise();
    test_display_character();
    test_tidy_name();
  }

  void output_free_test::test_emphasise()
  {
    using namespace std::string_literals;
    check(equality, report_line("Emphasis"), emphasise("foo"), "--foo--"s);
    check(equality, report_line("Nothing to emphasise"), emphasise(""), ""s);
  }

  void output_free_test::test_display_character()
  {
    using namespace std::string_literals;
    check(equality, report_line(""), display_character('\n'), "'\\n'"s);
    check(equality, report_line(""), display_character('\t'), "'\\t'"s);
    check(equality, report_line(""), display_character('\0'), "'\\0'"s);
    check(equality, report_line(""), display_character(' '), "' '"s);
  }

  void output_free_test::test_tidy_name()
  {
    using namespace std::string_literals;
    check(equality, report_line(""), tidy_name("(some enum)0", clang_type{}), "0"s);
    check(equality, report_line(""), tidy_name("struct foo", msvc_type{}), "foo"s);
  }
}
