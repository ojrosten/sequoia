////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief The separator which ends each top-level check in a dump.
 */

#include <string_view>

namespace sequoia::testing
{
  /** \brief The separator which ends each top-level check in a dump. */
  inline constexpr std::string_view check_separator{"\n=======================================\n\n"};
}
