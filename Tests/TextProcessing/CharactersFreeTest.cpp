////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CharactersFreeTest.hpp"
#include "sequoia/TextProcessing/Characters.hpp"

#include <concepts>
#include <filesystem>
#include <format>
#include <string>
#include <type_traits>

namespace sequoia::testing
{
  using namespace std::string_literals;

  namespace
  {
    template<character Char>
    [[nodiscard]]
    std::string describe(std::string_view what, Char unit)
    {
      return std::format("{}: {:#x}", what, static_cast<std::make_unsigned_t<Char>>(unit));
    }

    template<class Fn>
    constexpr bool accepts_every_character_type_v{
         std::invocable<Fn, char>     && std::invocable<Fn, wchar_t>  && std::invocable<Fn, char8_t>
      && std::invocable<Fn, char16_t> && std::invocable<Fn, char32_t> && std::invocable<Fn, const char>
    };

    template<class Fn>
    constexpr bool refuses_integers_v{
      !std::invocable<Fn, int> && !std::invocable<Fn, signed char> && !std::invocable<Fn, unsigned char>
    };

    template<class Fn, class Arg, class Result>
    constexpr bool returns_v{std::same_as<std::invoke_result_t<Fn, Arg>, Result>};
  }

  [[nodiscard]]
  std::filesystem::path characters_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void characters_free_test::run_tests()
  {
    test_constraints();
    test_same_ignoring_case();
    test_character_types();
    test_classification();
    test_is_identifier_character();
    test_is_identifier_delimiter();
    test_conversion();
  }

  void characters_free_test::test_constraints()
  {
    STATIC_CHECK(accepts_every_character_type_v<decltype(is_ascii)>);
    STATIC_CHECK(accepts_every_character_type_v<decltype(ascii::is_digit)>);
    STATIC_CHECK(accepts_every_character_type_v<decltype(ascii::is_uppercase)>);
    STATIC_CHECK(accepts_every_character_type_v<decltype(ascii::is_lowercase)>);
    STATIC_CHECK(accepts_every_character_type_v<decltype(ascii::is_alphabetic)>);
    STATIC_CHECK(accepts_every_character_type_v<decltype(ascii::is_alphanumeric)>);
    STATIC_CHECK(accepts_every_character_type_v<decltype(ascii::is_hex_digit)>);
    STATIC_CHECK(accepts_every_character_type_v<decltype(ascii::is_identifier_character)>);
    STATIC_CHECK(accepts_every_character_type_v<decltype(is_identifier_delimiter)>);
    STATIC_CHECK(accepts_every_character_type_v<decltype(ascii::is_whitespace)>);
    STATIC_CHECK(accepts_every_character_type_v<decltype(ascii::to_lowercase)>);
    STATIC_CHECK(accepts_every_character_type_v<decltype(ascii::to_uppercase)>);

    STATIC_CHECK(refuses_integers_v<decltype(is_ascii)>);
    STATIC_CHECK(refuses_integers_v<decltype(ascii::is_digit)>);
    STATIC_CHECK(refuses_integers_v<decltype(ascii::is_uppercase)>);
    STATIC_CHECK(refuses_integers_v<decltype(ascii::is_lowercase)>);
    STATIC_CHECK(refuses_integers_v<decltype(ascii::is_alphabetic)>);
    STATIC_CHECK(refuses_integers_v<decltype(ascii::is_alphanumeric)>);
    STATIC_CHECK(refuses_integers_v<decltype(ascii::is_hex_digit)>);
    STATIC_CHECK(refuses_integers_v<decltype(ascii::is_identifier_character)>);
    STATIC_CHECK(refuses_integers_v<decltype(is_identifier_delimiter)>);
    STATIC_CHECK(refuses_integers_v<decltype(ascii::is_whitespace)>);
    STATIC_CHECK(refuses_integers_v<decltype(ascii::to_lowercase)>);
    STATIC_CHECK(refuses_integers_v<decltype(ascii::to_uppercase)>);

    // A converted character keeps its type; a converted string is a std::basic_string of its character type
    STATIC_CHECK(returns_v<decltype(ascii::to_lowercase), char16_t,              char16_t>);
    STATIC_CHECK(returns_v<decltype(ascii::to_uppercase), const char32_t&,       char32_t>);
    STATIC_CHECK(returns_v<decltype(ascii::to_lowercase), std::string&,          std::string>);
    STATIC_CHECK(returns_v<decltype(ascii::to_lowercase), const std::string&,    std::string>);
    STATIC_CHECK(returns_v<decltype(ascii::to_lowercase), std::string,           std::string>);
    STATIC_CHECK(returns_v<decltype(ascii::to_lowercase), std::string_view,      std::string>);
    STATIC_CHECK(returns_v<decltype(ascii::to_lowercase), const char(&)[4],      std::string>);
    STATIC_CHECK(returns_v<decltype(ascii::to_lowercase), const char*,           std::string>);
    STATIC_CHECK(returns_v<decltype(ascii::to_lowercase), std::u16string_view,   std::u16string>);
    STATIC_CHECK(returns_v<decltype(ascii::to_lowercase), const char16_t(&)[4],  std::u16string>);
    STATIC_CHECK(returns_v<decltype(ascii::to_uppercase), std::string&,          std::string>);
    STATIC_CHECK(returns_v<decltype(ascii::to_uppercase), const std::string&,    std::string>);
    STATIC_CHECK(returns_v<decltype(ascii::to_uppercase), std::string,           std::string>);
    STATIC_CHECK(returns_v<decltype(ascii::to_uppercase), const std::u32string&, std::u32string>);
    STATIC_CHECK(returns_v<decltype(ascii::to_uppercase), const wchar_t*,        std::wstring>);

    STATIC_CHECK( std::invocable<decltype(ascii::same_ignoring_case), std::string_view,   std::string_view>);
    STATIC_CHECK( std::invocable<decltype(ascii::same_ignoring_case), const std::string&, const char(&)[4]>);
    STATIC_CHECK( std::invocable<decltype(ascii::same_ignoring_case), std::u16string_view, const char16_t*>);
    STATIC_CHECK(!std::invocable<decltype(ascii::same_ignoring_case), std::string_view,   std::u16string_view>);
    STATIC_CHECK(!std::invocable<decltype(ascii::same_ignoring_case), std::u16string_view, std::string_view>);
    STATIC_CHECK(!std::invocable<decltype(ascii::same_ignoring_case), char,               char>);

    // Strings of integers, a path, and a string of a volatile type, are refused without a hard error
    STATIC_CHECK(!std::invocable<decltype(ascii::to_lowercase),        const unsigned char*>);
    STATIC_CHECK(!std::invocable<decltype(ascii::to_lowercase),        const signed char*>);
    STATIC_CHECK(!std::invocable<decltype(ascii::to_lowercase),        const std::filesystem::path&>);
    STATIC_CHECK(!std::invocable<decltype(ascii::to_lowercase),        const volatile char*>);
    STATIC_CHECK(!std::invocable<decltype(ascii::same_ignoring_case), const std::filesystem::path&, std::string_view>);
  }

