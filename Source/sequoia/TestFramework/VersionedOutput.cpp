////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file
    \brief Definitions for VersionedOutput.hpp
 */

#include "sequoia/TestFramework/VersionedOutput.hpp"

#include "sequoia/Streaming/Streaming.hpp"

#include <algorithm>
#include <ranges>
#include <stdexcept>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    /** Dot-prefixed names belong to other tools and change on their schedule rather than sequoia's:
        `.DS_Store` lives in `output/TestSummaries` and moves whenever a file browser looks at the
        directory. Every component below `dir` is checked, so that a hidden directory takes its
        contents with it; only components below, since the tree itself may sit beneath one.
     */
    [[nodiscard]]
    auto is_versioned_file(const fs::path& dir)
    {
      return [&dir](const fs::directory_entry& entry) {
        auto hidden{[](const fs::path& component) { return component.string().starts_with('.'); }};

        return std::ranges::none_of(fs::relative(entry.path(), dir), hidden) && entry.is_regular_file();
      };
    }

    [[nodiscard]]
    auto to_snapshot_entry(const fs::path& root)
    {
      return [&root](const fs::directory_entry& entry) {
        auto text{read_to_string(entry.path())};
        if(!text) throw std::runtime_error{report_failed_read(entry.path())};

        return versioned_output_snapshot::value_type{fs::relative(entry.path(), root), std::move(*text)};
      };
    }

    [[nodiscard]]
    versioned_output_snapshot gather(const fs::path& dir, const fs::path& root)
    {
      if(!fs::exists(dir)) return {};

      return fs::recursive_directory_iterator{dir}
           | std::views::filter(is_versioned_file(dir))
           | std::views::transform(to_snapshot_entry(root))
           | std::ranges::to<versioned_output_snapshot>();
    }

    [[nodiscard]]
    auto absent_from(const versioned_output_snapshot& snapshot)
    {
      return [&snapshot](const fs::path& path) { return !snapshot.contains(path); };
    }
  }

  [[nodiscard]]
  versioned_output_snapshot take_versioned_output_snapshot(const output_paths& output)
  {
    auto snapshot{gather(output.diagnostics(), output.dir())};
    snapshot.merge(gather(output.test_summaries(), output.dir()));

    return snapshot;
  }

  [[nodiscard]]
  versioned_output_differences compare_versioned_output(const versioned_output_snapshot& before, const versioned_output_snapshot& after)
  {
    auto changed{
      [&before](const versioned_output_snapshot::value_type& entry) {
        const auto found{before.find(entry.first)};
        return (found != before.end()) && (found->second != entry.second);
      }
    };

    return {.added    = after  | std::views::keys | std::views::filter(absent_from(before)) | std::ranges::to<std::vector>(),
            .removed  = before | std::views::keys | std::views::filter(absent_from(after))  | std::ranges::to<std::vector>(),
            .modified = after  | std::views::filter(changed) | std::views::keys | std::ranges::to<std::vector>()};
  }
}
