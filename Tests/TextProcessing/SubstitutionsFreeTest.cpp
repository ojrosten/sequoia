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
  using namespace std::string_literals;

  [[nodiscard]]
  std::filesystem::path substitutions_free_test::source_file() const
  {
    return std::source_location::current().file_name();
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
    test_replace_all_recursive();
  }

  void substitutions_free_test::test_camel_case()
  {
    check(equality, "Camel from empty string", to_camel_case(""), ""s);
    check(equality, "Camel from letter", to_camel_case("a"), "A"s);
    check(equality, "Camel from minimal snake", to_camel_case("a_b"), "AB"s);
    check(equality, "Camel from three segment snake", to_camel_case("foo_bar_baz"), "FooBarBaz"s);

    check(equality, "Modified Camel from empty string", to_camel_case("", " "), ""s);
    check(equality, "Modified Camel from letter", to_camel_case("a", " "), "A"s);
    check(equality, "Modified Camel from minimal snake", to_camel_case("a_b", " "), "A B"s);
    check(equality, "Modified Camel from three segment snake", to_camel_case("foo_bar_baz", " "), "Foo Bar Baz"s);
  }

  void substitutions_free_test::test_camel_to_words()
  {
    check(equality, "Camel to words from empty string", camel_to_words(""), ""s);
    check(equality, "Camel to words from letter", camel_to_words("a"), "a"s);
    check(equality, "Camel to words from captial letter", camel_to_words("A"), "A"s);
    check(equality, "Camel to words from two letters", camel_to_words("aa"), "aa"s);
    check(equality, "Camel to words from minimal camel", camel_to_words("aA"), "a A"s);
    check(equality, "Camel to words exceeding capacity", camel_to_words("aA", "_____________________"), "a_____________________A"s);
  }

  void substitutions_free_test::test_snake_case()
  {
    check(equality, "Snake from empty string", to_snake_case(""), ""s);
    check(equality, "Snake from letter", to_snake_case("A"), "a"s);
    check(equality, "Snake from minimal camel", to_snake_case("AB"), "a_b"s);
    check(equality, "", to_snake_case("FooBarBaz"), "foo_bar_baz"s);
  }

  void substitutions_free_test::test_capitalize()
  {
    check(equality, "Capitalize empty string", capitalize(""), ""s);
    check(equality, "Capitalize letter", capitalize("a"), "A"s);
    check(equality, "Capitalize word", capitalize("foo"), "Foo"s);
  }

  void substitutions_free_test::test_uncapitalize()
  {
    check(equality, "Uncapitalize empty string", uncapitalize(""), ""s);
    check(equality, "Uncapitalize letter", uncapitalize("A"), "a"s);
    check(equality, "Uncapitalize word", uncapitalize("Foo"), "foo"s);
  }

  void substitutions_free_test::test_replace()
  {
    check(equality, "Replace in empty string", replace("", "foo", "bar"), ""s);
    check(equality, "Single replacement", replace("foo", "foo", "bar"), "bar"s);
    check(equality, "Single replacement; multiple instances", replace("foofoo", "foo", "bar"), "barfoo"s);
    check(equality, "Single replacement, arbitrary position", replace("barfoobaz", "foo", "bar"), "barbarbaz"s);
  }

  void substitutions_free_test::test_replace_all()
  {
    check(equality, "Replace in empty string", replace_all("", "foo", "bar"), ""s);
    check(equality, "Single replacement", replace_all("foo", "foo", "bar"), "bar"s);
    check(equality, "Multiple adjacent replacement", replace_all("foofoo", "foo", "bar"), "barbar"s);
    check(equality, "Multiple separated replacement", replace_all("foo bar foo", "foo", "bar"), "bar bar bar"s);

    check(equality, "Mutliple replacement patters", replace_all("foobarbaz", replacement{"foo", "zoo"}, replacement{"bar", "bfg"}, replacement{"baz", "bat"}), "zoobfgbat"s);

    check(equality, "LR Replace in empty string", replace_all("", ",<", "foo", ",>", "bar"), ""s);
    check(equality, "LR single replacement", replace_all(",foo,", ",<", "foo", ",>", "bar"), ",bar,"s);
    check(equality, "LR single replacement", replace_all(",foo>", ",<", "foo", ",>", "bar"), ",bar>"s);
    check(equality, "LR single replacement", replace_all("<foo,", ",<", "foo", ",>", "bar"), "<bar,"s);
    check(equality, "LR single replacement", replace_all("<foo>", ",<", "foo", ",>", "bar"), "<bar>"s);
    check(equality, "LR failed replacement", replace_all("foo,", ",<", "foo", ",>", "bar"), "foo,"s);
    check(equality, "LR failed replacement", replace_all("pfoo,", ",<", "foo", ",>", "bar"), "pfoo,"s);
    check(equality, "LR failed replacement", replace_all(",foo", ",<", "foo", ",>", "bar"), ",foo"s);
    check(equality, "LR failed replacement", replace_all(",foop", ",<", "foo", ",>", "bar"), ",foop"s);
    check(equality, "LR multiple replacement", replace_all(",foo,foo,", ",<", "foo", ",>", "bar"), ",bar,bar,"s);
    check(equality, "L single replacement", replace_all(",foo", ",<", "foo", "", "baz"), ",baz"s);
    check(equality, "R single replacement", replace_all("foo,", "", "foo", ",", "baz"), "baz,"s);
  }

  void substitutions_free_test::test_replace_all_recursive()
  {
    check(equality, "Expand 2 chevrons", replace_all_recursive(">>", ">>", "> >"), "> >"s);
    check(equality, "Expand 3 chevrons", replace_all_recursive(">>>", ">>", "> >"), "> > >"s);
    check(equality, "Expand 3 chevrons", replace_all_recursive(">>>>", ">>", "> >"), "> > > >"s);
  }
}
