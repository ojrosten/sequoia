////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for Format.hpp.
 */

#include "Format.hpp"

namespace sequoia::testing
{
  std::string footer(std::string_view indentation)
  {
    return indent("=======================================\n\n", indentation);
  }
  
  std::string indent(std::string_view s, std::string_view indentation)
  {
    return s.empty() ? std::string{} : std::string{indentation}.append(s);
  }

  std::string& append_indented(std::string& s1, std::string_view s2, std::string_view indentation)
  {
    if(!s2.empty())
    {
      if(s1.empty())
      {
        s1 = s2;
      }
      else
      {
        s1.append("\n").append(indentation).append(s2);
      }
    }

    return s1;
  }

  [[nodiscard]]
  std::string append_indented(std::string_view s1, std::string_view s2, std::string_view indentation)
  {
    std::string s{s1};
    append_indented(s, s2, indentation);

    return s;
  }

  void end_block(std::string& s, const std::size_t newlines, std::string footer)
  {    
    if(!s.empty())
    {
      std::size_t n{};
      for(; n < std::min(s.size(), newlines); ++n)
      {
        if(s[s.size() - 1 - n] != '\n') break;
      }

      for(; n<newlines; ++n)
      {
        s.append("\n");
      }

      s.append(std::move(footer));
    }
  }

  [[nodiscard]]
  std::string end_block(std::string_view s, const std::size_t newlines, std::string footer)
  {
    std::string str{s};
    end_block(str, newlines, footer);

    return str;
  }

  [[nodiscard]]
  std::string emphasise(std::string_view s)
  {
    std::string_view emph{"--"};

    return std::string{emph}.append(s).append(emph);
  }
  
  [[nodiscard]]
  std::string exception_message(std::string_view tag, std::string_view currentMessage, std::string_view exceptionMessage, const bool exceptionsDetected)  
  {
    auto mess{indent("Error -- ").append(tag).append(" Exception:")};
    append_indented(mess, exceptionMessage).append("\n");
        
    if(exceptionsDetected)
    {
      append_indented(mess, "Exception thrown during last check");
    }
    else
    {
      append_indented(mess, "Exception thrown after check completed");
    }

    append_indented(mess, "Last Recorded Message:\n");
    append_indented(mess, currentMessage);
 
    return mess;
  }

  [[nodiscard]]
  std::string operator_message(std::string_view op, std::string_view opRetVal)
  {
    return std::string{"operator"}.append(op).append(" returned ").append(opRetVal);
  }

  [[nodiscard]]
  std::string prediction_message(std::string_view obtained, std::string_view predicted)
  {
    auto info{std::string{"Obtained : "}.append(obtained)};
    append_indented(info, "Predicted: ").append(predicted);

    return info;
  }

  [[nodiscard]]
  std::string to_camel_case(std::string text)
  {
    if(!text.empty())
    {
      if(std::isalpha(text.front()))
      {
        text.front() = std::toupper(text.front());
      }

      using size_t = std::string::size_type;

      size_t pos{};
      while((pos = text.find("_", pos)) != std::string::npos)
      {
        text.erase(pos, 1);
        if((pos < text.length()) && std::isalpha(text[pos]))
        {
          text[pos] = std::toupper(text[pos]);
        }

        pos += 1;
      }
    }

    return text;
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

}
