////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestFixedTopology.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    void test_fixed_topology::run_tests()
    {
      using namespace maths;

      {
        graph_test_helper<int, null_weight> helper{};
        helper.run_tests<generic_fixed_topology_tests>(*this);
      }

      {
        graph_test_helper<int, int> helper{};
        helper.run_tests<generic_fixed_topology_tests>(*this);
      }
    }
  }
}
