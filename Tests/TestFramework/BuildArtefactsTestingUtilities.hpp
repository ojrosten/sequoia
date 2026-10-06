////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Writers for the build artefacts which BuildArtefacts.cppm reads, so that a build can be described without
           being performed.
 */

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

import std;
import sequoia.test_framework;

namespace sequoia::testing
{
  /** An object file and the files read to produce it, spelled out: what the tests write, and what they compare a
      reading to
   */
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

  /** `p` in UTF-16, as MSBuild's file tracker spells a file the compiler read or wrote: its ASCII letters are in
      upper case, and every other character is as it is.
   */
  [[nodiscard]]
  std::u16string to_tracker_spelling(const std::filesystem::path& p);

  /** The line of a tracker log naming `file`, ended by CRLF. */
  [[nodiscard]]
  std::u16string tracker_line(const std::filesystem::path& file);

  /** The `^`-led line of a tracker log naming `sources`, which were compiled together, ended by CRLF.

      Each source is spelled by `to_tracker_spelling`, as only the CL task's read log spells it. In every other log
      MSBuild upper-cases a source line in full, which this does not reproduce for a character beyond ASCII.
   */
  [[nodiscard]]
  std::u16string tracker_sources_line(std::initializer_list<std::filesystem::path> sources);

  /** Writes `text` to `log` in the tracker's encoding: UTF-16, little-endian, behind a byte order mark.

      \throws std::runtime_error if `log` cannot be opened, written or closed.
   */
  void write_tlog(const std::filesystem::path& log, std::u16string_view text);

  /// Writes the logs MSBuild's file tracker would.
  void write_tlogs(const std::filesystem::path& tlogDir, std::span<const compilation_record> records);
}
