////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CharactersFreeTest.hpp"
#include "sequoia/TextProcessing/Characters.hpp"

#include <concepts>
#include <format>
#include <string>
#include <type_traits>

namespace sequoia::testing
{
  using namespace std::string_literals;

  namespace
  {
    [[nodiscard]]
    std::string describe(std::string_view what, char byte)
    {
      return std::format("{}: {:#x}", what, static_cast<unsigned char>(byte));
    }
  }

  [[nodiscard]]
  std::filesystem::path characters_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void characters_free_test::run_tests()
  {
    test_constraints();
    test_classification();
    test_is_identifier_character();
    test_is_identifier_delimiter();
    test_conversion();
  }

  void characters_free_test::test_constraints()
  {
    STATIC_CHECK( std::invocable<decltype(is_digit),                char>);
    STATIC_CHECK(!std::invocable<decltype(is_digit),                wchar_t>);
    STATIC_CHECK(!std::invocable<decltype(is_digit),                int>);
    STATIC_CHECK( std::invocable<decltype(is_identifier_character), char>);
    STATIC_CHECK(!std::invocable<decltype(is_identifier_character), int>);
    STATIC_CHECK( std::invocable<decltype(is_identifier_delimiter), char>);
    STATIC_CHECK(!std::invocable<decltype(is_identifier_delimiter), int>);
    STATIC_CHECK( std::invocable<decltype(is_whitespace),           char>);
    STATIC_CHECK(!std::invocable<decltype(is_whitespace),           int>);
    STATIC_CHECK( std::invocable<decltype(to_lowercase),            char>);
    STATIC_CHECK(!std::invocable<decltype(to_lowercase),            char16_t>);
    STATIC_CHECK(!std::invocable<decltype(to_lowercase),            int>);
    STATIC_CHECK( std::invocable<decltype(to_uppercase),            char>);
    STATIC_CHECK(!std::invocable<decltype(to_uppercase),            char16_t>);
    STATIC_CHECK(!std::invocable<decltype(to_uppercase),            int>);

    // Every conversion of a string is returned as a std::string
    STATIC_CHECK(std::same_as<std::invoke_result_t<decltype(to_lowercase), std::string&>,       std::string>);
    STATIC_CHECK(std::same_as<std::invoke_result_t<decltype(to_lowercase), const std::string&>, std::string>);
    STATIC_CHECK(std::same_as<std::invoke_result_t<decltype(to_lowercase), std::string>,        std::string>);
    STATIC_CHECK(std::same_as<std::invoke_result_t<decltype(to_uppercase), std::string&>,       std::string>);
    STATIC_CHECK(std::same_as<std::invoke_result_t<decltype(to_uppercase), const std::string&>, std::string>);
    STATIC_CHECK(std::same_as<std::invoke_result_t<decltype(to_uppercase), std::string>,        std::string>);
  }

