////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Classifications and conversions of a character of any character type, and conversions and comparisons of
           strings of characters.

    `is_ascii` says whether a code unit, read as unsigned, is below 0x80. Such a code unit is its ASCII character in
    UTF-8, UTF-16 and UTF-32 alike.
    The helpers in the namespace `ascii` answer for ASCII alone. No other code unit is a letter, a digit, whitespace
    or an identifier character, and a conversion leaves it as it is. `ascii::is_identifier_character` accepts `_` as
    well as the ASCII letters and digits. The whitespace characters are those of the C locale: space, horizontal tab,
    line feed, vertical tab, form feed and carriage return. A comparison ignoring case compares the lowercase forms.
    `is_identifier_delimiter` is not an ASCII classification: every code unit beyond ASCII counts as part of an
    identifier. None of these helpers depends on the locale.
 */

#include "sequoia/Core/Meta/Concepts.hpp"

#include <algorithm>
#include <concepts>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace sequoia
{
  inline constexpr auto is_ascii{
    [](character auto c){ return static_cast<std::make_unsigned_t<decltype(c)>>(c) < 0x80; }
  };

  namespace ascii
  {
    inline constexpr auto is_digit{
      [](character auto c){ return (c >= '0') && (c <= '9'); }
    };

    inline constexpr auto is_uppercase{
      [](character auto c){ return (c >= 'A') && (c <= 'Z'); }
    };

    inline constexpr auto is_lowercase{
      [](character auto c){ return (c >= 'a') && (c <= 'z'); }
    };

    inline constexpr auto is_alphabetic{
      [](character auto c){ return is_uppercase(c) || is_lowercase(c); }
    };

    inline constexpr auto is_alphanumeric{
      [](character auto c){ return is_alphabetic(c) || is_digit(c); }
    };

    inline constexpr auto is_hex_digit{
      [](character auto c){ return is_digit(c) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F')); }
    };

    inline constexpr auto is_identifier_character{
      [](character auto c){ return is_alphanumeric(c) || (c == '_'); }
    };

    inline constexpr auto is_whitespace{
      [](character auto c){ return (c == ' ') || ((c >= '\t') && (c <= '\r')); }
    };
  }

  /** Whether `c` is ASCII and not an identifier character. Every code unit beyond ASCII counts as part of an
      identifier, so an identifier holding a character that spans several code units is never split. This
      approximates C++'s rules for identifiers.
   */
  inline constexpr auto is_identifier_delimiter{
    [](character auto c){ return is_ascii(c) && !ascii::is_identifier_character(c); }
  };

  namespace impl
  {
    template<class Char>
    concept unqualified_character = character<Char> && std::same_as<Char, std::remove_cv_t<Char>>;

    template<unqualified_character Char>
    [[nodiscard]]
    constexpr std::basic_string_view<Char> as_string_view(std::basic_string_view<Char> text) noexcept
    {
      return text;
    }

    template<unqualified_character Char, class Allocator>
    [[nodiscard]]
    constexpr std::basic_string_view<Char>
      as_string_view(const std::basic_string<Char, std::char_traits<Char>, Allocator>& text) noexcept
    {
      return text;
    }

    template<unqualified_character Char>
    [[nodiscard]]
    constexpr std::basic_string_view<Char> as_string_view(const Char* text)
    {
      return text;
    }

    /** A `std::basic_string_view` or `std::basic_string` of characters, or a pointer to a null-terminated string of
        characters.
     */
    template<class T>
    concept character_string = requires(const T& text) { impl::as_string_view(text); };

    template<character_string Text>
    using character_of_t = decltype(impl::as_string_view(std::declval<const Text&>()))::value_type;

    template<class CharacterConversion>
    struct case_conversion
    {
      template<character Char>
      [[nodiscard]]
      constexpr Char operator()(Char c) const
      {
        return CharacterConversion{}(c);
      }

      template<character_string Text>
      [[nodiscard]]
      constexpr std::basic_string<character_of_t<Text>> operator()(const Text& text) const
      {
        return impl::as_string_view(text)
             | std::views::transform(CharacterConversion{})
             | std::ranges::to<std::basic_string<character_of_t<Text>>>();
      }
    };

    struct to_lowercase_character
    {
      template<character Char>
      [[nodiscard]]
      constexpr Char operator()(Char c) const
      {
        return ascii::is_uppercase(c) ? static_cast<Char>(c - 'A' + 'a') : c;
      }
    };

    struct to_uppercase_character
    {
      template<character Char>
      [[nodiscard]]
      constexpr Char operator()(Char c) const
      {
        return ascii::is_lowercase(c) ? static_cast<Char>(c - 'a' + 'A') : c;
      }
    };

    struct same_ignoring_case_fn
    {
      template<character_string Lhs, character_string Rhs>
        requires std::same_as<character_of_t<Lhs>, character_of_t<Rhs>>
      [[nodiscard]]
      constexpr bool operator()(const Lhs& lhs, const Rhs& rhs) const
      {
        auto sameIgnoringCase{
          [](auto l, auto r){ return to_lowercase_character{}(l) == to_lowercase_character{}(r); }
        };
        return std::ranges::equal(impl::as_string_view(lhs), impl::as_string_view(rhs), sameIgnoringCase);
      }
    };
  }

  namespace ascii
  {
    /** \brief The lowercase form of a character, of the same type, or of a string, as a `std::basic_string` of its
               character type. A string may be a `std::basic_string_view`, a `std::basic_string`, or a pointer to a
               null-terminated string such as a literal.
     */
    inline constexpr impl::case_conversion<impl::to_lowercase_character> to_lowercase{};

    /** \brief The uppercase form of a character, of the same type, or of a string, as a `std::basic_string` of its
               character type. A string may be a `std::basic_string_view`, a `std::basic_string`, or a pointer to a
               null-terminated string such as a literal.
     */
    inline constexpr impl::case_conversion<impl::to_uppercase_character> to_uppercase{};

    /** \brief Whether two strings of the same character type are equal once each is converted to lowercase. */
    inline constexpr impl::same_ignoring_case_fn same_ignoring_case{};
  }
}
