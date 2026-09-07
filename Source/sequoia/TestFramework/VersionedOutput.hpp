////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Detection of changes to a project's versioned output.
 */

#include "sequoia/TestFramework/ProjectPaths.hpp"

#include <map>
#include <string>
#include <vector>

namespace sequoia::testing
{
  /** The content of the versioned output files, keyed by path relative to the output directory. */
  using versioned_output_snapshot = std::map<std::filesystem::path, std::string>;

  [[nodiscard]]
  versioned_output_snapshot take_versioned_output_snapshot(const output_paths& output);

  /** Paths, relative to the output directory, grouped by how they changed between two snapshots. */
  struct versioned_output_differences
  {
    std::vector<std::filesystem::path> added{}, removed{}, modified{};

    [[nodiscard]]
    bool empty() const noexcept { return added.empty() && removed.empty() && modified.empty(); }

    [[nodiscard]]
    friend bool operator==(const versioned_output_differences&, const versioned_output_differences&) noexcept = default;
  };

  [[nodiscard]]
  versioned_output_differences compare_versioned_output(const versioned_output_snapshot& before, const versioned_output_snapshot& after);

  /** The differences, listed under a heading for each non-empty group; empty if there are none.
      Paths are rendered generically, since this is written both to the console and to files which
      are compared across platforms.
   */
  [[nodiscard]]
  std::string to_string(const versioned_output_differences& differences);
}
