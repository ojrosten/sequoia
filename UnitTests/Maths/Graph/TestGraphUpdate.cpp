#include "TestGraphUpdate.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    void test_graph_update::run_tests()
    {
      graph_test_helper<std::vector<double>, std::vector<int>> helper;
      helper.run_tests<test_BF_update>(*this);

      graph_test_helper<size_t, size_t> helper2;
      helper2.run_tests<test_update>(*this);
    }
  }
}
