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

  std::string format(std::string_view s)
  {
    return s.empty() ? std::string{s} : std::string{'\t'}.append(s);
  }

  [[nodiscard]]
  std::string make_message(std::string_view tag, std::string_view currentMessage, std::string_view exceptionMessage, const bool exceptionsDetected)
  {
    auto mess{(std::string{"\tError -- "}.append(tag) += " Exception:") += format(exceptionMessage) += '\n'};
    if(exceptionsDetected)
    {
      mess += "\tException thrown during last check\n";
    }
    else
    {
      mess += "\tException thrown after check completed\n";
    }

    mess += ("\tLast Recorded Message:\n" + format(currentMessage));

    return mess;
  }

  [[nodiscard]]
  std::string operator_message(std::string_view description, std::string_view typeInfo, std::string_view op, std::string_view opRetVal)
  {
    std::string info{typeInfo};

    info.append("\toperator").append(op)
        .append(" returned ").append(opRetVal).append("\n");

    return description.empty()
      ? std::string{"\t"}.append(std::move(info))
      : std::string{"\t"}.append(description).append("\n\t" + std::move(info));
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
