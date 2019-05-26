////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphInitializationTest.hpp"

#include <complex>

namespace sequoia
{
  namespace unit_testing
  {
    void test_graph_init::run_tests()
    {
      using namespace maths;

      {
        graph_test_helper<null_weight, null_weight> helper{};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<int, null_weight> helper{};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<unsortable, null_weight> helper{};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<big_unsortable, null_weight> helper{};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<null_weight, int> helper{};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<int, int> helper{};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<unsortable, int> helper{};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<big_unsortable, int> helper{};
        helper.run_tests<test_initialization>(*this);
      }
    }
  }
}
