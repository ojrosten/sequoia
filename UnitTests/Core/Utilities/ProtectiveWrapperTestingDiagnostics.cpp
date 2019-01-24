////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ProtectiveWrapperTestingDiagnostics.hpp"

namespace sequoia::unit_testing
{
  void protective_wrapper_false_positive_test::run_tests()
  {
    test_basic_type();
    test_container_type();
    test_aggregate_type();
  }

  void protective_wrapper_false_positive_test::test_basic_type()
  {
    using namespace utilities;

    protective_wrapper<int> w{1}, v{};
    check_equality(w, v, LINE(""));
  }

  void protective_wrapper_false_positive_test::test_container_type()
  {
    using namespace utilities;

    protective_wrapper<std::vector<int>> w{}, v{1};
    check_equality(w, v, LINE(""));

    w.mutate([](auto& vec) { vec.push_back(2); });
    check_equality(w, v, LINE(""));

    protective_wrapper<std::vector<int>> u{std::vector<int>(1)};
    check_equality(u, v, LINE(""));
  }

  void protective_wrapper_false_positive_test::test_aggregate_type()
  {
    using namespace utilities;
    
    protective_wrapper<data> w{}, v{1, 2.0};
    check_equality(w, v, LINE(""));
  }
}
