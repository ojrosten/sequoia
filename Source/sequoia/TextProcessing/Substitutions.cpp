////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for Substitutions.hpp
 */

#include "sequoia/TextProcessing/Substitutions.hpp"

namespace sequoia
{
  std::string& to_camel_case(std::string& text, std::string_view separator)
  {
    if(text.empty()) return text;

    auto upper{
      [](char c) {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
      }
    };

    if(std::isalpha(text.front()))
    {
      text.front() = upper(text.front());
    }

    using size_t = std::string::size_type;

    size_t pos{};
    while((pos = text.find("_", pos)) != std::string::npos)
    {
      text.replace(pos, 1, separator);
      const auto next{pos + separator.size()};
      if((next < text.length()) && std::isalpha(text[next]))
      {
        text[next] = upper(text[next]);
      }

      pos += (separator.size() + 1);
    }

    return text;
  }

  [[nodiscard]]
  std::string to_camel_case(std::string_view text, std::string_view separator)
  {
    std::string str{text};
    return to_camel_case(str, separator);
  }

  std::string& to_snake_case(std::string& text)
  {
    return camel_to_words(text, "_", [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
  }

  [[nodiscard]]
  std::string to_snake_case(std::string_view text)
  {
    std::string str{text};
    return to_snake_case(str);
  }

  std::string& capitalize(std::string& text)
  {
    if(!text.empty())
    {
      auto& c{text.front()};
      c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    return text;
  }

  [[nodiscard]]
  std::string capitalize(std::string_view text)
  {
    std::string str{text};
    return capitalize(str);
  }

  std::string& uncapitalize(std::string& text)
  {
    if(!text.empty())
    {
      auto& c{text.front()};
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return text;
  }

  [[nodiscard]]
  std::string uncapitalize(std::string_view text)
  {
    std::string str{text};
    return uncapitalize(str);
  }

  std::string& replace(std::string& text, std::string_view from, std::string_view to)
  {
    if(auto pos{text.find(from)}; pos != std::string::npos)
    {
      text.replace(pos, from.size(), to);
    }
    return text;
  }

  [[nodiscard]]
  std::string replace(std::string_view text, std::string_view from, std::string_view to)
  {
    std::string str{text};
    return replace(str, from, to);
  }

  std::string& replace_all(std::string& text, std::string_view from, std::string_view to)
  {
    std::string::size_type pos{};
    while((pos = text.find(from, pos)) != std::string::npos)
    {
      text.replace(pos, from.length(), to);
      pos += to.length();
    }

    return text;
  }

  [[nodiscard]]
  std::string replace_all(std::string_view text, std::string_view from, std::string_view to)
  {
    std::string str{text};
    return replace_all(str, from, to);
  }

  std::string& replace_all_recursive(std::string& text, std::string_view from, std::string_view to)
  {
    std::string::size_type pos{};
    while((pos = text.find(from, pos)) != std::string::npos)
    {
      text.replace(pos, from.length(), to);
      ++pos;
    }

    return text;
  }

  [[nodiscard]]
  std::string replace_all_recursive(std::string_view text, std::string_view from, std::string_view to)
  {
    std::string str{text};
    return replace_all_recursive(str, from, to);
  }

  std::string& replace_all(std::string& text, std::string_view anyOfLeft, std::string_view from, std::string_view anyOfRight, std::string_view to)
  {
    constexpr auto npos{std::string::npos};

    return replace_all(text,
		       [anyOfLeft] (char c) { return !anyOfLeft.size() || (anyOfLeft.find(c)  < npos); },
		       from,
		       [anyOfRight](char c) {return !anyOfRight.size() || (anyOfRight.find(c) < npos); },
		       to);
  }

  [[nodiscard]]
  std::string replace_all(std::string_view text, std::string_view anyOfLeft, std::string_view from, std::string_view anyOfRight, std::string_view to)
  {
    std::string str{text};
    return replace_all(str, anyOfLeft, from, anyOfRight, to);
  }
}
