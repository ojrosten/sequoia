#include "TestFixedTopology.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    void test_fixed_topology::run_tests()
    {
      using namespace maths;

      {
        graph_test_helper<int, null_weight> helper{"int, unweighted"};
        helper.run_tests<generic_fixed_topology_tests>(*this);
      }

      {
        graph_test_helper<int, int> helper{"int, int"};
        helper.run_tests<generic_fixed_topology_tests>(*this);
      }
    }
  }
}
