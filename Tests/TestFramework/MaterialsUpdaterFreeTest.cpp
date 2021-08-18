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
    // e.g. check_equality(LINE("Useful description"), some_function(), 42);
  }
}
