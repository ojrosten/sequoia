////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "MaterialsUpdaterFreeTest.hpp"
#include "sequoia/TestFramework/MaterialsUpdater.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path materials_updater_free_test::source_file() const noexcept
  {
    return std::source_location::current().file_name();
  }

  void materials_updater_free_test::run_tests()
  {
    const auto auxiliary{auxiliary_materials()}, working{working_materials()}, predictive{predictive_materials()};

    check_exception_thrown<std::runtime_error>(report_line("Empty 'to' path"),   [&]() { soft_update("", working); });
    check_exception_thrown<std::runtime_error>(report_line("Empty 'from' path"), [&]() { soft_update(auxiliary, ""); });

    soft_update(auxiliary, working);
    check(weak_equivalence, report_line("Soft update"), working, predictive);

    check(equality, report_line("Ensure that a target file equivalent to its replacement is not replaced"),
                   read_to_string(working_materials() /= "DirToBeKept/Comments.txt"),
                   read_to_string(predictive_materials() /= "DirToBeKept/Comments.txt"));

    check(report_line("Ensure fidelity of previous check"),
              read_to_string(working_materials() /= "DirToBeKept/Comments.txt")
          !=  read_to_string(auxiliary_materials() /= "DirToBeKept/Comments.txt"));
  }
}
