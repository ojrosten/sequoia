////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FreeTestCore.hpp"

/*! \brief Definitions for a handful of functions which are not templated. */

namespace sequoia::unit_testing
{
  namespace impl
  {
    [[nodiscard]]
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
  }
  
  [[nodiscard]]
  std::string report_line(std::string_view file, const int line, const std::string_view message)
  {
    auto s{std::string{file}.append(", Line ").append(std::to_string(line))};
    if(!message.empty()) (s += ":\n\t") += message;

    return s;
  }
}
