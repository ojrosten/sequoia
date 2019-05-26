////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphUpdateTest.hpp"

namespace sequoia::unit_testing
{
  void test_graph_update::run_tests()
  {
    graph_test_helper<std::vector<double>, std::vector<int>> helper;
    helper.run_tests<test_BF_update>(*this);

    graph_test_helper<size_t, size_t> helper2;
    helper2.run_tests<test_update>(*this);
  }
}
