////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UsefulThingsFreeTest.hpp"
#include "generatedProject/Utilities/UsefulThings.hpp"

namespace generatedProject::testing
{
  [[nodiscard]]
  std::filesystem::path useful_things_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void useful_things_free_test::run_tests()
  {
    check(equality, "", utils::foo(), 42);
    check_exception_thrown<std::runtime_error>("", [](){ throw std::runtime_error{"Uh-oh"};  });
  }
}
