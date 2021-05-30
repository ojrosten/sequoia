////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "SubstitutionsFreeTest.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view substitutions_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void substitutions_free_test::run_tests()
  {
    test_camel_case();
    test_snake_case();
    test_capitalize();
    test_uncapitalize();
    test_replace();
    test_replace_all();
  }

  void substitutions_free_test::test_camel_case()
  {
    using namespace std::string_literals;
    check_equality(LINE("Camel from empty string"), to_camel_case(""), ""s);
    check_equality(LINE("Camel from letter"), to_camel_case("a"), "A"s);
    check_equality(LINE("Camel from minimal snake"), to_camel_case("a_b"), "AB"s);
    check_equality(LINE(""), to_camel_case("foo_bar_baz"), "FooBarBaz"s);
  }

  void substitutions_free_test::test_snake_case()
  {
    using namespace std::string_literals;
    check_equality(LINE("Snake from empty string"), to_snake_case(""), ""s);
    check_equality(LINE("Snake from letter"), to_snake_case("A"), "a"s);
    check_equality(LINE("Snake from minimal camel"), to_snake_case("AB"), "a_b"s);
    check_equality(LINE(""), to_snake_case("FooBarBaz"), "foo_bar_baz"s);
  }

  void substitutions_free_test::test_capitalize()
  {
    using namespace std::string_literals;
    check_equality(LINE("Capitalize empty string"), capitalize(""), ""s);
    check_equality(LINE("Capitalize letter"), capitalize("a"), "A"s);
    check_equality(LINE("Capitalize word"), capitalize("foo"), "Foo"s);
  }

  void substitutions_free_test::test_uncapitalize()
  {
    using namespace std::string_literals;
    check_equality(LINE("Uncapitalize empty string"), uncapitalize(""), ""s);
    check_equality(LINE("Uncapitalize letter"), uncapitalize("A"), "a"s);
    check_equality(LINE("Uncapitalize word"), uncapitalize("Foo"), "foo"s);
  }

  void substitutions_free_test::test_replace()
  {
    using namespace std::string_literals;
    check_equality(LINE("Replace in empty string"), replace("", "foo", "bar"), ""s);
    check_equality(LINE("Single replacement"), replace("foo", "foo", "bar"), "bar"s);
    check_equality(LINE("Single replacement; multiple instances"), replace("foofoo", "foo", "bar"), "barfoo"s);
  }

  void substitutions_free_test::test_replace_all()
  {
    using namespace std::string_literals;
    check_equality(LINE("Replace in empty string"), replace_all("", "foo", "bar"), ""s);
    check_equality(LINE("Single replacement"), replace_all("foo", "foo", "bar"), "bar"s);
    check_equality(LINE("Multiple adjacent replacement"), replace_all("foofoo", "foo", "bar"), "barbar"s);
    check_equality(LINE("Multiple separated replacement"), replace_all("foo bar foo", "foo", "bar"), "bar bar bar"s);

    check_equality(LINE("Mutliple replacement patters"), replace_all("foobarbaz", {{"foo", "zoo"}, {"bar", "bfg"}, {"baz", "bat"}}), "zoobfgbat"s);

    check_equality(LINE("LR Replace in empty string"), replace_all("", ",<", "foo", ",>", "bar"), ""s);
    check_equality(LINE("LR single replacement"), replace_all(",foo,", ",<", "foo", ",>", "bar"), ",bar,"s);
    check_equality(LINE("LR single replacement"), replace_all(",foo>", ",<", "foo", ",>", "bar"), ",bar>"s);
    check_equality(LINE("LR single replacement"), replace_all("<foo,", ",<", "foo", ",>", "bar"), "<bar,"s);
    check_equality(LINE("LR single replacement"), replace_all("<foo>", ",<", "foo", ",>", "bar"), "<bar>"s);
    check_equality(LINE("LR failed replacement"), replace_all("foo,", ",<", "foo", ",>", "bar"), "foo,"s);
    check_equality(LINE("LR failed replacement"), replace_all("pfoo,", ",<", "foo", ",>", "bar"), "pfoo,"s);
    check_equality(LINE("LR failed replacement"), replace_all(",foo", ",<", "foo", ",>", "bar"), ",foo"s);
    check_equality(LINE("LR failed replacement"), replace_all(",foop", ",<", "foo", ",>", "bar"), ",foop"s);
    check_equality(LINE("LR multiple replacement"), replace_all(",foo,foo,", ",<", "foo", ",>", "bar"), ",bar,bar,"s);
    check_equality(LINE("L single replacement"), replace_all(",foo", ",<", "foo", "", "baz"), ",baz"s);
    check_equality(LINE("R single replacement"), replace_all("foo,", "", "foo", ",", "baz"), "baz,"s);
  }
}
