////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.test_framework:MaterialsUpdater;

import std;

/** \file
    \brief Contains utilities for updating test materials.
 */

export namespace sequoia::testing
{
  void soft_update(const std::filesystem::path& from, const std::filesystem::path& to);
}