  void characters_free_test::test_same_ignoring_case()
  {
    STATIC_CHECK( ascii::same_ignoring_case("", ""));
    STATIC_CHECK( ascii::same_ignoring_case("FooBar", "fOObAR"));
    STATIC_CHECK( ascii::same_ignoring_case("fOObAR", "FooBar"));
    STATIC_CHECK(!ascii::same_ignoring_case("Foo", "Foob"));
    STATIC_CHECK(!ascii::same_ignoring_case("Foob", "Foo"));
    STATIC_CHECK(!ascii::same_ignoring_case("", "a"));
    STATIC_CHECK(!ascii::same_ignoring_case("a", ""));
    STATIC_CHECK(!ascii::same_ignoring_case("Fop", "foo"));

    // Non-letters 0x20 apart, as a letter's two cases are
    STATIC_CHECK(!ascii::same_ignoring_case("[", "{"));
    STATIC_CHECK(!ascii::same_ignoring_case("@", "`"));

    // UTF-8's capital and small e with acute
    STATIC_CHECK(!ascii::same_ignoring_case("\xC3\x89", "\xC3\xA9"));
    STATIC_CHECK(!ascii::same_ignoring_case("\xC3\xA9", "\xC3\x89"));
    STATIC_CHECK( ascii::same_ignoring_case("\xC3\x89" "A", "\xC3\x89" "a"));

    STATIC_CHECK( ascii::same_ignoring_case(u"FooBar", u"fOObAR"));
    STATIC_CHECK( ascii::same_ignoring_case(std::u16string_view{u"fOObAR"}, u"FooBar"));
    STATIC_CHECK( ascii::same_ignoring_case(u"", u""));
    STATIC_CHECK(!ascii::same_ignoring_case(u"", u"a"));
    STATIC_CHECK(!ascii::same_ignoring_case(u"Foo", u"fOOb"));
    STATIC_CHECK(!ascii::same_ignoring_case(u"fOOb", u"Foo"));
    STATIC_CHECK(!ascii::same_ignoring_case(u"\u00c9", u"\u00e9"));
    STATIC_CHECK(!ascii::same_ignoring_case(u"\u00e9", u"\u00c9"));
    // A code unit whose low byte is `S`
    STATIC_CHECK(!ascii::same_ignoring_case(u"\u0153", u"s"));
    STATIC_CHECK(!ascii::same_ignoring_case(u"s", u"\u0153"));
    STATIC_CHECK( ascii::same_ignoring_case(std::string{"Tests/Foo.cpp"}, "tests/foo.CPP"));
  }

