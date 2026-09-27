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

namespace sequoia::testing
{
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
    test_conversion();
  }

  void characters_free_test::test_constraints()
  {
    STATIC_CHECK( std::invocable<decltype(is_digit),     char>);
    STATIC_CHECK(!std::invocable<decltype(is_digit),     wchar_t>);
    STATIC_CHECK(!std::invocable<decltype(is_digit),     int>);
    STATIC_CHECK( std::invocable<decltype(to_lowercase), char>);
    STATIC_CHECK(!std::invocable<decltype(to_lowercase), char16_t>);
  }

  void characters_free_test::test_classification()
  {
    check("An ASCII character",                   is_ascii('\x7f'));
    check("A byte beyond ASCII",                 !is_ascii('\x80'));
    check("A letter",                             is_alphabetic('a'));
    check("A digit is not a letter",             !is_alphabetic('1'));
    check("A letter is alphanumeric",             is_alphanumeric('Z'));
    check("A digit is alphanumeric",              is_alphanumeric('0'));
    check("An underscore is not alphanumeric",   !is_alphanumeric('_'));
    check("A decimal digit",                      is_digit('9'));
    check("A hexadecimal letter is not a digit", !is_digit('a'));
    check("A lowercase hexadecimal digit",        is_hex_digit('f'));
    check("An uppercase hexadecimal digit",       is_hex_digit('F'));
    check("A letter beyond f",                   !is_hex_digit('g'));
    check("An uppercase letter",                  is_uppercase('Q'));
    check("A lowercase letter is not uppercase", !is_uppercase('q'));

    for(const char byte : {'\x80', '\xc3', '\xff'})
    {
      check(describe("A byte beyond ASCII is not a letter",            byte), !is_alphabetic(byte));
      check(describe("A byte beyond ASCII is not alphanumeric",        byte), !is_alphanumeric(byte));
      check(describe("A byte beyond ASCII is not a digit",             byte), !is_digit(byte));
      check(describe("A byte beyond ASCII is not a hexadecimal digit", byte), !is_hex_digit(byte));
      check(describe("A byte beyond ASCII is not uppercase",           byte), !is_uppercase(byte));
    }
  }

  void characters_free_test::test_conversion()
  {
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
  }
}
