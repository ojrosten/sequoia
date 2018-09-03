#include "TestGraph.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    void test_graph::run_tests()
    {
      using std::complex;
      using namespace maths;

      struct null_weight {};
      
      {
        graph_test_helper<null_weight, null_weight> helper{"Unweighted"};
        helper.run_tests<generic_graph_operations>(*this);
      }
      
      {
        graph_test_helper<complex<double>, int>  helper{"Weighted (complex<double>,int)"};
        helper.run_tests<generic_weighted_graph_tests>(*this);
      }

      {
        graph_test_helper<complex<double>, complex<int>>  helper{"Weighted (complex<double>, complex<int>)"};
        helper.run_tests<generic_weighted_graph_tests>(*this);
      }

      {
        graph_test_helper<int, complex<double>> helper{"Weighted (int, complex<double>)"};
        helper.run_tests<more_generic_weighted_graph_tests>(*this);
      }
      
      {
        graph_test_helper<std::vector<int>, std::vector<int>>  helper{"Weighted (vector<int>, vector<int>)"};
        helper.run_tests<test_copy_move>(*this);
      }
    }    
  }
}
