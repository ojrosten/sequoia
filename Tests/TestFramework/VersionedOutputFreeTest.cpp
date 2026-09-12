////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "VersionedOutputFreeTest.hpp"

#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TestFramework/VersionedOutput.hpp"

#include <ranges>
#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path versioned_output_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void versioned_output_free_test::run_tests()
  {
    test_comparisons();
    test_reporting();
    test_snapshot();
  }

  void versioned_output_free_test::test_comparisons()
  {
    namespace fs = std::filesystem;

    auto check_differences{
      [this](std::string_view description,
             const versioned_output_differences& differences,
             const std::vector<fs::path>& added,
             const std::vector<fs::path>& removed,
             const std::vector<fs::path>& modified)
      {
        check(equality, append_lines(description, "added"),    differences.added,    added);
        check(equality, append_lines(description, "removed"),  differences.removed,  removed);
        check(equality, append_lines(description, "modified"), differences.modified, modified);
        check(append_lines(description, "empty"), differences.empty() == (added.empty() && removed.empty() && modified.empty()));
      }
    };

    const versioned_output_snapshot none{}, one{{"a.txt", "alpha"}}, two{{"a.txt", "alpha"}, {"b.txt", "beta"}};

    check_differences("Two empty snapshots",     compare_versioned_output(none, none), {}, {}, {});
    check_differences("A snapshot against self", compare_versioned_output(two,  two),  {}, {}, {});
    check_differences("A file appears",          compare_versioned_output(one,  two),  {"b.txt"}, {}, {});
    check_differences("A file disappears",       compare_versioned_output(two,  one),  {}, {"b.txt"}, {});

    check_differences("A file changes",
                      compare_versioned_output(one, versioned_output_snapshot{{"a.txt", "not alpha"}}),
                      {}, {}, {"a.txt"});

    check_differences("One of each, reported separately",
                      compare_versioned_output(two, versioned_output_snapshot{{"a.txt", "not alpha"}, {"c.txt", "gamma"}}),
                      {"c.txt"}, {"b.txt"}, {"a.txt"});
  }

  void versioned_output_free_test::test_reporting()
  {
    namespace fs = std::filesystem;

    check(equality, "Nothing to report", to_string(versioned_output_differences{}), std::string{});

    // `fs::path{"Sub"} / "b.txt"` carries the platform's separator, which is what `fs::relative`
    // yields when a snapshot is taken; the report must nevertheless read the same everywhere.
    check(equality,
          "Only the groups with something in them are listed",
          to_string({.modified{"a.txt", fs::path{"Sub"} / "b.txt"}}),
          std::string{"Modified:\n  a.txt\n  Sub/b.txt\n"});

    check(equality,
          "Each group under its own heading, in a fixed order",
          to_string({.added{"c.txt"}, .removed{"d.txt"}, .modified{"a.txt"}}),
          std::string{"Added:\n  c.txt\nRemoved:\n  d.txt\nModified:\n  a.txt\n"});
  }

  void versioned_output_free_test::test_snapshot()
  {
    namespace fs = std::filesystem;

    const auto root{working_materials()};

    // Written here rather than committed as materials, since dot-prefixed names are gitignored
    // and so would not survive a checkout.
    write_to_file(root / "output" / "TestSummaries" / ".DS_Store", "not sequoia's", std::ios_base::out);
    const auto hidden{root / "output" / "DiagnosticsOutput" / ".hidden"};
    fs::create_directories(hidden);
    write_to_file(hidden / "inside.txt", "nor this", std::ios_base::out);

    // Written here so that the bytes are known: a checkout would give them the platform's line endings.
    write_to_file(root / "output" / "TestSummaries" / "gamma.txt", "gamma\r\n", std::ios_base::out | std::ios_base::binary);

    const auto snapshot{take_versioned_output_snapshot(output_paths{root})};

    check(equality,
          "Both versioned trees are gathered recursively; nothing else under output/ is, nor dot-files, "
          "nor the contents of a dot-directory",
          snapshot | std::views::keys | std::ranges::to<std::vector>(),
          std::vector<fs::path>{"DiagnosticsOutput/Sub/alpha.txt", "DiagnosticsOutput/empty.txt", "TestSummaries/beta.txt", "TestSummaries/gamma.txt"});

    check(equality, "Content is captured", snapshot.at("TestSummaries/beta.txt"), std::string{"beta\n"});
    check(equality, "Content is captured byte for byte", snapshot.at("TestSummaries/gamma.txt"), std::string{"gamma\r\n"});

    // `versioned_write` truncates rather than deletes, so an empty versioned file is a legitimate
    // state and must be captured rather than skipped.
    check(equality, "An empty file is an entry, not an absence", snapshot.at("DiagnosticsOutput/empty.txt"), std::string{});
  }
}
