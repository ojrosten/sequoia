////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MaterialsUpdaterFreeTest.hpp"
#include "sequoia/TestFramework/MaterialsUpdater.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view materials_updater_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void materials_updater_free_test::run_tests()
  {
    const auto& auxiliary{auxiliary_materials()}, working{working_materials()}, predictive{predictive_materials()};

    check_exception_thrown<std::runtime_error>(LINE("Empty 'to' path"),   [&]() { soft_update("", working); });
    check_exception_thrown<std::runtime_error>(LINE("Empty 'from' path"), [&]() { soft_update(auxiliary, ""); });

    soft_update(auxiliary, working);
    check_weak_equivalence(LINE("Soft update"), working, predictive);

    check_equality(LINE("Ensure that a target file equivalent to its replacement is not replaced"),
                   read_to_string(working_materials() / "DirToBeKept" / "Comments.txt"),
                   read_to_string(predictive_materials() / "DirToBeKept" / "Comments.txt"));

    check(LINE("Ensure fidelity of previous check"),
              read_to_string(working_materials() / "DirToBeKept" / "Comments.txt")
          !=  read_to_string(auxiliary_materials() / "DirToBeKept" / "Comments.txt"));
  }
}