  void characters_free_test::test_character_types()
  {
    STATIC_CHECK( ascii::is_digit(u'7'));
    STATIC_CHECK( ascii::is_uppercase(U'Q'));
    STATIC_CHECK( ascii::is_lowercase(L'q'));
    STATIC_CHECK( ascii::is_whitespace(u8'\t'));
    STATIC_CHECK( ascii::is_hex_digit(u'F'));
    STATIC_CHECK( ascii::is_identifier_character(U'_'));
    STATIC_CHECK( is_identifier_delimiter(u'<'));
    STATIC_CHECK( is_ascii(u'\x7f'));
    STATIC_CHECK(!is_ascii(u'\x80'));
    STATIC_CHECK(!is_ascii(static_cast<wchar_t>(-1)));
    STATIC_CHECK(ascii::to_lowercase(u'A') == u'a');
    STATIC_CHECK(ascii::to_uppercase(U'z') == U'Z');
    STATIC_CHECK(ascii::to_lowercase(L'Q') == L'q');
    STATIC_CHECK(ascii::to_uppercase(u8'q') == u8'Q');
    STATIC_CHECK(ascii::to_lowercase(u"FooBAR") == u"foobar");
    STATIC_CHECK(ascii::to_uppercase(std::u32string_view{U"foo\u00e9"}) == U"FOO\u00e9");

    // A code point beyond the basic multilingual plane, and one whose low byte is `a`
    STATIC_CHECK(!ascii::is_alphanumeric(U'\U0001F600'));
    STATIC_CHECK(!ascii::is_alphanumeric(U'\U00010061'));
    STATIC_CHECK(ascii::to_uppercase(U'\U00010061') == U'\U00010061');

    // An accented letter, a surrogate, and code units whose low bytes are `a`, a tab and a digit
    for(const char16_t unit : {u'\u00e9', u'\xd800', u'\u0161', u'\u0109', u'\u0131'})
    {
      check(describe("A code unit beyond ASCII is not ASCII",                   unit), !is_ascii(unit));
      check(describe("A code unit beyond ASCII is not a letter",                unit), !ascii::is_alphabetic(unit));
      check(describe("A code unit beyond ASCII is not alphanumeric",            unit), !ascii::is_alphanumeric(unit));
      check(describe("A code unit beyond ASCII is not a digit",                 unit), !ascii::is_digit(unit));
      check(describe("A code unit beyond ASCII is not a hexadecimal digit",     unit), !ascii::is_hex_digit(unit));
      check(describe("A code unit beyond ASCII is not uppercase",               unit), !ascii::is_uppercase(unit));
      check(describe("A code unit beyond ASCII is not lowercase",               unit), !ascii::is_lowercase(unit));
      check(describe("A code unit beyond ASCII is not whitespace",              unit), !ascii::is_whitespace(unit));
      check(describe("A code unit beyond ASCII is not an identifier character", unit),
            !ascii::is_identifier_character(unit));
      check(describe("A code unit beyond ASCII is not an identifier delimiter", unit), !is_identifier_delimiter(unit));
      check(equality, describe("A code unit beyond ASCII to lowercase", unit), ascii::to_lowercase(unit), unit);
      check(equality, describe("A code unit beyond ASCII to uppercase", unit), ascii::to_uppercase(unit), unit);
    }
  }

