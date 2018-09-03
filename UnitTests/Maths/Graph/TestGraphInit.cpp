#include "TestGraphInit.hpp"

#include <complex>

namespace sequoia
{
  namespace unit_testing
  {
    void test_graph_init::run_tests()
    {
      using namespace maths;

      {
        graph_test_helper<null_weight, null_weight> helper{"Unweighted, Unweighted"};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<null_weight, int> helper{"Unweighted, int"};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<null_weight, unsortable> helper{"Unweighted, unsortable"};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<null_weight, big_unsortable> helper{"Unweighted, big unsortable"};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<int, null_weight> helper{"int, Unweighted"};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<int, int> helper{"int, int"};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<int, unsortable> helper{"int, unsortable"};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<int, big_unsortable> helper{"int, big unsortable"};
        helper.run_tests<test_initialization>(*this);
      }
    }
  }
}
