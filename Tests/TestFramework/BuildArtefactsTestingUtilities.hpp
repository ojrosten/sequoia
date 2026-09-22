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

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

import std;
import sequoia.test_framework;

namespace sequoia::testing
{
  /// An object file and the files read to produce it, spelled out: what the tests write, and what they compare a reading to
  struct compilation_record
  {
    std::filesystem::path object{};
    std::vector<std::filesystem::path> inputs{};

    [[nodiscard]]
    friend bool operator==(const compilation_record&, const compilation_record&) noexcept = default;

    friend std::ostream& operator<<(std::ostream& s, const compilation_record& record);
  };

  /// Every record, its files spelled out
  [[nodiscard]]
  std::vector<compilation_record> expand(const compilations& c);

  /// Writes a dependency log which ninja would read.
  void write_ninja_deps(const std::filesystem::path& log, std::span<const compilation_record> records);

  /// Writes the logs MSBuild's file tracker would.
  void write_tlogs(const std::filesystem::path& tlogDir, std::span<const compilation_record> records);
}