  void characters_free_test::test_classification()
  {
    // Either side of the ends of each range
    STATIC_CHECK( is_ascii('\0'));
    STATIC_CHECK( is_ascii('\x7f'));
    STATIC_CHECK(!is_ascii('\x80'));
    STATIC_CHECK( ascii::is_digit('0'));
    STATIC_CHECK( ascii::is_digit('9'));
    STATIC_CHECK(!ascii::is_digit('/'));
    STATIC_CHECK(!ascii::is_digit(':'));
    STATIC_CHECK( ascii::is_uppercase('A'));
    STATIC_CHECK( ascii::is_uppercase('Z'));
    STATIC_CHECK(!ascii::is_uppercase('@'));
    STATIC_CHECK(!ascii::is_uppercase('['));
    STATIC_CHECK( ascii::is_lowercase('a'));
    STATIC_CHECK( ascii::is_lowercase('z'));
    STATIC_CHECK(!ascii::is_lowercase('`'));
    STATIC_CHECK(!ascii::is_lowercase('{'));
    STATIC_CHECK( ascii::is_alphabetic('A'));
    STATIC_CHECK( ascii::is_alphabetic('Z'));
    STATIC_CHECK( ascii::is_alphabetic('a'));
    STATIC_CHECK( ascii::is_alphabetic('z'));
    STATIC_CHECK(!ascii::is_alphabetic('@'));
    STATIC_CHECK(!ascii::is_alphabetic('['));
    STATIC_CHECK(!ascii::is_alphabetic('`'));
    STATIC_CHECK(!ascii::is_alphabetic('{'));
    STATIC_CHECK( ascii::is_alphanumeric('0'));
    STATIC_CHECK( ascii::is_alphanumeric('9'));
    STATIC_CHECK( ascii::is_alphanumeric('A'));
    STATIC_CHECK( ascii::is_alphanumeric('z'));
    STATIC_CHECK(!ascii::is_alphanumeric('/'));
    STATIC_CHECK(!ascii::is_alphanumeric(':'));
    STATIC_CHECK(!ascii::is_alphanumeric('_'));
    STATIC_CHECK( ascii::is_hex_digit('0'));
    STATIC_CHECK( ascii::is_hex_digit('9'));
    STATIC_CHECK(!ascii::is_hex_digit('/'));
    STATIC_CHECK(!ascii::is_hex_digit(':'));
    STATIC_CHECK( ascii::is_hex_digit('a'));
    STATIC_CHECK( ascii::is_hex_digit('f'));
    STATIC_CHECK(!ascii::is_hex_digit('`'));
    STATIC_CHECK(!ascii::is_hex_digit('g'));
    STATIC_CHECK( ascii::is_hex_digit('A'));
    STATIC_CHECK( ascii::is_hex_digit('F'));
    STATIC_CHECK(!ascii::is_hex_digit('@'));
    STATIC_CHECK(!ascii::is_hex_digit('G'));
    STATIC_CHECK( ascii::is_whitespace('\t'));
    STATIC_CHECK( ascii::is_whitespace('\n'));
    STATIC_CHECK( ascii::is_whitespace('\v'));
    STATIC_CHECK( ascii::is_whitespace('\f'));
    STATIC_CHECK( ascii::is_whitespace('\r'));
    STATIC_CHECK(!ascii::is_whitespace('\b'));
    STATIC_CHECK(!ascii::is_whitespace('\x0e'));
    STATIC_CHECK( ascii::is_whitespace(' '));
    STATIC_CHECK(!ascii::is_whitespace('\x1f'));
    STATIC_CHECK(!ascii::is_whitespace('!'));
    // Latin-1's next line and no-break space, which some locales count as whitespace
    STATIC_CHECK(!ascii::is_whitespace('\x85'));
    STATIC_CHECK(!ascii::is_whitespace('\xa0'));

    STATIC_CHECK(!ascii::is_alphabetic('0'));
    STATIC_CHECK(!ascii::is_digit('a'));
    STATIC_CHECK(!ascii::is_uppercase('a'));
    STATIC_CHECK(!ascii::is_lowercase('A'));

    for(const char byte : {'\x80', '\xc3', '\xff'})
    {
      check(describe("A byte beyond ASCII is not a letter",            byte), !ascii::is_alphabetic(byte));
      check(describe("A byte beyond ASCII is not alphanumeric",        byte), !ascii::is_alphanumeric(byte));
      check(describe("A byte beyond ASCII is not a digit",             byte), !ascii::is_digit(byte));
      check(describe("A byte beyond ASCII is not a hexadecimal digit", byte), !ascii::is_hex_digit(byte));
      check(describe("A byte beyond ASCII is not uppercase",           byte), !ascii::is_uppercase(byte));
      check(describe("A byte beyond ASCII is not lowercase",           byte), !ascii::is_lowercase(byte));
      check(describe("A byte beyond ASCII is not whitespace",          byte), !ascii::is_whitespace(byte));
    }
  }

