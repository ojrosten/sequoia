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
    check_equality(LINE("Camel from empty string"), to_camel_case(""), ""s);
    check_equality(LINE("Camel from letter"), to_camel_case("a"), "A"s);
    check_equality(LINE("Camel from minimal snake"), to_camel_case("a_b"), "AB"s);
    check_equality(LINE(""), to_camel_case("foo_bar_baz"), "FooBarBaz"s);
  }

  void output_free_test::test_snake_case()
  {
    using namespace std::string_literals;
    check_equality(LINE("Snake from empty string"), to_snake_case(""), ""s);
    check_equality(LINE("Snake from letter"), to_snake_case("A"), "a"s);
    check_equality(LINE("Snake from minimal camel"), to_snake_case("AB"), "a_b"s);
    check_equality(LINE(""), to_snake_case("FooBarBaz"), "foo_bar_baz"s);
  }
  

  void output_free_test::test_capitalize()
  {
    using namespace std::string_literals;
    check_equality(LINE("Capitalize empty string"), capitalize(""), ""s);
    check_equality(LINE("Capitalize letter"), capitalize("a"), "A"s);
    check_equality(LINE("Capitalize word"), capitalize("foo"), "Foo"s);
  }

  void output_free_test::test_replace()
  {
  }
}
