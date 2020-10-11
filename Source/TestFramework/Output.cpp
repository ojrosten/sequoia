////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for Output.hpp.
 */

#include "Output.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string footer()
  {
    return "=======================================\n\n";
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
  std::string exception_message(std::string_view tag, const std::filesystem::path& filename, std::string_view currentMessage, std::string_view exceptionMessage, const bool exceptionsDetected)  
  {
    auto mess{append_lines(std::string{"Error -- "}.append(tag).append(" Exception:"), exceptionMessage).append("\n")};

    if(!currentMessage.empty())
    {      
      std::string_view suffix{exceptionsDetected ? "during last check" : "after check completed"};
      append_lines(mess, std::string{"Exception thrown "}.append(suffix), "Last Recorded Message:\n", currentMessage);
    }
    else
    {
      append_lines(mess, "Exception thrown before any checks performed in file", filename.string());
    }

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
  std::string display_character(char c)
  {
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

  [[nodiscard]]
  std::string prediction_message(char obtained, char predicted)
  {
    return prediction_message(display_character(obtained), display_character(predicted));
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
