////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief A collection of functions for making substitutions within text.
 */

#include "sequoia/Core/Meta/Concepts.hpp"

#include <string>

namespace sequoia
{
  struct char_to_char
  {
    [[nodiscard]]
    char operator()(char c) const noexcept
    {
      return c;
    }
  };

  template<invocable_r<char, char> OnUpper=char_to_char>
  std::string& camel_to_words(std::string& text, std::string_view separator = " ", OnUpper onUpper = OnUpper{})
  {
    auto i{text.begin()};
    while(i != text.end())
    {
      auto& c{*i};
      if(std::isupper(c))
      {
        c = onUpper(c);
        if((std::ranges::distance(text.begin(), i) > 0))
        {
          i = text.insert(i, separator.begin(), separator.end());
          i += std::ranges::distance(separator);
        }
      }

      ++i;
    }

    return text;
  }

  template<invocable_r<char, char> OnUpper = char_to_char>
  [[nodiscard]]
  std::string camel_to_words(std::string_view text, std::string_view separator = " ", OnUpper onUpper = OnUpper{})
  {
    std::string str{text};
    return camel_to_words(str, separator, onUpper);
  }

  std::string& to_camel_case(std::string& text, std::string_view separator="");

  [[nodiscard]]
  std::string to_camel_case(std::string_view text, std::string_view separator = "");

  std::string& to_snake_case(std::string& text);

  [[nodiscard]]
  std::string to_snake_case(std::string_view text);

  std::string& capitalize(std::string& text);

  [[nodiscard]]
  std::string capitalize(std::string_view text);

  std::string& uncapitalize(std::string& text);

  [[nodiscard]]
  std::string uncapitalize(std::string_view text);

  std::string& replace(std::string& text, std::string_view from, std::string_view to);

  [[nodiscard]]
  std::string replace(std::string_view text, std::string_view from, std::string_view to);

  std::string& replace_all(std::string& text, std::string_view from, std::string_view to);

  [[nodiscard]]
  std::string replace_all(std::string_view text, std::string_view from, std::string_view to);

  std::string& replace_all_recursive(std::string& text, std::string_view from, std::string_view to);

  [[nodiscard]]
  std::string replace_all_recursive(std::string_view text, std::string_view from, std::string_view to);

  struct replacement
  {
    std::string from, to;
  };

  template<class... Replacement>
    requires (std::is_same_v<Replacement, replacement> && ...)
  std::string& replace_all(std::string& text, const Replacement&... rs)
  {
    (replace_all(text, rs.from, rs.to), ...);

    return text;
  }

  template<class... Replacement>
    requires (std::is_same_v<Replacement, replacement> && ...)
  [[nodiscard]]
  std::string replace_all(std::string_view text, const Replacement&... rs)
  {
    std::string str{text};
    return replace_all(str, rs...);
  }

  std::string& replace_all(std::string& text, std::string_view anyOfLeft, std::string_view from, std::string_view anyOfRight, std::string_view to);

  [[nodiscard]]
  std::string replace_all(std::string_view text, std::string_view anyOfLeft, std::string_view from, std::string_view anyOfRight, std::string_view to);

  template<invocable_r<bool, char> LeftPred, invocable_r<bool, char> RightPred>
  std::string& replace_all(std::string& text, LeftPred lPred, std::string_view from, RightPred rPred, std::string_view to)
  {
    constexpr auto npos{std::string::npos};
    std::string::size_type pos{};
    while((pos = text.find(from, pos)) != npos)
    {
      if(    (((pos > 0) && lPred(text[pos - 1])) || ((pos == 0) && lPred('\0')))
          && (   ((pos + from.length() < text.length())  && rPred(text[pos + from.length()]))
              || ((pos + from.length() == text.length()) && rPred('\0')))
        )
      {
        text.replace(pos, from.length(), to);
        pos += (to.length() + 1);
      }
      else
      {
        pos += (from.length() + 1) ;
      }
    }

    return text;
  }

  template<invocable_r<bool, char> LeftPred, invocable_r<bool, char> RightPred>
  [[nodiscard]]
  std::string replace_all(std::string_view text, LeftPred lPred, std::string_view from, RightPred rPred, std::string_view to)
  {
    std::string str{text};
    return replace_all(str, lPred, from, rPred, to);
  }
}
