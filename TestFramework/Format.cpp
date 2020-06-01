////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for various functions used to format test output.
 */

#include "Format.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string merge(std::string_view s1, std::string_view s2, std::string_view sep)
  {
    std::string mess{};
    if(s1.empty())
    {
      if(!s2.empty()) mess.append(s2);
    }
    else
    {
      mess.append(s1);
      if(!s2.empty())
      {
        if((mess.back() == '\n') && (sep.empty() || sep == " "))
        {
          mess.append("\t");
        }
        else
        {
          mess.append(sep);
          if(sep.back() == '\n') mess.append("\t");
        }
          
        mess.append(s2);
      }
    }
        
    return mess;
  }

  std::string indent(std::string_view s, std::string_view space)
  {
    return s.empty() ? std::string{s} : std::string{space}.append(s);
  }

  /*[[nodiscard]]
  std::string indent(std::string_view s1, std::string_view s2, std::string_view space)
  {
    if(s1.empty() && s2.empty()) return "";

    if(s1.empty()) return std::string{space}.append(s2);

    if(s2.empty()) return std::string{space}.append(s1);

    auto mess{std::string{space}.append(s1)};
    if(mess.back() != '\n') mess.append("\n");

    return mess.append(space).append(s2);
    }*/

  void indent_after(std::string& s1, std::string_view s2, std::string_view space)
  {
    if(s2.empty()) return;

    if(s1.empty())
    {
      s1 = std::string{space}.append(s2);
    }
    else
    {
      if(s1.back() != '\n') s1.append("\n");
      
      s1.append(space).append(s2);
    }
  }

  
  [[nodiscard]]
  std::string make_message(std::string_view tag, std::string_view currentMessage, std::string_view exceptionMessage, const bool exceptionsDetected)  
  {
    auto mess{indent("Error -- ").append(tag).append(" Exception:")};
    indent_after(mess, exceptionMessage);
    mess.append("\n");
        
    if(exceptionsDetected)
    {
      indent_after(mess, "Exception thrown during last check");
    }
    else
    {
      indent_after(mess, "\tException thrown after check completed");
    }

    indent_after(mess, "Last Recorded Message:");
    indent_after(mess, currentMessage);
 
    return mess;
  }

  [[nodiscard]]
  std::string operator_message(std::string_view description, std::string_view op, std::string_view opRetVal)
  {
    std::string info{indent(description)};
    indent_after(info, "operator");
    info.append(op).append(" returned ").append(opRetVal).append("\n");

    return info;
  }

  [[nodiscard]]
  std::string prediction_message(std::string_view obtained, std::string_view predicted, std::string_view advice)
  {
    std::string mess{};

    mess.append("\tObtained : ").append(obtained).append("\n");
    mess.append("\tPredicted: ").append(predicted).append("\n\n");

    if(!advice.empty())
    {
      mess.append("\tAdvice: ").append(advice).append("\n\n");
    }

    return mess;
  }
  
  [[nodiscard]]
  std::string report_line(std::string_view file, const int line, const std::string_view message)
  {
    auto s{std::string{file}.append(", Line ").append(std::to_string(line))};
    if(!message.empty()) s.append(":\n\t").append(message);

    s.append("\n");

    return s;
  }
}
