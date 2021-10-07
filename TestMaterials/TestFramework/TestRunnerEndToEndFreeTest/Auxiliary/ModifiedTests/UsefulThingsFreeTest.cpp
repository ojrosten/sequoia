////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UsefulThingsFreeTest.hpp"
#include "generatedProject/Utilities/UsefulThings.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  [[nodiscard]]
  std::string_view useful_things_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void useful_things_free_test::run_tests()
  {
    check_equality(LINE(""), utils::foo(), 42);
    check(LINE(""), !fs::exists(temp_test_summaries_path(test_repository().parent_path() / "output")));
  }
}
