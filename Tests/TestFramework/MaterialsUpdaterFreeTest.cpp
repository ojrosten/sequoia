////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MaterialsUpdaterFreeTest.hpp"
#include "sequoia/TestFramework/MaterialsUpdater.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TestFramework/SumTypeCheckers.hpp"
#include "Utilities/TestUtilities.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    // The wording and the form of the path are the standard library's, so only the type is witnessed
    const auto elide_message{
      [](const project_paths&, std::string) { return std::string{"[Message elided: it varies by standard library]"}; }
    };
  }

  [[nodiscard]]
  fs::path materials_updater_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void materials_updater_free_test::run_tests()
  {
    test_update();
    test_deletions_recorded_before_a_throw();
  }

  void materials_updater_free_test::test_update()
  {
    const auto auxiliary{auxiliary_materials()}, working{working_materials()}, predictive{predictive_materials()};

    const auto update{
      [](const fs::path& from, const fs::path& to) {
        std::vector<fs::path> deleted{};
        soft_update(from, to, deleted);
      }
    };

    check_exception_thrown<std::runtime_error>("Empty 'to' path",   [&]() { update("", working); });
    check_exception_thrown<std::runtime_error>("Empty 'from' path", [&]() { update(auxiliary, ""); });

    // Beside the materials rather than in them, so that neither the update nor the equivalence check below sees it
    const transient_file notADirectory{auxiliary.parent_path() / "NotADirectory.txt", ""};

    check_exception_thrown<fs::filesystem_error>(
      "'to' path exists but is not a directory",
      [&]() { update(auxiliary, notADirectory.path()); },
      elide_message);

    check_exception_thrown<fs::filesystem_error>(
      "'from' path exists but is not a directory",
      [&]() { update(notADirectory.path(), working); },
      elide_message);

    std::vector<fs::path> deleted{};
    soft_update(auxiliary, working, deleted);

    check(equality,
          "Paths the update deleted",
          deleted,
          std::vector<fs::path>{
            working / "AnotherDirToBeRemoved",
            working / "DirToBeRemoved",
            working / "DirWithFewerDirs/Gone",
            working / "DirWithFewerFiles/file2.txt",
            working / "ToBeRemoved.seqpat",
            working / "ToBeRemoved.txt"
          });

    check(weak_equivalence, "Soft update", working, predictive);

    check(equality, "Ensure that a target file equivalent to its replacement is not replaced",
                   read_to_string(working_materials() /= "DirToBeKept/Comments.txt", std::ios_base::in),
                   read_to_string(predictive_materials() /= "DirToBeKept/Comments.txt", std::ios_base::in));

    check("Ensure fidelity of previous check",
              read_to_string(working_materials() /= "DirToBeKept/Comments.txt", std::ios_base::in)
          !=  read_to_string(auxiliary_materials() /= "DirToBeKept/Comments.txt", std::ios_base::in));
  }

  /** `from` holds a regular file `B` where `to` holds a directory. `soft_update` does not handle a
      change of type, and copying the file over the directory throws. `A` sorts before `B`, so the
      update has deleted `A/old.txt` by then.
   */
  void materials_updater_free_test::test_deletions_recorded_before_a_throw()
  {
    const auto root{auxiliary_materials().parent_path() / "TypeSwap"};
    const auto from{root / "From"}, to{root / "To"};

    fs::create_directories(from / "A");
    write_to_file(from / "B", "", std::ios_base::out);

    fs::create_directories(to / "A");
    write_to_file(to / "A/old.txt", "", std::ios_base::out);
    fs::create_directories(to / "B");

    std::vector<fs::path> deleted{};

    check_exception_thrown<fs::filesystem_error>(
      "A regular file in 'from' where 'to' holds a directory",
      [&]() { soft_update(from, to, deleted); },
      elide_message);

    check(equality, "Deletion recorded before the throw", deleted, std::vector<fs::path>{to / "A/old.txt"});
  }
}
