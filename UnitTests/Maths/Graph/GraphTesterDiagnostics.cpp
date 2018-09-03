#include "GraphTesterDiagnostics.hpp"

namespace sequoia::unit_testing
{
  void test_graph_false_positives::run_tests()
  {
    struct null_weight {};
    
    graph_test_helper<null_weight, null_weight> helper;
    helper.run_tests<generic_graph_false_positives>(*this);
  }  
}
