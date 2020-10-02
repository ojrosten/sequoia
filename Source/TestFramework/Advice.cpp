////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for Advice.hpp
 */

#include "Advice.hpp"

namespace sequoia::testing
{
  std::string& advice_data::append_and_tidy(std::string& message) const
  {
    if(!m_Advice.empty())
    {
      append_lines(message, m_Prefix).append(m_Advice);
    }

    return tidy(message);
  }

  std::string& advice_data::tidy(std::string& message) const
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

    return message;
  }

  std::string& append_advice(std::string& message, const advice_data& adviceData)
  {
    return adviceData.append_and_tidy(message);
  }
}
