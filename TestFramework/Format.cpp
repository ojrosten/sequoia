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

  std::string indent(std::string_view s, std::string_view gap)
  {
    return s.empty() ? std::string{s} : std::string{gap}.append(s);
  }

  std::string& append_indented(std::string& s1, std::string_view s2, std::string_view gap)
  {
    if(!s2.empty())
    {
      if(s1.empty())
      {
        s1 = s2;
      }
      else
      {
        s1.append("\n").append(gap).append(s2);
      }
    }

    return s1;
  }

  [[nodiscard]]
  std::string append_indented(std::string_view s1, std::string_view s2, std::string_view gap)
  {
    std::string s{s1};
    append_indented(s, s2, gap);

    return s;
  }

  void end_block(std::string& s, const std::size_t gap)
  {    
    if(!s.empty())
    {
      std::size_t n{};
      for(; n < std::min(s.size(), gap); ++n)
      {
        if(s[s.size() - 1 - n] != '\n') break;
      }

      for(; n<gap; ++n)
      {
        s.append("\n");
      }
    }
  }
  
  [[nodiscard]]
  std::string make_message(std::string_view tag, std::string_view currentMessage, std::string_view exceptionMessage, const bool exceptionsDetected)  
  {
    auto mess{indent("Error -- ").append(tag).append(" Exception:")};
    append_indented(mess, exceptionMessage);
    mess.append("\n");
        
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
    std::string info{indent(description)};
    append_indented(info, "operator");
    info.append(op).append(" returned ").append(opRetVal);

    return info;
  }

  [[nodiscard]]
  std::string prediction_message(std::string_view obtained, std::string_view predicted, std::string_view advice)
  {
    auto info{std::string{"Obtained : "}.append(obtained)};
    append_indented(info, "Predicted: ").append(predicted);
    
    if(!advice.empty())
    {
      append_indented(info, "Advice: ").append(advice);
    }

    return info;
  }
  
  [[nodiscard]]
  std::string report_line(std::string_view file, const int line, const std::string_view message)
  {
    auto info{std::string{file}.append(", Line ").append(std::to_string(line))};
    append_indented(info, message).append("\n");

    return info;
  }
}
