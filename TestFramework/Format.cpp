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

    append_indented(mess, "Last Recorded Message:");
    append_indented(mess, currentMessage);
 
    return mess;
  }

  [[nodiscard]]
  std::string operator_message(std::string_view description, std::string_view op, std::string_view opRetVal)
  {
    return append_indented(description, "operator").append(op).append(" returned ").append(opRetVal);
  }

  [[nodiscard]]
  std::string prediction_message(std::string_view obtained, std::string_view predicted)
  {
    auto info{std::string{"Obtained : "}.append(obtained)};
    append_indented(info, "Predicted: ").append(predicted);

    return info;
  }

  void advice_data::append_and_tidy(std::string& message) const
  {
    if(!m_Advice.empty())
    {
      append_indented(message, "Advice: ").append(m_Advice);
    }

    tidy(message);
  }

  void advice_data::tidy(std::string& message) const
  {
    if(!m_AdviceTypeName.empty())
    {
      auto pos{message.find(m_AdviceTypeName)};
      while(pos != std::string::npos)
      {
        const auto posBack{message.rfind(',', pos)};
        const auto posFwd{message.find('\n', pos)};

        if((posBack != std::string::npos) && (posFwd != std::string::npos))
        {        
          const auto count{posFwd - posBack - 1};
          message.erase(posBack, count); 
        }

        pos = message.find(m_AdviceTypeName);
      }
    }
  }

  void append_advice(std::string& message, const advice_data& adviceData)
  {
    adviceData.append_and_tidy(message);
  }
  
  [[nodiscard]]
  std::string report_line(std::string_view file, const int line, const std::string_view message)
  {
    auto info{std::string{file}.append(", Line ").append(std::to_string(line))};
    append_indented(info, message).append("\n");

    return info;
  }

  void tidy_name(std::string& name)
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
  }
}
