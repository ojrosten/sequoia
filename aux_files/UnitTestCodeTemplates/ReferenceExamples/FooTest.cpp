////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FooTestingUtilities.hpp"
#include "FooTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view foo_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void foo_test::run_tests()
  {
    // TO DO
  }
}
