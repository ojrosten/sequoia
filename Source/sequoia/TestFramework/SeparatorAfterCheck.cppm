////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.test_framework:SeparatorAfterCheck;

import std;

/** \file
    \brief The separator which ends each top-level check in a dump.
 */

export namespace sequoia::testing
{
  /** \brief The separator which ends each top-level check in a dump. */
  inline constexpr std::string_view separator_after_check{"\n=======================================\n\n"};
}
