////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
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
