////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Writers for the build artefacts which BuildArtefacts.hpp reads, so that a build can be described without being performed.
 */

#include "sequoia/TestFramework/BuildArtefacts.hpp"

#include <span>

namespace sequoia::testing
{
  /// Writes a dependency log which ninja would read.
  void write_ninja_deps(const std::filesystem::path& log, std::span<const compilation_record> records);

  /// Writes the logs MSBuild's file tracker would.
  void write_tlogs(const std::filesystem::path& tlogDir, std::span<const compilation_record> records);
}
