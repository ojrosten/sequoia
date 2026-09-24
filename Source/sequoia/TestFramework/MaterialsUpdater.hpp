////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Contains utilities for updating test materials.
 */

#include <filesystem>
#include <vector>

namespace sequoia::testing
{
  /** \brief Makes `to` mirror `from`, rewriting only the files whose contents differ once reduced
             by any `.seqpat` beside them in `to`.

      Returns the paths removed from `to` because `from` has no counterpart, relative to `to` and
      sorted. A removed directory is one entry.
   */
  [[nodiscard]]
  std::vector<std::filesystem::path> soft_update(const std::filesystem::path& from, const std::filesystem::path& to);
}