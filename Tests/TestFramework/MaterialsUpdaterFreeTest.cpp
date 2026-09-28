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
    test_incomplete_update_copies();
    test_update();
    test_update_of_disjoint_directories();
    test_deletions_recorded_before_a_throw();
    test_unsupported_entry_type();
  }

  void materials_updater_free_test::test_incomplete_update_copies()
  {
    const std::vector<fs::path> deleted{"Gone.txt"};

    incomplete_update source{"", deleted};
    const incomplete_update constructed{std::move(source)};

    check(equality,
          "Deletions of a move-constructed incomplete_update",
          std::ranges::to<std::vector>(constructed.deleted()),
          deleted);

    check(equality,
          "Deletions of an incomplete_update moved from by construction",
          std::ranges::to<std::vector>(source.deleted()),
          deleted);

    incomplete_update assigned{"", {"Other.txt"}};
    assigned = std::move(source);

    check(equality,
          "Deletions of a move-assigned incomplete_update",
          std::ranges::to<std::vector>(assigned.deleted()),
          deleted);

    check(equality,
          "Deletions of an incomplete_update moved from by assignment",
          std::ranges::to<std::vector>(source.deleted()),
          deleted);
  }

  void materials_updater_free_test::test_update()
  {
    const auto auxiliary{auxiliary_materials()}, working{working_materials()}, predictive{predictive_materials()};

    check_exception_thrown<std::runtime_error>("Empty 'from' path", [&]() { return soft_update("", working); });
    check_exception_thrown<std::runtime_error>("Empty 'to' path",   [&]() { return soft_update(auxiliary, ""); });

    // In the scratchpad rather than the materials, so that neither the update nor the equivalence check below sees it
    const transient_file notADirectory{scratchpad_materials() / "NotADirectory.txt", ""};

    check_exception_thrown<std::runtime_error>(
      "'to' path exists but is not a directory",
      [&]() { return soft_update(auxiliary, notADirectory.path()); });

    check_exception_thrown<std::runtime_error>(
      "'from' path exists but is not a directory",
      [&]() { return soft_update(notADirectory.path(), working); });

    check(equality,
          "Paths the update deleted",
          soft_update(auxiliary, working),
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

  /** The two directories share no entry names, and their entries alternate when sorted. In the
      first case the last entry is `to`'s, and so is deleted once every entry of `from` has been added;
      in the second, the last entry is `from`'s, and so is added once every entry of `to` has been
      deleted.
   */
  void materials_updater_free_test::test_update_of_disjoint_directories()
  {
    auto makeDirectory{
      [](const fs::path& dir, std::initializer_list<std::string_view> fileNames) {
        fs::create_directories(dir);
        for(const auto name : fileNames)
        {
          write_to_file(dir / name, "", std::ios_base::out);
        }

        return dir;
      }
    };

    const auto root{scratchpad_materials() / "Disjoint"};

    {
      const auto from{makeDirectory(root / "LastEntryDeleted/From", {"a.txt", "c.txt"})},
                 to  {makeDirectory(root / "LastEntryDeleted/To",   {"b.txt", "d.txt"})};

      check(equality,
            "Deletions when the last entry is in 'to' alone",
            soft_update(from, to),
            std::vector<fs::path>{to / "b.txt", to / "d.txt"});

      check(weak_equivalence, "Update when the last entry is in 'to' alone", to, from);
    }

    {
      const auto from{makeDirectory(root / "LastEntryAdded/From", {"b.txt", "d.txt"})},
                 to  {makeDirectory(root / "LastEntryAdded/To",   {"a.txt", "c.txt"})};

      check(equality,
            "Deletions when the last entry is in 'from' alone",
            soft_update(from, to),
            std::vector<fs::path>{to / "a.txt", to / "c.txt"});

      check(weak_equivalence, "Update when the last entry is in 'from' alone", to, from);
    }
  }

  /** `from` holds a regular file `B` where `to` holds a directory. `soft_update` does not handle a
      change of type, and copying the file over the directory throws. `A` sorts before `B`, so the
      update has deleted `A/old.txt` by then.
   */
  void materials_updater_free_test::test_deletions_recorded_before_a_throw()
  {
    const auto root{scratchpad_materials() / "TypeSwap"};
    const auto from{root / "From"}, to{root / "To"};

    fs::create_directories(from / "A");
    write_to_file(from / "B", "", std::ios_base::out);

    fs::create_directories(to / "A");
    write_to_file(to / "A/old.txt", "", std::ios_base::out);
    fs::create_directories(to / "B");

    std::vector<fs::path> deleted{};

    check_exception_thrown<incomplete_update>(
      "A regular file in 'from' where 'to' holds a directory",
      [&]() {
        try
        {
          return soft_update(from, to);
        }
        catch(const incomplete_update& e)
        {
          deleted = std::ranges::to<std::vector>(e.deleted());
          throw;
        }
      },
      elide_message);

    check(equality, "Deletion recorded before the throw", deleted, std::vector<fs::path>{to / "A/old.txt"});
  }

  /** `from` and `to` each hold `Dangling`, a symbolic link to nothing. Its status, which follows the link,
      is neither a regular file nor a directory.
   */
  void materials_updater_free_test::test_unsupported_entry_type()
  {
    const auto root{scratchpad_materials() / "UnsupportedType"};
    const auto from{root / "From"}, to{root / "To"};

    fs::create_directories(from);
    fs::create_directories(to);
    fs::create_symlink(root / "Nowhere", from / "Dangling");
    fs::create_symlink(root / "Nowhere", to / "Dangling");

    check_exception_thrown<incomplete_update>(
      "An entry in both that is neither a regular file nor a directory",
      [&]() { return soft_update(from, to); });
  }
}
