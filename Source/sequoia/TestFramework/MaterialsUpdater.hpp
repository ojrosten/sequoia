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

      -# Each path deleted from `to` is appended to `deleted` before its deletion is attempted. The
         order is that of a depth-first walk over each directory's sorted entries, and so is sorted.
         A deleted directory is one entry.
      -# Each `.seqpat` in `to` whose target `from` holds, and each `.keep` in `to`, is first copied
         into the corresponding directory of `from`, so that `to` keeps it.

      \throws std::runtime_error if `from` or `to` does not exist
      \throws std::logic_error if an entry in both is neither a regular file nor a directory
      \throws std::filesystem::filesystem_error if a filesystem operation fails

      After a throw, `to` may be partly updated, and `deleted` holds every deletion attempted before it.
   */
  void soft_update(const std::filesystem::path& from,
                   const std::filesystem::path& to,
                   std::vector<std::filesystem::path>& deleted);
}
