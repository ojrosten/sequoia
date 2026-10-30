////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Formatting of error and warning messages.
 */

#include "sequoia/TextProcessing/Indent.hpp"

#include <initializer_list>
#include <string>
#include <string_view>

namespace sequoia::parsing::commandline
{
  [[nodiscard]]
  std::string error(std::string_view message, indentation indent=indentation{"  "});

  [[nodiscard]]
  std::string error(std::initializer_list<std::string_view> messages, indentation indent=indentation{"  "});

  [[nodiscard]]
  std::string warning(std::string_view message, indentation indent=indentation{"  "});

  [[nodiscard]]
  std::string warning(std::initializer_list<std::string_view> messages, indentation indent=indentation{"  "});
}
