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
  std::string& to_camel_case(std::string& text)
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
      text.erase(pos, 1);
      if((pos < text.length()) && std::isalpha(text[pos]))
      {
        text[pos] = upper(text[pos]);
      }

      pos += 1;
    }

    return text;
  }

  [[nodiscard]]
  std::string to_camel_case(std::string_view text)
  {
    std::string str{text};
    return to_camel_case(str);
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

  std::string& replace_all(std::string& text, std::string_view fromBegin, std::string_view fromEnd, std::string_view to)
  {
    constexpr auto npos{std::string::npos};
    std::string::size_type pos{};
    while((pos = text.find(fromBegin, pos)) != npos)
    {
      const auto pos2{text.find(fromEnd, pos + fromBegin.length())};
      if(pos2 == npos) break;

      text.replace(pos, pos2-pos, to);
      pos += to.length();
    }

    return text;
  }

  [[nodiscard]]
  std::string replace_all(std::string_view text, std::string_view fromBegin, std::string_view fromEnd, std::string_view to)
  {
    std::string str{text};
    return replace_all(str, fromBegin, fromEnd, to);
  }

  std::string& replace_all(std::string& text, std::initializer_list<replacement> data)
  {
    for(const auto& r : data)
      replace_all(text, r.from, r.to);

    return text;
  }

  [[nodiscard]]
  std::string replace_all(std::string_view text, std::initializer_list<replacement> data)
  {
    std::string str{text};
    return replace_all(str, data);
  }

  std::string& replace_all(std::string& text, std::string_view anyOfLeft, std::string_view from, std::string_view anyOfRight, std::string_view to)
  {
    constexpr auto npos{std::string::npos};

    auto left{
      [anyOfLeft](char c) { return !anyOfLeft.size() || (anyOfLeft.find(c) < npos); }
    };

    auto right{
      [anyOfRight](char c) {return !anyOfRight.size() || (anyOfRight.find(c) < npos); }
    };

    return replace_all(text, left, from, right, to);
  }

  [[nodiscard]]
  std::string replace_all(std::string_view text, std::string_view anyOfLeft, std::string_view from, std::string_view anyOfRight, std::string_view to)
  {
    std::string str{text};
    return replace_all(str, anyOfLeft, from, anyOfRight, to);
  }
}
