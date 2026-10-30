////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.parsing:CommandLineMessages;

import std;

export import sequoia.text_processing;

/** \file
    \brief Formatting of error and warning messages.
 */

export namespace sequoia::parsing::commandline
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
