////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Classifications and conversions of a `char`.

    `is_ascii` says whether a `char` is ASCII. Each of the others makes the classification or conversion that the
    corresponding `<cctype>` function makes in the current locale. Each is defined for every value of `char`,
    including the negative values that a `<cctype>` function does not accept.
 */

#include <cctype>
#include <concepts>

namespace sequoia
{
  inline constexpr auto is_ascii{
    [](std::same_as<char> auto c){ return static_cast<unsigned char>(c) < 0x80; }
  };

  inline constexpr auto is_alphabetic{
    [](std::same_as<char> auto c){ return std::isalpha( static_cast<unsigned char>(c)) != 0; }
  };

  inline constexpr auto is_alphanumeric{
    [](std::same_as<char> auto c){ return std::isalnum( static_cast<unsigned char>(c)) != 0; }
  };

  inline constexpr auto is_digit{
    [](std::same_as<char> auto c){ return std::isdigit( static_cast<unsigned char>(c)) != 0; }
  };

  inline constexpr auto is_hex_digit{
    [](std::same_as<char> auto c){ return std::isxdigit(static_cast<unsigned char>(c)) != 0; }
  };

  inline constexpr auto is_uppercase{
    [](std::same_as<char> auto c){ return std::isupper( static_cast<unsigned char>(c)) != 0; }
  };

  inline constexpr auto to_lowercase{
    [](std::same_as<char> auto c){ return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
  };

  inline constexpr auto to_uppercase{
    [](std::same_as<char> auto c){ return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
  };
}
