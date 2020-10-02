////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
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
  std::string footer()
  {
    return "=======================================\n\n";
  }
  
  std::string indent(std::string_view s, indentation ind)
  {
    if(s.empty()) return {};
    if(ind == no_indent) return std::string{s};

    std::string str{};
    str.reserve(s.size());
    
    std::string::size_type current{};
    
    while(current < s.size())
    {      
      constexpr auto npos{std::string::npos};
      const auto dist{s.substr(current).find('\n')};

      const auto count{dist == npos ? npos : dist + 1};
      auto line{s.substr(current, count == npos ? npos : count)};
      if(line.find_first_not_of('\n') != npos)
        str.append(ind);
      
      str.append(line);

      current = (count == npos) ? npos : current + count;
    }    
    
    return str;
  }

  std::string& append_indented(std::string& s1, std::string_view s2, indentation ind)
  {
    if(!s2.empty())
    {
      if(s1.empty())
      {
        s1 = s2;
      }
      else
      {
        s1.append("\n").append(indent(s2, ind));
      }
    }

    return s1;
  }

  void end_block(std::string& s, const std::size_t newlines, std::string_view footer)
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

      s.append(footer);
    }
  }

  [[nodiscard]]
  std::string end_block(std::string_view s, const std::size_t newlines, std::string_view footer)
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
    auto mess{append_lines(std::string{"Error -- "}.append(tag).append(" Exception:"), exceptionMessage).append("\n")};
        
    if(exceptionsDetected)
    {
      append_lines(mess, "Exception thrown during last check");
    }
    else
    {
      append_lines(mess, "Exception thrown after check completed");
    }

    append_lines(mess, "Last Recorded Message:\n", currentMessage);
 
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
    return append_lines(std::string{"Obtained : "}.append(obtained), std::string{"Predicted: "}.append(predicted));
  }

  [[nodiscard]]
  std::string prediction_message(char obtained, char predicted)
  {
    auto transformer{
      [](char c) -> std::string {
        if(c == '\a') return "'\\a'";
        if(c == '\b') return "'\\b'";
        if(c == '\f') return "'\\f'";
        if(c == '\n') return "'\\n'";
        if(c == '\r') return "'\\r'";
        if(c == '\t') return "'\\t'";
        if(c == '\v') return "'\\v'";
        if(c == ' ')  return "' '";

        return std::string(1, c);
      }
    };

    return prediction_message(transformer(obtained), transformer(predicted));
  }

  [[nodiscard]]
  std::string failure_message(bool, bool)
  {
    return "check failed";
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

  [[nodiscard]]
  std::string report_line(std::string_view file, const int line, const std::string_view message)
  {
    return append_lines(std::string{file}.append(", Line ").append(std::to_string(line)), message).append("\n");
  }

  std::string& tidy_name(std::string& name)
  {
    auto pos{name.find("::__")};
    while(pos != std::string::npos)
    {
      const auto pos2{name.find("::", pos+4)};
      if(pos2 == std::string::npos) break;

      name.erase(pos, pos2 - pos);
      pos = name.find("::__", pos);
    }

    pos = name.find(">>");
    while(pos != std::string::npos)
    {
      name.insert(++pos, " ");
      pos = name.find(">>", pos + 1);
    }

    return name;
  }
}
