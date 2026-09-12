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

#include <fstream>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path materials_updater_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void materials_updater_free_test::run_tests()
  {
    const auto auxiliary{auxiliary_materials()}, working{working_materials()}, predictive{predictive_materials()};

    check_exception_thrown<std::runtime_error>("Empty 'to' path",   [&]() { soft_update("", working); });
    check_exception_thrown<std::runtime_error>("Empty 'from' path", [&]() { soft_update(auxiliary, ""); });

    // Beside the materials rather than in them, so that neither the update nor the equivalence check below sees it
    const auto notADirectory{auxiliary.parent_path() / "NotADirectory.txt"};
    { std::ofstream{notADirectory}; }

    // The wording and the form of the path are the standard library's, so only the type is witnessed
    const auto elideMessage{[](const project_paths&, std::string) { return std::string{"[Message elided: it varies by standard library]"}; }};

    check_exception_thrown<std::filesystem::filesystem_error>(
      "'to' path exists but is not a directory",
      [&]() { soft_update(auxiliary, notADirectory); },
      elideMessage);

    check_exception_thrown<std::filesystem::filesystem_error>(
      "'from' path exists but is not a directory",
      [&]() { soft_update(notADirectory, working); },
      elideMessage);

    std::filesystem::remove(notADirectory);

    soft_update(auxiliary, working);
    check(weak_equivalence, "Soft update", working, predictive);

    check(equality, "Ensure that a target file equivalent to its replacement is not replaced",
                   read_to_string(working_materials() /= "DirToBeKept/Comments.txt"),
                   read_to_string(predictive_materials() /= "DirToBeKept/Comments.txt"));

    check("Ensure fidelity of previous check",
              read_to_string(working_materials() /= "DirToBeKept/Comments.txt")
          !=  read_to_string(auxiliary_materials() /= "DirToBeKept/Comments.txt"));
  }
}
