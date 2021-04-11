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
  [[nodiscard]]
  std::string_view output_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void output_free_test::run_tests()
  {
    test_emphasise();
    test_camel_case();
    test_snake_case();
    test_capitalize();
    test_replace();
  }

  void output_free_test::test_emphasise()
  {
    using namespace std::string_literals;
    check_equality(LINE("Emphasis"), emphasise("foo"), "--foo--"s);
    check_equality(LINE("Nothing to emphasise"), emphasise(""), ""s);
  }

  void output_free_test::test_camel_case()
  {
    using namespace std::string_literals;
    check_equality(LINE("Camel empty string"), to_camel_case(""), ""s);
    check_equality(LINE("Camel letter"), to_camel_case("a"), "A"s);
    check_equality(LINE("Camel minimal snake"), to_camel_case("a_b"), "AB"s);
    check_equality(LINE(""), to_camel_case("foo_bar_baz"), "FooBarBaz"s);
  }

  void output_free_test::test_snake_case()
  {
  }

  void output_free_test::test_capitalize()
  {
  }

  void output_free_test::test_replace()
  {
  }
}
