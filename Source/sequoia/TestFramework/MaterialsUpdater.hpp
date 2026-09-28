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
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace sequoia::testing
{
  /** \brief Thrown by `soft_update` when the update fails after it has begun.

      `what()` is the message of the exception that stopped the update.
   */
  class incomplete_update : public std::runtime_error
  {
  public:
    incomplete_update(const std::string& message, std::vector<std::filesystem::path> deleted);

    incomplete_update(const incomplete_update&) noexcept = default;

    incomplete_update& operator=(const incomplete_update&) noexcept = default;

    /** \brief The paths `soft_update` deleted before the update stopped, in the order `soft_update` returns them.

        If a deletion itself threw, the path being deleted is the last, and may be partly deleted.
     */
    [[nodiscard]]
    std::span<const std::filesystem::path> deleted() const noexcept
    {
      return *m_Deleted;
    }
  private:
    std::shared_ptr<const std::vector<std::filesystem::path>> m_Deleted{};
  };

  /** \brief Makes `to` mirror `from`, rewriting only the files whose contents differ once reduced
             by any `.seqpat` beside them in `to`.

      -# Each `.seqpat` in `to` whose target `from` holds, and each `.keep` in `to`, is first copied
         into the corresponding directory of `from`, so that `to` keeps it.

      \returns Each path deleted from `to`. The order is that of a depth-first walk over each
               directory's sorted entries, and so is sorted. A deleted directory is one entry.

      \throws std::runtime_error if `from` or `to` is not a directory; no update is performed
      \throws incomplete_update if the update fails after it has begun; `to` may be partly updated
   */
  [[nodiscard]]
  std::vector<std::filesystem::path> soft_update(const std::filesystem::path& from, const std::filesystem::path& to);
}
