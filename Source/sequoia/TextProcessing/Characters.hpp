////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Classifications and conversions of a `char`, and the lowercase conversion of a string.

    Only ASCII letters and digits are recognised as letters and digits. `is_identifier_character` also accepts `_`.
    A conversion changes the case of an ASCII letter and leaves every other `char` as it is. None of these helpers
    depends on the locale.
 */

#include <algorithm>
#include <concepts>
#include <string>
#include <string_view>

namespace sequoia
{
  inline constexpr auto is_ascii{
    [](std::same_as<char> auto c){ return static_cast<unsigned char>(c) < 0x80; }
  };

  inline constexpr auto is_digit{
    [](std::same_as<char> auto c){ return (c >= '0') && (c <= '9'); }
  };

  inline constexpr auto is_uppercase{
    [](std::same_as<char> auto c){ return (c >= 'A') && (c <= 'Z'); }
  };

  inline constexpr auto is_lowercase{
    [](std::same_as<char> auto c){ return (c >= 'a') && (c <= 'z'); }
  };

  inline constexpr auto is_alphabetic{
    [](std::same_as<char> auto c){ return is_uppercase(c) || is_lowercase(c); }
  };

  inline constexpr auto is_alphanumeric{
    [](std::same_as<char> auto c){ return is_alphabetic(c) || is_digit(c); }
  };

  inline constexpr auto is_hex_digit{
    [](std::same_as<char> auto c){ return is_digit(c) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F')); }
  };

  inline constexpr auto is_identifier_character{
    [](std::same_as<char> auto c){ return is_alphanumeric(c) || (c == '_'); }
  };

  namespace impl
  {
    struct to_lowercase_fn
    {
      [[nodiscard]]
      constexpr char operator()(std::same_as<char> auto c) const
      {
        return is_uppercase(c) ? static_cast<char>(c - 'A' + 'a') : c;
      }

      [[nodiscard]]
      constexpr std::string operator()(std::string_view text) const
      {
        std::string str{text};
        std::ranges::transform(str, str.begin(), *this);
        return str;
      }
    };
  }

  /** \brief Converts a `char`, or a copy of a string, to lowercase. */
  inline constexpr impl::to_lowercase_fn to_lowercase{};

  inline constexpr auto to_uppercase{
    [](std::same_as<char> auto c){ return is_lowercase(c) ? static_cast<char>(c - 'a' + 'A') : c; }
  };
}
