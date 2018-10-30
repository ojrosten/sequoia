#include "EdgeTestingDiagnostics.hpp"

#include "SharingPolicies.hpp"

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

    using compact_edge_t = partial_edge<null_weight, independent, utilities::protective_wrapper<null_weight>, unsigned char>;

    check_equality(compact_edge_t{10}, compact_edge_t{255}, LINE("Differing target indices"));
  }
  
  void test_edge_false_positives::test_partial_edge_indep_weight()
  {
    using namespace maths;
    using namespace data_sharing;

    using edge_t = partial_edge<int, independent, utilities::protective_wrapper<int>>;

    check_equality(edge_t{0,0}, edge_t{1,0}, LINE("Differing targets, identical weights"));
    check_equality(edge_t{0,5}, edge_t{1,5}, LINE("Differing targets, identical weights"));
    check_equality(edge_t{0,0}, edge_t{0,1}, LINE("Identical targets, differeing weights"));
    check_equality(edge_t{0,5}, edge_t{0,6}, LINE("Identical targets, differeing weights"));
    check_equality(edge_t{0,0}, edge_t{1,1}, LINE("Differing targets and weights"));
    check_equality(edge_t{0,1}, edge_t{2,3}, LINE("Differing targets and weights")); 
  }

  void test_edge_false_positives::test_partial_edge_shared_weight()
  {
    using namespace maths;
    using namespace data_sharing;

    using edge_t = partial_edge<int, shared, utilities::protective_wrapper<int>>;

    check_equality(edge_t{0,0}, edge_t{1,0}, LINE("Differing targets, identical weights"));
    check_equality(edge_t{0,5}, edge_t{1,5}, LINE("Differing targets, identical weights"));
    check_equality(edge_t{0,0}, edge_t{0,1}, LINE("Identical targets, differeing weights"));
    check_equality(edge_t{0,5}, edge_t{0,6}, LINE("Identical targets, differeing weights"));
    check_equality(edge_t{0,0}, edge_t{1,1}, LINE("Differing targets and weights"));
    check_equality(edge_t{0,1}, edge_t{2,3}, LINE("Differing targets and weights"));  
  }

  void test_edge_false_positives::test_plain_embedded_partial_edge()
  {
    using namespace maths;
    using namespace data_sharing;

    using edge_t = partial_edge<int, shared, utilities::protective_wrapper<int>>;

    check_equality(edge_t{0,0}, edge_t{1,0}, LINE("Differing targets, identical complementary indices"));
    check_equality(edge_t{0,5}, edge_t{1,5}, LINE("Differing targets, identical complementary indices"));
    check_equality(edge_t{0,0}, edge_t{0,1}, LINE("Identical targets, differeing complementary indices"));
    check_equality(edge_t{0,5}, edge_t{0,6}, LINE("Identical targets, differeing complementary indices"));
    check_equality(edge_t{0,0}, edge_t{1,1}, LINE("Differing targets and complementary indices"));
    check_equality(edge_t{0,1}, edge_t{1,0}, LINE("Differing targets and complementary indices")); 
  }
  
  void test_edge_false_positives::test_embedded_partial_edge_indep_weight()
  {
    using namespace maths;
    using namespace data_sharing;

    using edge_t = embedded_partial_edge<double, independent, utilities::protective_wrapper<double>>;

    check_equality(edge_t{0,0,0.0}, edge_t{1,0,0.0}, LINE("Differing targets, identical complementary indices and weights"));
    check_equality(edge_t{1,10,0.0}, edge_t{0,10,0.0}, LINE("Differing targets, identical complementary indices and weights"));
    check_equality(edge_t{0,20,5.0}, edge_t{1,20,5.0}, LINE("Differing targets, identical complementary indices and weights"));

    check_equality(edge_t{0,0,0.0}, edge_t{0,1,0.0}, LINE("Differing complementary indices, identical targets and weights"));
    check_equality(edge_t{3,0,0.0}, edge_t{3,1,0.0}, LINE("Differing complementary indices, identical targets and weights"));
    check_equality(edge_t{4,0,-7.0}, edge_t{4,1,-7.0}, LINE("Differing complementary indices, identical targets and weights"));

    check_equality(edge_t{0,0,0.0}, edge_t{0,0,1.0}, LINE("Differing weights, identical targets and complementary indices"));
    check_equality(edge_t{3,0,0.0}, edge_t{3,0,1.0}, LINE("Differing weights, identical targets and complementary indices"));
    check_equality(edge_t{3,4,0.0}, edge_t{3,4,1.0}, LINE("Differing weights, identical targets and complementary indices"));

    check_equality(edge_t{0,1,0.0}, edge_t{1,0,0.0}, LINE("Differing targets and complementary indices, identical weights"));
    check_equality(edge_t{0,1,6.0}, edge_t{2,1,5.0}, LINE("Differing targets and weights, identical complementary indices"));
    check_equality(edge_t{0,1,4.0}, edge_t{0,3,5.0}, LINE("Differing complementary indices and weights, identical targets"));

    check_equality(edge_t{0,1,2.0}, edge_t{1,0,5.0}, LINE("Differing targets, complementary indices and weights"));    
  }
  
  void test_edge_false_positives::test_embedded_partial_edge_shared_weight()
  {
    using namespace maths;
    using namespace data_sharing;

     using edge_t = embedded_partial_edge<double, independent, utilities::protective_wrapper<double>>;

    check_equality(edge_t{0,0,0.0}, edge_t{1,0,0.0}, LINE("Differing targets, identical complementary indices and weights"));
    check_equality(edge_t{0,10,0.0}, edge_t{1,10,0.0}, LINE("Differing targets, identical complementary indices and weights"));
    check_equality(edge_t{0,20,5.0}, edge_t{1,20,5.0}, LINE("Differing targets, identical complementary indices and weights"));

    check_equality(edge_t{0,0,0.0}, edge_t{0,1,0.0}, LINE("Differing complementary indices, identical targets and weights"));
    check_equality(edge_t{3,0,0.0}, edge_t{3,1,0.0}, LINE("Differing complementary indices, identical targets and weights"));
    check_equality(edge_t{4,0,-7.0}, edge_t{4,1,-7.0}, LINE("Differing complementary indices, identical targets and weights"));

    check_equality(edge_t{0,0,0.0}, edge_t{0,0,1.0}, LINE("Differing weights, identical targets and complementary indices"));
    check_equality(edge_t{3,0,0.0}, edge_t{3,0,1.0}, LINE("Differing weights, identical targets and complementary indices"));
    check_equality(edge_t{3,4,0.0}, edge_t{3,4,1.0}, LINE("Differing weights, identical targets and complementary indices"));

    check_equality(edge_t{0,1,0.0}, edge_t{1,0,0.0}, LINE("Differing targets and complementary indices, identical weights"));
    check_equality(edge_t{0,1,6.0}, edge_t{2,1,5.0}, LINE("Differing targets and weights, identical complementary indices"));
    check_equality(edge_t{0,1,4.0}, edge_t{0,3,5.0}, LINE("Differing complementary indices and weights, identical targets"));

    check_equality(edge_t{0,1,2.0}, edge_t{1,0,5.0}, LINE("Differing targets, complementary indices and weights"));
  }
      
  void test_edge_false_positives::test_plain_edge()
  {
    using namespace maths;
    using namespace data_sharing;

    using edge_t = edge<null_weight, utilities::protective_wrapper<null_weight>>;

    check_equality(edge_t{0,0}, edge_t{0,1}, LINE("Differing targets, identical hosts"));
    check_equality(edge_t{4,1}, edge_t{4,0}, LINE("Differing targets, identical hosts"));

    check_equality(edge_t{0,0}, edge_t{1,0}, LINE("Differing hosts, identical targets"));
    check_equality(edge_t{1,8}, edge_t{0,8}, LINE("Differing hosts, identical targets"));

    check_equality(edge_t{0,1}, edge_t{1,0}, LINE("Differing hosts and targets"));

    check_equality(edge_t{0, inversion_constant<true>{}}, edge_t{0, inversion_constant<false>{}}, LINE("Differing inversion flag"));
  }
  
  void test_edge_false_positives::test_weighted_edge()
  {
    using namespace maths;
    using namespace data_sharing;

    using edge_t = edge<double, utilities::protective_wrapper<double>>;

    check_equality(edge_t{0,0,0.0}, edge_t{0,1,0.0}, LINE("Differing targets, identical hosts and weight"));
    check_equality(edge_t{0,10,0.0}, edge_t{1,10,0.0}, LINE("Differing targets, identical host and weights"));
    check_equality(edge_t{0,20,5.0}, edge_t{1,20,5.0}, LINE("Differing targets, identical host and weights"));
    check_equality(edge_t{0, inversion_constant<true>{}, 0.0}, edge_t{0, inversion_constant<false>{}, 0.0}, LINE("Differing inversion flag"));
    
    check_equality(edge_t{0,0,0.0}, edge_t{0,1,0.0}, LINE("Differing hosts, identical targets and weights"));
    check_equality(edge_t{3,0,0.0}, edge_t{3,1,0.0}, LINE("Differing hosts, identical targets and weights"));
    check_equality(edge_t{4,0,-7.0}, edge_t{4,1,-7.0}, LINE("Differing hosts, identical targets and weights"));

    check_equality(edge_t{0,0,0.0}, edge_t{0,0,1.0}, LINE("Differing weights, identical targets and hosts"));
    check_equality(edge_t{3,0,0.0}, edge_t{3,0,1.0}, LINE("Differing weights, identical targets and hosts"));
    check_equality(edge_t{3,4,0.0}, edge_t{3,4,1.0}, LINE("Differing weights, identical targets and hosts"));

    check_equality(edge_t{0,1,0.0}, edge_t{1,0,0.0}, LINE("Differing targets and hosts, identical weights"));
    check_equality(edge_t{0,1,6.0}, edge_t{2,1,5.0}, LINE("Differing targets and weights, identical hosts"));
    check_equality(edge_t{0,1,4.0}, edge_t{0,3,5.0}, LINE("Differing hosts and weights, identical targets"));

    check_equality(edge_t{0,1,2.0}, edge_t{1,0,5.0}, LINE("Differing targets, hosts and weights"));
  }

  void test_edge_false_positives::test_plain_embedded_edge()
  {
    using namespace maths;
    using namespace data_sharing;

    using edge_t = embedded_edge<null_weight, independent, utilities::protective_wrapper<null_weight>>;

    check_equality(edge_t{0,0,0},  edge_t{0,1,0},  LINE("Differing targets, identical hosts and complementary indices"));
    check_equality(edge_t{0,10,0}, edge_t{1,10,0}, LINE("Differing targets, identical host and complementary indices"));
    check_equality(edge_t{0,0,20}, edge_t{1,0,20}, LINE("Differing targets, identical host and complementary indices"));
    check_equality(edge_t{0, inversion_constant<true>{}, 0}, edge_t{0, inversion_constant<false>{}, 0}, LINE("Differing inversion flag"));

    check_equality(edge_t{0,0,0}, edge_t{0,1,0}, LINE("Differing hosts, identical targets and complementary indices"));
    check_equality(edge_t{3,0,0}, edge_t{3,1,0}, LINE("Differing hosts, identical targets and complementary indices"));
    check_equality(edge_t{4,0,7}, edge_t{4,1,7}, LINE("Differing hosts, identical targets and complementary indices"));

    check_equality(edge_t{0,0,0}, edge_t{0,0,1}, LINE("Differing complementary indicess, identical targets and hosts"));
    check_equality(edge_t{3,0,0}, edge_t{3,0,1}, LINE("Differing complementary indicess, identical targets and hosts"));
    check_equality(edge_t{3,4,0}, edge_t{3,4,1}, LINE("Differing complementary indicess, identical targets and hosts"));

    check_equality(edge_t{0,1,0}, edge_t{1,0,0}, LINE("Differing targets and hosts, identical complementary indices"));
    check_equality(edge_t{0,1,6}, edge_t{2,1,5}, LINE("Differing targets and complementary indices, identical hosts"));
    check_equality(edge_t{0,1,4}, edge_t{0,3,5}, LINE("Differing hosts and complementary indices, identical targets"));

    check_equality(edge_t{0,1,2}, edge_t{2,0,1}, LINE("Differing targets, hosts and complementary indicess"));
  }
  
  void test_edge_false_positives::test_embedded_edge_indep_weight()
  {
    using namespace maths;
    using namespace data_sharing;

    using edge_t = embedded_edge<double, independent, utilities::protective_wrapper<double>>;

    check_equality(edge_t{0,0,0,0.0}, edge_t{0,1,0,0.0}, LINE("Differing targets, identical hosts, complementary indices and weights"));
    check_equality(edge_t{1,0,0,0.0}, edge_t{0,0,0,0.0}, LINE("Differing hosts, identical targets, complementary indices and weights"));
    check_equality(edge_t{0,0,0,0.0}, edge_t{0,0,1,0.0}, LINE("Differing complementary indices, identical hosts, targets and weights"));
    check_equality(edge_t{0,0,0,0.0}, edge_t{0,0,0,1.0}, LINE("Differing weights, identical hosts, targets, complementary indices"));
    check_equality(edge_t{0, inversion_constant<true>{}, 0, 0.0}, edge_t{0, inversion_constant<false>{}, 0, 0.0}, LINE("Differing inversion flag"));

  }
  
  void test_edge_false_positives::test_embedded_edge_shared_weight()
  {
    using namespace maths;
    using namespace data_sharing;

    using edge_t = embedded_edge<double, shared, utilities::protective_wrapper<double>>;

    check_equality(edge_t{0,0,0,0.0}, edge_t{0,1,0,0.0}, LINE("Differing targets, identical hosts, complementary indices and weights"));
    check_equality(edge_t{1,0,0,0.0}, edge_t{0,0,0,0.0}, LINE("Differing hosts, identical targets, complementary indices and weights"));
    check_equality(edge_t{0,0,0,0.0}, edge_t{0,0,1,0.0}, LINE("Differing complementary indices, identical hosts, targets and weights"));
    check_equality(edge_t{0,0,0,0.0}, edge_t{0,0,0,1.0}, LINE("Differing weights, identical hosts, targets, complementary indices"));
    check_equality(edge_t{0, inversion_constant<true>{}, 0, 0.0}, edge_t{0, inversion_constant<false>{}, 0, 0.0}, LINE("Differing inversion flag"));
  }
}
