////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/VersionedOutput.hpp"

#include "sequoia/Streaming/Streaming.hpp"

#include <algorithm>
#include <format>
#include <optional>
#include <ranges>

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
        auto text{read_to_string(entry.path(), std::ios_base::in | std::ios_base::binary)};
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
    std::vector<fs::path> keys_only_in(const versioned_output_snapshot& snapshot, const versioned_output_snapshot& other)
    {
      std::vector<fs::path> keys{};
      std::ranges::set_difference(snapshot | std::views::keys, other | std::views::keys, std::back_inserter(keys));

      return keys;
    }

    void append_group(std::string& text, std::string_view heading, const std::vector<fs::path>& paths)
    {
      if(paths.empty()) return;

      text.append(heading).append(":\n");
      for(const auto& p : paths) text.append(std::format("  {}\n", p.generic_string()));
    }

    /** A final line lacking its newline still counts, as it does for git */
    [[nodiscard]]
    std::size_t line_count(std::string_view text)
    {
      const auto newlines{static_cast<std::size_t>(std::ranges::count(text, '\n'))};
      return newlines + ((!text.empty() && !text.ends_with('\n')) ? 1 : 0);
    }

    void append_hunk_lines(std::string& patch, char marker, std::string_view text)
    {
      for(std::size_t pos{}; pos < text.size();)
      {
        const auto end{text.find('\n', pos)};
        patch.push_back(marker);
        patch.append(text.substr(pos, end - pos)).push_back('\n');
        if(end == std::string_view::npos)
        {
          patch.append("\\ No newline at end of file\n");
          break;
        }

        pos = end + 1;
      }
    }

    /** An absent `before` is a file added, an absent `after` one removed. A file created or deleted
        empty is the header alone, which is how git itself writes it.
     */
    void append_file_patch(std::string& patch, const fs::path& path, std::optional<std::string_view> before, std::optional<std::string_view> after)
    {
      const auto name{path.generic_string()};
      patch.append(std::format("diff --git a/{} b/{}\n", name, name));
      if(!before) patch.append("new file mode 100644\n");
      if(!after)  patch.append("deleted file mode 100644\n");

      const auto oldText{before.value_or("")}, newText{after.value_or("")};
      if(oldText.empty() && newText.empty()) return;

      patch.append(before ? std::format("--- a/{}\n", name) : "--- /dev/null\n");
      patch.append(after  ? std::format("+++ b/{}\n", name) : "+++ /dev/null\n");
      patch.append(std::format("@@ -{},{} +{},{} @@\n", oldText.empty() ? 0 : 1, line_count(oldText), newText.empty() ? 0 : 1, line_count(newText)));
      append_hunk_lines(patch, '-', oldText);
      append_hunk_lines(patch, '+', newText);
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

    return {.added    = keys_only_in(after, before),
            .removed  = keys_only_in(before, after),
            .modified = after | std::views::filter(changed) | std::views::keys | std::ranges::to<std::vector>()};
  }

  [[nodiscard]]
  std::string to_string(const versioned_output_differences& differences)
  {
    std::string text{};

    append_group(text, "Added",    differences.added);
    append_group(text, "Removed",  differences.removed);
    append_group(text, "Modified", differences.modified);

    return text;
  }

  [[nodiscard]]
  std::string unified_diff(const versioned_output_snapshot& before, const versioned_output_snapshot& after, const fs::path& snapshotDir)
  {
    const auto differences{compare_versioned_output(before, after)};

    std::string patch{};
    for(const auto& p : differences.added)    append_file_patch(patch, snapshotDir / p, std::nullopt, after.at(p));
    for(const auto& p : differences.removed)  append_file_patch(patch, snapshotDir / p, before.at(p), std::nullopt);
    for(const auto& p : differences.modified) append_file_patch(patch, snapshotDir / p, before.at(p), after.at(p));

    return patch;
  }
}
