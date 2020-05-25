////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ExperimentalTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view experimental_test::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void experimental_test::run_tests()
  {
  }
}
