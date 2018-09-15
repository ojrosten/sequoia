#include "TestGraphTraversals.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    void test_graph_traversals::run_tests()
    {
      test_PRS_helpers();

      {
        graph_test_helper<null_weight, null_weight> helper;
        helper.run_tests<tracker_test>(*this);
      }

      {
        graph_test_helper<null_weight, int> helper;
        helper.run_tests<test_weighted_BFS_tasks>(*this);
      }
      
      {
        graph_test_helper<null_weight, int> helper;
        helper.run_tests<test_priority_traversal>(*this);
      }
    }
  }
}
