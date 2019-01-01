#include "TestEdges.hpp"

#include "EdgeTestingUtilities.hpp"

#include "SharingPolicies.hpp"
#include "ProtectiveWrapper.hpp"

#include <complex>
#include <list>
#include <vector>

namespace sequoia
{
  namespace unit_testing
  {
    void test_edges::run_tests()
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

    void test_edges::test_plain_partial_edge()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = partial_edge<null_weight, independent, utilities::protective_wrapper<null_weight>>;
      static_assert(sizeof(std::size_t) == sizeof(edge_t));
      
      using compact_edge_t = partial_edge<null_weight, independent, utilities::protective_wrapper<null_weight>, unsigned char>;
      static_assert(sizeof(unsigned char) == sizeof(compact_edge_t));

      edge_t e1{0};
      check_equality(edge_t{0}, e1, LINE("Construction"));

      e1.target_node(1);
      check_equality(edge_t{1}, e1, LINE("Changing target node"));
  
      edge_t e2{3};
      check_equality(edge_t{3}, e2, LINE(""));
      
      check_regular_semantics(e1, e2, LINE("Standard semantics"));
    }
    
    void test_edges::test_partial_edge_shared_weight()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = partial_edge<int, shared, utilities::protective_wrapper<int>>;

      edge_t edge{1, 4};
      check_equality(edge_t{1, 4}, edge, LINE("Construction"));

      edge.weight(5);
      check_equality(edge_t{1, 5}, edge, LINE("Set weight"));

      edge.mutate_weight([](auto& i){ i -= 6;});
      check_equality(edge_t{1, -1}, edge, LINE("Manipulate weight"));

      edge.target_node(2);
      check_equality(edge_t{2, -1}, edge, LINE("Change target node"));

      edge_t edge1{2,-7};
      check_equality(edge_t{2, -7}, edge1, LINE("Construction"));

      check_regular_semantics(edge, edge1, LINE("Standard Semantics"));      

      edge_t edge2{6, edge};
      check_equality(edge_t{6, -1}, edge2, LINE("Construction with shared weight"));

      edge.target_node(1);
      check_equality(edge_t{1, -1}, edge, LINE("Change target node of edge with shared weight"));
      check_equality(edge_t{6, -1}, edge2, LINE(""));

      edge2.target_node(5);
      check_equality(edge_t{1, -1}, edge, LINE("Change target node of edge with shared weight"));
      check_equality(edge_t{5, -1}, edge2, LINE(""));      
      
      edge2.weight(-3);
      check_equality(edge_t{1, -3}, edge, "Implicit change of shared weight");
      check_equality(edge_t{5, -3}, edge2, "Explicit change of shared weight");

      check_regular_semantics(edge, edge2, LINE("Standard semantics with shared weight"));

      edge_t edge3(2, 8);
      check_equality(edge_t{2, 8}, edge3, LINE(""));

      check_regular_semantics(edge, edge2, LINE("Standard semantics with one having a shared weight"));
 
      std::swap(edge, edge2);
      std::swap(edge, edge3);
      check_equality(edge_t{2, 8}, edge, LINE("Swap edge with one of an edge sharing pair"));
      check_equality(edge_t{1, -3}, edge2, LINE("Swap edge with one of an edge sharing pair"));
      check_equality(edge_t{5, -3}, edge3, LINE("Swap edge with one of an edge sharing pair"));

      edge.mutate_weight([](int& a) { ++a; });
      check_equality(edge_t{2, 9}, edge, LINE("Mutate weight of edge which previously but no longer shares its weight"));
      check_equality(edge_t{1, -3}, edge2, LINE(""));
      check_equality(edge_t{5, -3}, edge3, LINE(""));

      edge2.weight(4);
      check_equality(edge_t{2, 9}, edge, LINE(""));
      check_equality(edge_t{1, 4}, edge2, LINE("Set weight which is now shared with a different weight"));
      check_equality(edge_t{5, 4}, edge3, LINE("Set weight which is now shared with a different weight"));

