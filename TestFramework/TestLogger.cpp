////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestLogger.hpp"

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
}
