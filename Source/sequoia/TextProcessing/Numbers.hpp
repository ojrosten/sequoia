////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Extraction of the numbers in text.
 */

#include "sequoia/Core/Meta/Concepts.hpp"

#include <array>
#include <charconv>
#include <optional>
#include <string_view>

namespace sequoia
{
  /** \brief Extracts the numbers in `text`, if there are exactly `N`.

      A number is anything `std::from_chars` reads as a `T`. Reading tries
      each character from left to right, and resumes after each number it
      reads. So for a floating-point `T`, integers count, and so do `inf` and
      `nan`, even inside a word. A `-` directly before a number is its sign,
      if `T` has one.

      Text whose value `T` cannot represent is not a number. Reading resumes
      at its second character, so the rest of that text may be read as one.

      \returns
      -# The numbers, in the order of `text`, if there are exactly `N`;
      -# `std::nullopt`, otherwise.
   */
  template<class T, std::size_t N>
    requires (integer<T> || std::floating_point<T>) && std::same_as<T, std::remove_cv_t<T>>
  [[nodiscard]]
  std::optional<std::array<T, N>> extract_numbers_from(std::string_view text)
  {
    std::array<T, N> numbers{};
    std::size_t count{};
    const auto last{text.data() + text.size()};
    auto first{text.data()};
    while(first != last)
    {
      T value{};
      if(const auto [next, error]{std::from_chars(first, last, value)}; error == std::errc{})
      {
        if(count == N)
          return std::nullopt;

        numbers[count++] = value;
        first = next;
      }
      else
      {
        ++first;
      }
    }

    return count == N ? std::optional{numbers} : std::nullopt;
  }
}
