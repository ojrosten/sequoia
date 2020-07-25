////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MyClassTest.hpp"

// #include "MyClass.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view my_class_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void my_class_test::run_tests()
  {
    // TO DO
  }
}