  void characters_free_test::test_classification()
  {
    // Either side of the ends of each range
    STATIC_CHECK( is_ascii('\0'));
    STATIC_CHECK( is_ascii('\x7f'));
    STATIC_CHECK(!is_ascii('\x80'));
    STATIC_CHECK( is_digit('0'));
    STATIC_CHECK( is_digit('9'));
    STATIC_CHECK(!is_digit('/'));
    STATIC_CHECK(!is_digit(':'));
    STATIC_CHECK( is_uppercase('A'));
    STATIC_CHECK( is_uppercase('Z'));
    STATIC_CHECK(!is_uppercase('@'));
    STATIC_CHECK(!is_uppercase('['));
    STATIC_CHECK( is_lowercase('a'));
    STATIC_CHECK( is_lowercase('z'));
    STATIC_CHECK(!is_lowercase('`'));
    STATIC_CHECK(!is_lowercase('{'));
    STATIC_CHECK( is_alphabetic('A'));
    STATIC_CHECK( is_alphabetic('Z'));
    STATIC_CHECK( is_alphabetic('a'));
    STATIC_CHECK( is_alphabetic('z'));
    STATIC_CHECK(!is_alphabetic('@'));
    STATIC_CHECK(!is_alphabetic('['));
    STATIC_CHECK(!is_alphabetic('`'));
    STATIC_CHECK(!is_alphabetic('{'));
    STATIC_CHECK( is_alphanumeric('0'));
    STATIC_CHECK( is_alphanumeric('9'));
    STATIC_CHECK( is_alphanumeric('A'));
    STATIC_CHECK( is_alphanumeric('z'));
    STATIC_CHECK(!is_alphanumeric('/'));
    STATIC_CHECK(!is_alphanumeric(':'));
    STATIC_CHECK(!is_alphanumeric('_'));
    STATIC_CHECK( is_hex_digit('0'));
    STATIC_CHECK( is_hex_digit('9'));
    STATIC_CHECK(!is_hex_digit('/'));
    STATIC_CHECK(!is_hex_digit(':'));
    STATIC_CHECK( is_hex_digit('a'));
    STATIC_CHECK( is_hex_digit('f'));
    STATIC_CHECK(!is_hex_digit('`'));
    STATIC_CHECK(!is_hex_digit('g'));
    STATIC_CHECK( is_hex_digit('A'));
    STATIC_CHECK( is_hex_digit('F'));
    STATIC_CHECK(!is_hex_digit('@'));
    STATIC_CHECK(!is_hex_digit('G'));
    STATIC_CHECK( is_whitespace('\t'));
    STATIC_CHECK( is_whitespace('\n'));
    STATIC_CHECK( is_whitespace('\v'));
    STATIC_CHECK( is_whitespace('\f'));
    STATIC_CHECK( is_whitespace('\r'));
    STATIC_CHECK(!is_whitespace('\b'));
    STATIC_CHECK(!is_whitespace('\x0e'));
    STATIC_CHECK( is_whitespace(' '));
    STATIC_CHECK(!is_whitespace('\x1f'));
    STATIC_CHECK(!is_whitespace('!'));
    // Latin-1's next line and no-break space, which some locales count as whitespace
    STATIC_CHECK(!is_whitespace('\x85'));
    STATIC_CHECK(!is_whitespace('\xa0'));

    STATIC_CHECK(!is_alphabetic('0'));
    STATIC_CHECK(!is_digit('a'));
    STATIC_CHECK(!is_uppercase('a'));
    STATIC_CHECK(!is_lowercase('A'));

    for(const char byte : {'\x80', '\xc3', '\xff'})
    {
      check(describe("A byte beyond ASCII is not a letter",            byte), !is_alphabetic(byte));
      check(describe("A byte beyond ASCII is not alphanumeric",        byte), !is_alphanumeric(byte));
      check(describe("A byte beyond ASCII is not a digit",             byte), !is_digit(byte));
      check(describe("A byte beyond ASCII is not a hexadecimal digit", byte), !is_hex_digit(byte));
      check(describe("A byte beyond ASCII is not uppercase",           byte), !is_uppercase(byte));
      check(describe("A byte beyond ASCII is not lowercase",           byte), !is_lowercase(byte));
      check(describe("A byte beyond ASCII is not whitespace",          byte), !is_whitespace(byte));
    }
  }

  void characters_free_test::test_is_identifier_character()
  {
    STATIC_CHECK(is_identifier_character('a'));
    STATIC_CHECK(is_identifier_character('z'));
    STATIC_CHECK(is_identifier_character('A'));
    STATIC_CHECK(is_identifier_character('Z'));
    STATIC_CHECK(is_identifier_character('0'));
    STATIC_CHECK(is_identifier_character('9'));
    STATIC_CHECK(is_identifier_character('_'));

    // Either side of each range, then characters of no range, bytes beyond ASCII among them
    STATIC_CHECK(!is_identifier_character('`'));
    STATIC_CHECK(!is_identifier_character('{'));
    STATIC_CHECK(!is_identifier_character('@'));
    STATIC_CHECK(!is_identifier_character('['));
    STATIC_CHECK(!is_identifier_character('/'));
    STATIC_CHECK(!is_identifier_character(':'));
    STATIC_CHECK(!is_identifier_character('-'));
    STATIC_CHECK(!is_identifier_character('\0'));
    STATIC_CHECK(!is_identifier_character(static_cast<char>(0x80)));
    STATIC_CHECK(!is_identifier_character(static_cast<char>(0xff)));
  }

