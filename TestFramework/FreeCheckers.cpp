////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for certain functions defined in FreeCheckers.hpp

 */

#include "FreeCheckers.hpp"

namespace sequoia::unit_testing
{
  [[nodiscard]]
  std::string operator_message(std::string_view description, std::string_view typeInfo, std::string_view op, std::string_view retVal)
  {
    std::string info{typeInfo};

    info.append("\toperator").append(op)
        .append(" returned ").append(retVal).append("\n");

    return description.empty()
      ? std::string{"\t"}.append(std::move(info))
      : std::string{"\t"}.append(description).append("\n\t" + std::move(info));
  }

  [[nodiscard]]
  std::string prediction_message(std::string_view obtained, std::string_view predicted)
  {
    std::string mess{};

    mess.append("\tObtained : ").append(obtained).append("\n");
    mess.append("\tPredicted: ").append(predicted).append("\n\n");

    return mess;
  }
}