      edge3.mutate_weight([](int& a) { a = -a;});
      check_equality(edge_t{2, 9}, edge, LINE(""));
      check_equality(edge_t{1, -4}, edge2, LINE("Mutate shared weight"));
      check_equality(edge_t{5, -4}, edge3, LINE("Mutate shared weight"));
    }

    void test_edges::test_partial_edge_indep_weight()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = partial_edge<int, independent, utilities::protective_wrapper<int>>;
      static_assert(2 * sizeof(std::size_t) == sizeof(edge_t));

      edge_t edge{2, 7};
      check_equality(edge_t{2, 7}, edge, LINE("Construction"));

      edge.target_node(3);
      check_equality(edge_t{3, 7}, edge, LINE("Change target node"));

      edge.weight(-5);
      check_equality(edge_t{3, -5}, edge, LINE("Set weight"));

      edge_t edge2(5, edge);
      check_equality(edge_t{5, -5}, edge2, LINE("Construction with independent weight"));
      check_equality(edge_t{3, -5}, edge, LINE(""));

      edge.mutate_weight([](int& a) { a *= 2;} );
      check_equality(edge_t{3, -10}, edge, LINE("Mutate weight"));
      check_equality(edge_t{5, -5}, edge2, LINE(""));

      check_regular_semantics(edge, edge2, LINE("Standard semantics"));
    }

    void test_edges::test_plain_embedded_partial_edge()
    {
      using namespace maths;
      using namespace data_sharing;     
      
      using edge_t = embedded_partial_edge<null_weight, independent, utilities::protective_wrapper<null_weight>>;
      static_assert(2*sizeof(std::size_t) == sizeof(edge_t));

      using compact_edge_t
        = embedded_partial_edge<null_weight, independent, utilities::protective_wrapper<null_weight>, unsigned char>;
      static_assert(2*sizeof(unsigned char) == sizeof(compact_edge_t));

      edge_t e1{0, 4};
      check_equality(edge_t{0, 4}, e1, LINE("Construction"));

      e1.target_node(1);
      check_equality(edge_t{1, 4}, e1, LINE("Change target"));

      e1.complementary_index(5);
      check_equality(edge_t{1, 5}, e1, LINE("Change complementary index"));

      edge_t e2{10, 10};
      check_equality(edge_t{10, 10}, e2, LINE("Construction"));

      check_regular_semantics(e1, e2, LINE("Standard semantics"));
    }
    
    void test_edges::test_embedded_partial_edge_indep_weight()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = embedded_partial_edge<double, independent, utilities::protective_wrapper<double>>;
      static_assert(2*sizeof(std::size_t) + sizeof(double) == sizeof(edge_t));
      
      constexpr edge_t edge1{1, 2, 5.0};
      check_equality(edge_t{1, 2, 5.0}, edge1, LINE("Construction"));

      edge_t edge2{3, 7, edge1};
      check_equality(edge_t{3, 7, 5.0}, edge2, LINE("Construction with independent weight"));

      edge2.target_node(13);
      check_equality(edge_t{13, 7, 5.0}, edge2, LINE("Change target node"));

      edge2.complementary_index(2);
      check_equality(edge_t{13, 2, 5.0}, edge2, LINE("Change complementary index"));

      edge2.weight(5.6);
      check_equality(edge_t{13, 2, 5.6}, edge2, LINE("Change weight"));

      check_regular_semantics(edge1, edge2, LINE("Standard semantics")); 
    }

    void test_edges::test_embedded_partial_edge_shared_weight()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = embedded_partial_edge<double, shared, utilities::protective_wrapper<double>>;
      
      edge_t edge1{1, 2, 5.0};
      check_equality(edge_t{1, 2, 5.0}, edge1, LINE("Construction"));

      edge_t edge2{3, 7, edge1};
      check_equality(edge_t{3, 7, 5.0}, edge2, LINE("Construction with shared weight"));

      edge2.target_node(13);
      check_equality(edge_t{13, 7, 5.0}, edge2, LINE("Change target node"));

      edge2.complementary_index(2);
      check_equality(edge_t{13, 2, 5.0}, edge2, LINE("Change complementary index"));

      edge2.weight(5.6);
      check_equality(edge_t{13, 2, 5.6}, edge2, LINE("Change weight"));
      check_equality(edge_t{1, 2, 5.6}, edge1, LINE("Induced change in shared weight"));

      check_regular_semantics(edge1, edge2, LINE("Standard semantics"));
    }


    void test_edges::test_plain_edge()
    {
      using namespace maths;

      using edge_t = edge<null_weight, utilities::protective_wrapper<null_weight>>;
      static_assert(2*sizeof(std::size_t) == sizeof(edge_t));

      using compact_edge_t = edge<null_weight, utilities::protective_wrapper<null_weight>, unsigned char>;
      static_assert(2 * sizeof(unsigned char) == sizeof(compact_edge_t));
      
      edge_t  
        e1(2, 3),
        e2(4, 6);
      
      check_equality(edge_t{2, 3}, e1, LINE("Construction"));
      check_equality(edge_t{4, 6}, e2, LINE("Construction"));

      e2.target_node(1);
      check_equality(edge_t{4, 1}, e2, LINE("Change target"));

      e2.host_node(3);
      check_equality(edge_t{3, 1}, e2, LINE("Change host"));      
      
      check_regular_semantics(e1, e2, LINE("Standard semantics"));
      
      edge_t e3{4, inversion_constant<false>{}}, e4{5, inversion_constant<true>{}};
      check_equality(edge_t{4, inversion_constant<false>{}}, e3, LINE("Construction"));
      check_equality(edge_t{5, inversion_constant<true>{}}, e4, LINE("Construction inverted edge"));

      check_regular_semantics(e3, e4, LINE("Standard semantics"));

      // Changing host / target node for inverted edge implicitly changes host
      e4.target_node(9);
      check_equality(edge_t{9, inversion_constant<true>{}}, e4, LINE(""));

      e4.host_node(4);
      check_equality(edge_t{4, inversion_constant<true>{}}, e4, LINE(""));
    }    

    void test_edges::test_weighted_edge()
    {
      using namespace maths;

      {
        using edge_t = edge<double, utilities::protective_wrapper<double>>;
        static_assert(sizeof(edge_t) == sizeof(double) + 2*sizeof(std::size_t));

        edge_t
          e1(0, 1, 1.2),
          e2(1, 0, -3.1);

        check_equality(edge_t{0, 1, 1.2}, e1, LINE("Construction"));
        check_equality(edge_t{1, 0, -3.1}, e2, LINE("Construction"));
        
        e1.weight(2.3);
        check_equality(edge_t{0, 1, 2.3}, e1, LINE("Change weight"));

        e1.target_node(10);
        check_equality(edge_t{0, 10, 2.3}, e1, LINE("Change target"));

        e1.host_node(5);
        check_equality(edge_t{5, 10, 2.3}, e1, LINE("Change target"));

        check_regular_semantics(e1, e2, LINE("Standard semantics"));
      }

      {
        using std::complex;
        using edge_t = edge<complex<float>, utilities::protective_wrapper<complex<float>>>;
        static_assert(sizeof(edge_t) == sizeof(std::complex<float>) + 2*sizeof(std::size_t));
      
        edge_t
          e1(3, inversion_constant<true>{}, 1.2f),
          e2(4, inversion_constant<false>{}, -1.3f, -1.4f);

        check_equality(edge_t{3, inversion_constant<true>{}, 1.2f}, e1, LINE("Construction"));
        check_equality(edge_t{4, inversion_constant<false>{}, -1.3f, -1.4f}, e2, LINE("Construction"));

        check_regular_semantics(e1, e2, LINE(""));
      }

      {
        using std::vector;
        using edge_t = edge<vector<int>, utilities::protective_wrapper<vector<int>>>;

        edge_t
          e1(0, 0, 5, 1),
          e2(1, 1);

        check_equality(edge_t{0, 0, 5, 1}, e1, LINE("Construction"));
        check_equality(edge_t{1,1}, e2, LINE("Construction"));

        e1.weight(vector<int>{3, 2});
        check_equality(edge_t{0, 0, vector<int>{3, 2}}, e1, LINE("Change weight"));

        e1.host_node(2);
        check_equality(edge_t{2, 0, vector<int>{3, 2}}, e1, LINE("Change host, no induced change in target"));
        
        check_regular_semantics(e1, e2, LINE("Standard semantics"));
      }
    }

    void test_edges::test_plain_embedded_edge()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = embedded_edge<null_weight, independent, utilities::protective_wrapper<null_weight>>;
      check_equality(3*sizeof(std::size_t), sizeof(edge_t));

      using compact_edge_t = embedded_edge<null_weight, independent, utilities::protective_wrapper<null_weight>, unsigned char>;
      static_assert(3*sizeof(unsigned char) == sizeof(compact_edge_t));

      edge_t e{3, 4, 1};
      check_equality(edge_t{3, 4, 1}, e, LINE("Construction"));

      e.host_node(4);
      check_equality(edge_t{4, 4, 1}, e, LINE("Change host"));

      e.target_node(5);
      check_equality(edge_t{4, 5, 1}, e, LINE("Change target"));

      e.complementary_index(0);
      check_equality(edge_t{4, 5, 0}, e, LINE("Change complementary index"));

      edge_t e1{7, inversion_constant<true>{}, 9};
      check_equality(edge_t{7, inversion_constant<true>{}, 9}, e1, LINE("Construction"));

      e1.host_node(6);
      check_equality(edge_t{6, inversion_constant<true>{}, 9}, e1, LINE("Change host"));

      e1.target_node(8);
      check_equality(edge_t{8, inversion_constant<true>{}, 9}, e1, LINE("Induced change to host"));

      check_regular_semantics(e, e1, LINE("Standard semantics"));

    }
    
    void test_edges::test_embedded_edge_indep_weight()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = embedded_edge<double, independent, utilities::protective_wrapper<double>>;
      check_equality(3*sizeof(std::size_t) + sizeof(double), sizeof(edge_t));

      constexpr edge_t e{3, 4, 1, 4.2};
      check_equality(edge_t{3, 4, 1, 4.2}, e);

      edge_t e2{4, inversion_constant<true>{}, 1, 1.1};
      check_equality(edge_t{4, inversion_constant<true>{}, 1, 1.1}, e2);

      e2.host_node(8);
      check_equality(edge_t{8, inversion_constant<true>{}, 1, 1.1}, e2, LINE("Change host"));

      e2.target_node(7);
      check_equality(edge_t{7, inversion_constant<true>{}, 1, 1.1}, e2, LINE("Induced change host"));

      e2.complementary_index(4);
      check_equality(edge_t{7, inversion_constant<true>{}, 4, 1.1}, e2, LINE("Change complementary index"));

      e2.weight(-2.5);
      check_equality(edge_t{7, inversion_constant<true>{}, 4, -2.5}, e2, LINE("Change weight"));


      check_regular_semantics(e, e2, LINE("Standard semantics"));

    }

    void test_edges::test_embedded_edge_shared_weight()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = embedded_edge<double, shared, utilities::protective_wrapper<double>>;

        edge_t e{10, 11, 0, -1.2};
        check_equality(edge_t{10, 11, 0, -1.2}, e, LINE("Construction"));

        e.host_node(9);
        check_equality(edge_t{9, 11, 0, -1.2}, e, LINE("Change host node"));

        e.weight(5.2);
        check_equality(edge_t{9, 11, 0, 5.2}, e, LINE("Change weight"));

        e.complementary_index(3);
        check_equality(edge_t{9, 11, 3, 5.2}, e, LINE("Change complementary index"));

        e.target_node(0);
        check_equality(edge_t{9, 0, 3, 5.2}, e, LINE("Change target node"));

        edge_t e2{6, inversion_constant<true>{}, 4, 0.0};
        check_equality(edge_t{6, inversion_constant<true>{}, 4, 0.0}, e2, LINE("Construction"));

        e2.host_node(5);
        check_equality(edge_t{5, inversion_constant<true>{}, 4, 0.0}, e2, LINE("Change host node, inducing change in target"));
        check_regular_semantics(e, e2, LINE("Standard semantics"));

        edge_t e3{8, inversion_constant<false>{}, 3, e};
        check_equality(edge_t{8, 8, 3, 5.2}, e3, LINE("Construction"));

        e3.weight(0.0);
        check_equality(edge_t{9, 0, 3, 0.0}, e, LINE("Induced change to shared weight"));
        check_equality(edge_t{8, 8, 3, 0.0}, e3, LINE("Change to shared weight"));

        check_regular_semantics(e, e3, LINE("Standard semantics"));
        check_regular_semantics(e3, e2, LINE("Standard semantics"));
    }
  }
}