  void characters_free_test::test_is_identifier_delimiter()
  {
    STATIC_CHECK(!is_identifier_delimiter('_'));
    STATIC_CHECK(!is_identifier_delimiter('a'));
    STATIC_CHECK(!is_identifier_delimiter('0'));
    STATIC_CHECK( is_identifier_delimiter('<'));
    STATIC_CHECK( is_identifier_delimiter(' '));
    STATIC_CHECK( is_identifier_delimiter('\0'));
    STATIC_CHECK( is_identifier_delimiter('\x7f'));
    STATIC_CHECK(!is_identifier_delimiter(static_cast<char>(0x80)));
    STATIC_CHECK(!is_identifier_delimiter(static_cast<char>(0xc3)));
    STATIC_CHECK(!is_identifier_delimiter(static_cast<char>(0xff)));
  }

  void characters_free_test::test_conversion()
  {
    // Either side of each range of letters
    STATIC_CHECK(to_lowercase('@') == '@');
    STATIC_CHECK(to_lowercase('A') == 'a');
    STATIC_CHECK(to_lowercase('Z') == 'z');
    STATIC_CHECK(to_lowercase('[') == '[');
    STATIC_CHECK(to_uppercase('`') == '`');
    STATIC_CHECK(to_uppercase('a') == 'A');
    STATIC_CHECK(to_uppercase('z') == 'Z');
    STATIC_CHECK(to_uppercase('{') == '{');

    STATIC_CHECK(to_lowercase(std::string_view{"FooBAR"}) == "foobar");
    STATIC_CHECK(to_uppercase(std::string_view{"FooBAR"}) == "FOOBAR");

    check(equality, "An uppercase letter to lowercase", to_lowercase('A'), 'a');
    check(equality, "A lowercase letter to lowercase",  to_lowercase('a'), 'a');
    check(equality, "A digit to lowercase",             to_lowercase('1'), '1');
    check(equality, "A lowercase letter to uppercase",  to_uppercase('a'), 'A');
    check(equality, "An uppercase letter to uppercase", to_uppercase('A'), 'A');
    check(equality, "A digit to uppercase",             to_uppercase('1'), '1');

    for(const char byte : {'\x80', '\xc3', '\xff'})
    {
      check(equality, describe("A byte beyond ASCII to lowercase", byte), to_lowercase(byte), byte);
      check(equality, describe("A byte beyond ASCII to uppercase", byte), to_uppercase(byte), byte);
    }

    check(equality, "The lowercase of an empty string",        to_lowercase(""),                ""s);
    check(equality, "The lowercase of a letter",               to_lowercase("A"),               "a"s);
    check(equality, "The lowercase of mixed case",             to_lowercase("FooBAR"),          "foobar"s);
    check(equality, "The lowercase leaves other characters",   to_lowercase("Tests/Foo_1.cpp"), "tests/foo_1.cpp"s);
    check(equality, "The lowercase leaves bytes beyond ASCII", to_lowercase("\xC3\x89" "A"),    "\xC3\x89" "a"s);

    check(equality, "The uppercase of an empty string",        to_uppercase(""),                ""s);
    check(equality, "The uppercase of a letter",               to_uppercase("a"),               "A"s);
    check(equality, "The uppercase of mixed case",             to_uppercase("FooBAR"),          "FOOBAR"s);
    check(equality, "The uppercase leaves other characters",   to_uppercase("Tests/Foo_1.cpp"), "TESTS/FOO_1.CPP"s);
    check(equality, "The uppercase leaves bytes beyond ASCII", to_uppercase("\xC3\xA9" "a"),    "\xC3\xA9" "A"s);

    std::string original{"FooBAR"};
    check(equality, "The lowercase of a modifiable string", to_lowercase(original), "foobar"s);
    check(equality, "The uppercase of a modifiable string", to_uppercase(original), "FOOBAR"s);
    check(equality, "A modifiable string is left as it is", original, "FooBAR"s);
  }
}
