////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "VersionedOutputFreeTest.hpp"

#include "sequoia/Runtime/ShellCommands.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TestFramework/VersionedOutput.hpp"

#include <ranges>
#include <vector>

namespace sequoia::testing
{
  using namespace runtime;
  namespace fs = std::filesystem;

  [[nodiscard]]
  fs::path versioned_output_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void versioned_output_free_test::run_tests()
  {
    test_comparisons();
    test_reporting();
    test_patch();
    test_snapshot();
    test_patch_round_trip();
  }

  void versioned_output_free_test::test_comparisons()
  {
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

  void versioned_output_free_test::test_patch()
  {
    // The patch is git's own unified format, so each expectation here is what `git diff` prints
    // for the same change, but for two things: the hunk spans the whole file rather than the least
    // of it, and there is no `index` line, which is why an empty file added or removed is a header
    // alone.

    check(equality, "Nothing to patch", unified_diff({{"a.txt", "alpha\n"}}, {{"a.txt", "alpha\n"}}, "output"), std::string{});

    check(equality,
          "A modified file is one hunk, its old lines removed then its new lines added",
          unified_diff({{"a.txt", "alpha\n"}}, {{"a.txt", "alpha\nbeta\n"}}, "output"),
          std::string{"diff --git a/output/a.txt b/output/a.txt\n"
                      "--- a/output/a.txt\n"
                      "+++ b/output/a.txt\n"
                      "@@ -1,1 +1,2 @@\n"
                      "-alpha\n"
                      "+alpha\n"
                      "+beta\n"});

    // `fs::path{"Sub"} / "b.txt"` carries the platform's separator, as a snapshot's keys do; the
    // patch must nevertheless read the same everywhere.
    check(equality,
          "An added file comes from /dev/null, its path rendered generically",
          unified_diff({}, {{fs::path{"Sub"} / "b.txt", "beta\n"}}, "output"),
          std::string{"diff --git a/output/Sub/b.txt b/output/Sub/b.txt\n"
                      "new file mode 100644\n"
                      "--- /dev/null\n"
                      "+++ b/output/Sub/b.txt\n"
                      "@@ -0,0 +1,1 @@\n"
                      "+beta\n"});

    check(equality,
          "A removed file goes to /dev/null",
          unified_diff({{"a.txt", "alpha\n"}}, {}, "output"),
          std::string{"diff --git a/output/a.txt b/output/a.txt\n"
                      "deleted file mode 100644\n"
                      "--- a/output/a.txt\n"
                      "+++ /dev/null\n"
                      "@@ -1,1 +0,0 @@\n"
                      "-alpha\n"});

    check(equality,
          "A final line lacking its newline is a line, and is marked as git marks it",
          unified_diff({{"a.txt", "alpha"}}, {{"a.txt", "beta\n"}}, "output"),
          std::string{"diff --git a/output/a.txt b/output/a.txt\n"
                      "--- a/output/a.txt\n"
                      "+++ b/output/a.txt\n"
                      "@@ -1,1 +1,1 @@\n"
                      "-alpha\n"
                      "\\ No newline at end of file\n"
                      "+beta\n"});

    check(equality,
          "A carriage return is content, and is carried",
          unified_diff({{"a.txt", "alpha\r\n"}}, {{"a.txt", "beta\r\n"}}, "output"),
          std::string{"diff --git a/output/a.txt b/output/a.txt\n"
                      "--- a/output/a.txt\n"
                      "+++ b/output/a.txt\n"
                      "@@ -1,1 +1,1 @@\n"
                      "-alpha\r\n"
                      "+beta\r\n"});

    check(equality,
          "An empty file added or removed is a header alone",
          unified_diff({{"gone.txt", ""}}, {{"new.txt", ""}}, "output"),
          std::string{"diff --git a/output/new.txt b/output/new.txt\n"
                      "new file mode 100644\n"
                      "diff --git a/output/gone.txt b/output/gone.txt\n"
                      "deleted file mode 100644\n"});

    check(equality,
          "A file emptied, or filled from empty, has a hunk with one side of zero extent",
          unified_diff({{"emptied.txt", "was\n"}, {"filled.txt", ""}}, {{"emptied.txt", ""}, {"filled.txt", "now\n"}}, "output"),
          std::string{"diff --git a/output/emptied.txt b/output/emptied.txt\n"
                      "--- a/output/emptied.txt\n"
                      "+++ b/output/emptied.txt\n"
                      "@@ -1,1 +0,0 @@\n"
                      "-was\n"
                      "diff --git a/output/filled.txt b/output/filled.txt\n"
                      "--- a/output/filled.txt\n"
                      "+++ b/output/filled.txt\n"
                      "@@ -0,0 +1,1 @@\n"
                      "+now\n"});

    check(equality,
          "Files are patched in the order they are reported: added, removed, modified",
          unified_diff({{"a.txt", "alpha\n"}, {"b.txt", "beta\n"}}, {{"a.txt", "not alpha\n"}, {"c.txt", "gamma\n"}}, "output"),
          std::string{"diff --git a/output/c.txt b/output/c.txt\n"
                      "new file mode 100644\n"
                      "--- /dev/null\n"
                      "+++ b/output/c.txt\n"
                      "@@ -0,0 +1,1 @@\n"
                      "+gamma\n"
                      "diff --git a/output/b.txt b/output/b.txt\n"
                      "deleted file mode 100644\n"
                      "--- a/output/b.txt\n"
                      "+++ /dev/null\n"
                      "@@ -1,1 +0,0 @@\n"
                      "-beta\n"
                      "diff --git a/output/a.txt b/output/a.txt\n"
                      "--- a/output/a.txt\n"
                      "+++ b/output/a.txt\n"
                      "@@ -1,1 +1,1 @@\n"
                      "-alpha\n"
                      "+not alpha\n"});
  }

  void versioned_output_free_test::test_snapshot()
  {
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

  void versioned_output_free_test::test_patch_round_trip()
  {
    // The claim the patch exists for: applied by git to the tree it was taken from, the tree
    // becomes the second snapshot, byte for byte. The tree is the one test_snapshot left, so it
    // carries a carriage return and a hidden file, and every kind of change is made to it.

    const auto root{working_materials()};
    const auto before{take_versioned_output_snapshot(output_paths{root})};

    auto after{before};
    after.at("TestSummaries/beta.txt") = "beta\r\nand no newline";
    after.at("TestSummaries/gamma.txt") = "";
    after.at("DiagnosticsOutput/empty.txt") = "filled\n";
    after.erase("DiagnosticsOutput/Sub/alpha.txt");
    after.emplace(fs::path{"DiagnosticsOutput"} / "New" / "delta.txt", "delta\n");

    write_to_file(root / "VersionedOutput.patch", unified_diff(before, after, "output"), std::ios_base::out | std::ios_base::binary);

    // Outside a repository git applies a patch relative to the current directory; inside one,
    // relative to its root, silently ignoring whatever is not beneath the current directory - and
    // this tree lies within sequoia's own. A repository of its own makes it the root. Without the
    // autocrlf override a Windows checkout would rewrite the line endings the patch carries.
    check("The patch applies", invoke(cd_cmd(root) && "git init -q" && "git -c core.autocrlf=false apply --whitespace=nowarn VersionedOutput.patch") == 0);
    check(equality, "The patched tree is the second snapshot, byte for byte", take_versioned_output_snapshot(output_paths{root}), after);
  }
}
