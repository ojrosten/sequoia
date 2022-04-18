////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

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
    test_camel_to_words();
    test_snake_case();
    test_capitalize();
    test_uncapitalize();
    test_replace();
    test_replace_all();
  }

  void substitutions_free_test::test_camel_case()
  {
    using namespace std::string_literals;
    check(equality, LINE("Camel from empty string"), to_camel_case(""), ""s);
    check(equality, LINE("Camel from letter"), to_camel_case("a"), "A"s);
    check(equality, LINE("Camel from minimal snake"), to_camel_case("a_b"), "AB"s);
    check(equality, LINE("Camel from three segment snake"), to_camel_case("foo_bar_baz"), "FooBarBaz"s);

    check(equality, LINE("Modified Camel from empty string"), to_camel_case("", " "), ""s);
    check(equality, LINE("Modified Camel from letter"), to_camel_case("a", " "), "A"s);
    check(equality, LINE("Modified Camel from minimal snake"), to_camel_case("a_b", " "), "A B"s);
    check(equality, LINE("Modified Camel from three segment snake"), to_camel_case("foo_bar_baz", " "), "Foo Bar Baz"s);
  }

  void substitutions_free_test::test_camel_to_words()
  {
    using namespace std::string_literals;
    check(equality, LINE("Camel to words from empty string"), camel_to_words(""), ""s);
    check(equality, LINE("Camel to words from letter"), camel_to_words("a"), "a"s);
    check(equality, LINE("Camel to words from captial letter"), camel_to_words("A"), "A"s);
    check(equality, LINE("Camel to words from two letters"), camel_to_words("aa"), "aa"s);
    check(equality, LINE("Camel to words from minimal camel"), camel_to_words("aA"), "a A"s);
    check(equality, LINE("Camel to words exceeding capacity"), camel_to_words("aA", "_____________________"), "a_____________________A"s);
  }

  void substitutions_free_test::test_snake_case()
  {
    using namespace std::string_literals;
    check(equality, LINE("Snake from empty string"), to_snake_case(""), ""s);
    check(equality, LINE("Snake from letter"), to_snake_case("A"), "a"s);
    check(equality, LINE("Snake from minimal camel"), to_snake_case("AB"), "a_b"s);
    check(equality, LINE(""), to_snake_case("FooBarBaz"), "foo_bar_baz"s);
  }

  void substitutions_free_test::test_capitalize()
  {
    using namespace std::string_literals;
    check(equality, LINE("Capitalize empty string"), capitalize(""), ""s);
    check(equality, LINE("Capitalize letter"), capitalize("a"), "A"s);
    check(equality, LINE("Capitalize word"), capitalize("foo"), "Foo"s);
  }

  void substitutions_free_test::test_uncapitalize()
  {
    using namespace std::string_literals;
    check(equality, LINE("Uncapitalize empty string"), uncapitalize(""), ""s);
    check(equality, LINE("Uncapitalize letter"), uncapitalize("A"), "a"s);
    check(equality, LINE("Uncapitalize word"), uncapitalize("Foo"), "foo"s);
  }

  void substitutions_free_test::test_replace()
  {
    using namespace std::string_literals;
    check(equality, LINE("Replace in empty string"), replace("", "foo", "bar"), ""s);
    check(equality, LINE("Single replacement"), replace("foo", "foo", "bar"), "bar"s);
    check(equality, LINE("Single replacement; multiple instances"), replace("foofoo", "foo", "bar"), "barfoo"s);
    check(equality, LINE("Single replacement, arbitrary position"), replace("barfoobaz", "foo", "bar"), "barbarbaz"s);
  }

  void substitutions_free_test::test_replace_all()
  {
    using namespace std::string_literals;
    check(equality, LINE("Replace in empty string"), replace_all("", "foo", "bar"), ""s);
    check(equality, LINE("Single replacement"), replace_all("foo", "foo", "bar"), "bar"s);
    check(equality, LINE("Multiple adjacent replacement"), replace_all("foofoo", "foo", "bar"), "barbar"s);
    check(equality, LINE("Multiple separated replacement"), replace_all("foo bar foo", "foo", "bar"), "bar bar bar"s);

    check(equality, LINE("Mutliple replacement patters"), replace_all("foobarbaz", {{"foo", "zoo"}, {"bar", "bfg"}, {"baz", "bat"}}), "zoobfgbat"s);

    check(equality, LINE("LR Replace in empty string"), replace_all("", ",<", "foo", ",>", "bar"), ""s);
    check(equality, LINE("LR single replacement"), replace_all(",foo,", ",<", "foo", ",>", "bar"), ",bar,"s);
    check(equality, LINE("LR single replacement"), replace_all(",foo>", ",<", "foo", ",>", "bar"), ",bar>"s);
    check(equality, LINE("LR single replacement"), replace_all("<foo,", ",<", "foo", ",>", "bar"), "<bar,"s);
    check(equality, LINE("LR single replacement"), replace_all("<foo>", ",<", "foo", ",>", "bar"), "<bar>"s);
    check(equality, LINE("LR failed replacement"), replace_all("foo,", ",<", "foo", ",>", "bar"), "foo,"s);
    check(equality, LINE("LR failed replacement"), replace_all("pfoo,", ",<", "foo", ",>", "bar"), "pfoo,"s);
    check(equality, LINE("LR failed replacement"), replace_all(",foo", ",<", "foo", ",>", "bar"), ",foo"s);
    check(equality, LINE("LR failed replacement"), replace_all(",foop", ",<", "foo", ",>", "bar"), ",foop"s);
    check(equality, LINE("LR multiple replacement"), replace_all(",foo,foo,", ",<", "foo", ",>", "bar"), ",bar,bar,"s);
    check(equality, LINE("L single replacement"), replace_all(",foo", ",<", "foo", "", "baz"), ",baz"s);
    check(equality, LINE("R single replacement"), replace_all("foo,", "", "foo", ",", "baz"), "baz,"s);
  }
}