  void characters_free_test::test_is_identifier_character()
  {
    STATIC_CHECK(ascii::is_identifier_character('a'));
    STATIC_CHECK(ascii::is_identifier_character('z'));
    STATIC_CHECK(ascii::is_identifier_character('A'));
    STATIC_CHECK(ascii::is_identifier_character('Z'));
    STATIC_CHECK(ascii::is_identifier_character('0'));
    STATIC_CHECK(ascii::is_identifier_character('9'));
    STATIC_CHECK(ascii::is_identifier_character('_'));

    // Either side of each range, then characters of no range, bytes beyond ASCII among them
    STATIC_CHECK(!ascii::is_identifier_character('`'));
    STATIC_CHECK(!ascii::is_identifier_character('{'));
    STATIC_CHECK(!ascii::is_identifier_character('@'));
    STATIC_CHECK(!ascii::is_identifier_character('['));
    STATIC_CHECK(!ascii::is_identifier_character('/'));
    STATIC_CHECK(!ascii::is_identifier_character(':'));
    STATIC_CHECK(!ascii::is_identifier_character('-'));
    STATIC_CHECK(!ascii::is_identifier_character('\0'));
    STATIC_CHECK(!ascii::is_identifier_character(static_cast<char>(0x80)));
    STATIC_CHECK(!ascii::is_identifier_character(static_cast<char>(0xff)));
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
    STATIC_CHECK(ascii::to_lowercase('@') == '@');
    STATIC_CHECK(ascii::to_lowercase('A') == 'a');
    STATIC_CHECK(ascii::to_lowercase('Z') == 'z');
    STATIC_CHECK(ascii::to_lowercase('[') == '[');
    STATIC_CHECK(ascii::to_uppercase('`') == '`');
    STATIC_CHECK(ascii::to_uppercase('a') == 'A');
    STATIC_CHECK(ascii::to_uppercase('z') == 'Z');
    STATIC_CHECK(ascii::to_uppercase('{') == '{');

    STATIC_CHECK(ascii::to_lowercase(std::string_view{"FooBAR"}) == "foobar");
    STATIC_CHECK(ascii::to_uppercase(std::string_view{"FooBAR"}) == "FOOBAR");

    check(equality, "An uppercase letter to lowercase", ascii::to_lowercase('A'), 'a');
    check(equality, "A lowercase letter to lowercase",  ascii::to_lowercase('a'), 'a');
    check(equality, "A digit to lowercase",             ascii::to_lowercase('1'), '1');
    check(equality, "A lowercase letter to uppercase",  ascii::to_uppercase('a'), 'A');
    check(equality, "An uppercase letter to uppercase", ascii::to_uppercase('A'), 'A');
    check(equality, "A digit to uppercase",             ascii::to_uppercase('1'), '1');

    for(const char byte : {'\x80', '\xc3', '\xff'})
    {
      check(equality, describe("A byte beyond ASCII to lowercase", byte), ascii::to_lowercase(byte), byte);
      check(equality, describe("A byte beyond ASCII to uppercase", byte), ascii::to_uppercase(byte), byte);
    }

    check(equality, "Lowercase of an empty string",        ascii::to_lowercase(""),                ""s);
    check(equality, "Lowercase of a letter",               ascii::to_lowercase("A"),               "a"s);
    check(equality, "Lowercase of mixed case",             ascii::to_lowercase("FooBAR"),          "foobar"s);
    check(equality, "Lowercase leaves other characters",   ascii::to_lowercase("Tests/Foo_1.cpp"), "tests/foo_1.cpp"s);
    check(equality, "Lowercase leaves bytes beyond ASCII", ascii::to_lowercase("\xC3\x89" "A"),    "\xC3\x89" "a"s);

    check(equality, "Uppercase of an empty string",        ascii::to_uppercase(""),                ""s);
    check(equality, "Uppercase of a letter",               ascii::to_uppercase("a"),               "A"s);
    check(equality, "Uppercase of mixed case",             ascii::to_uppercase("FooBAR"),          "FOOBAR"s);
    check(equality, "Uppercase leaves other characters",   ascii::to_uppercase("Tests/Foo_1.cpp"), "TESTS/FOO_1.CPP"s);
    check(equality, "Uppercase leaves bytes beyond ASCII", ascii::to_uppercase("\xC3\xA9" "a"),    "\xC3\xA9" "A"s);

    std::string original{"FooBAR"};
    check(equality, "Lowercase of a modifiable string", ascii::to_lowercase(original), "foobar"s);
    check(equality, "Uppercase of a modifiable string", ascii::to_uppercase(original), "FOOBAR"s);
    check(equality, "A modifiable string is left as it is", original, "FooBAR"s);
  }
}
