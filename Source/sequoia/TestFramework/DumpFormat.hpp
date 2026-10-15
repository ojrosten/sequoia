////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief The form of a dump of a test run.
 */

#include <string_view>

namespace sequoia::testing
{
  /** \brief The form of a dump, shared by the logger, which writes dumps,
             and by `compare_dumps`, which reads them.

      A top-level check is its message, then the message of every check
      nested in it, one per line, then the separator. An empty message
      writes nothing.
   */
  namespace dump_format
  {
    constexpr std::string_view check_separator{"\n=======================================\n\n"};
  }
}
