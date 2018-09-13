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
        graph_test_helper<int, null_weight> helper{"int, unweighted"};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<unsortable, null_weight> helper{"unsortable, unweighted"};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<big_unsortable, null_weight> helper{"big unsortable, unweighted"};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<null_weight, int> helper{"unweighted, int"};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<int, int> helper{"int, int"};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<unsortable, int> helper{"unsortable, int"};
        helper.run_tests<test_initialization>(*this);
      }

      {
        graph_test_helper<big_unsortable, int> helper{"big unsortable, int"};
        helper.run_tests<test_initialization>(*this);
      }
    }
  }
}
