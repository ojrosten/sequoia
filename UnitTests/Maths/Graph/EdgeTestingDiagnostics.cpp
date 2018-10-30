#include "EdgeTestingDiagnostics.hpp"

namespace sequoia::unit_testing
{
  void test_edge_false_positives::run_tests()
  {
    test_plain_partial_edge();      
    test_partial_edge_indep_weight(); 
    test_partial_edge_shared_weight();

    test_plain_embedded_partial_edge();
    test_embedded_partial_edge_indep_weight();
    test_embedded_partial_edge_shared_weight();
      
    test_plain_edge();      
    test_weighted_edge();

    test_plain_embedded_edge();
    test_embedded_edge_indep_weight();
    test_embedded_edge_shared_weight();    
  }


  void test_edge_false_positives::test_plain_partial_edge()
  {
    using namespace maths;
    using namespace data_sharing;      
    using edge_t = partial_edge<null_weight, independent, utilities::protective_wrapper<null_weight>>;

    check_equality(edge_t{0}, edge_t{1}, LINE("Differing target indices"));
  }
  
  void test_edge_false_positives::test_partial_edge_indep_weight()
  {
  }

  void test_edge_false_positives::test_partial_edge_shared_weight()
  {
  }

  void test_edge_false_positives::test_plain_embedded_partial_edge()
  {
  }
  
  void test_edge_false_positives::test_embedded_partial_edge_indep_weight()
  {
  }
  
  void test_edge_false_positives::test_embedded_partial_edge_shared_weight()
  {
  }
      
  void test_edge_false_positives::test_plain_edge()
  {
  }
  
  void test_edge_false_positives::test_weighted_edge()
  {
  }

  void test_edge_false_positives::test_plain_embedded_edge()
  {
  }
  
  void test_edge_false_positives::test_embedded_edge_indep_weight()
  {
  }
  
  void test_edge_false_positives::test_embedded_edge_shared_weight()
  {
  }
}
