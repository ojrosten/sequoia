////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "OutputFreeTest.hpp"
#include "sequoia/TestFramework/Output.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view output_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void output_free_test::run_tests()
  {
    test_emphasise();
  }

  void output_free_test::test_emphasise()
  {
    using namespace std::string_literals;
    check_equality(LINE("Emphasis"), emphasise("foo"), "--foo--"s);
    check_equality(LINE("Nothing to emphasise"), emphasise(""), ""s);
  }
}
