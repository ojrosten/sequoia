////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "GlobalFreeTest.hpp"
#include "fakeProject/Global/Stuff/Global.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view global_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void global_free_test::run_tests()
  {
    // TO DO
  }
}
