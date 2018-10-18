#include "TestEdges.hpp"
#include "Edge.hpp"
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
      test_partial_edge_shared_weight();
      test_partial_edge_independent_weight(); 
      test_embedded_partial_edge();
      
      test_plain_edge();      
      test_weighted_edge();
      
      test_embedded_edge();
    }

    void test_edges::test_plain_partial_edge()
    {
      using namespace maths;
      using namespace data_sharing;

      struct null_weight{};
      using edge_t = partial_edge<null_weight, independent, utilities::protective_wrapper<null_weight>>;
      static_assert(sizeof(std::size_t) == sizeof(edge_t));
      
      using compact_edge_t = partial_edge<null_weight, independent, utilities::protective_wrapper<null_weight>, unsigned char>;
      static_assert(sizeof(unsigned char) == sizeof(compact_edge_t));

      edge_t e1{0};
      check_edge(0, e1, LINE("Construction"));

      e1.target_node(1);
      check_edge(1, e1, LINE("Change target node"));

      edge_t e2{e1};
      check_edge(1, e1, LINE(""));
      check_edge(1, e2, LINE("Copy construction"));

      check(e1 == e2, LINE("Equality"));

      e2.target_node(0);
      check_edge(1, e1, LINE(""));
      check_edge(0, e2, LINE(""));

      check(e1 != e2, LINE("Inequality"));

      e1 = e2;
      check_edge(0, e1, LINE("Assigment"));
      check_edge(0, e2, LINE(""));

      e2 = [](){ return edge_t{3}; }();
      check_edge(3, e2, LINE("Move assignment"));

      auto e3{[](){ return edge_t{4}; }()};
      check_edge(4, e3, LINE("Move construction"));

      std::swap(e3, e2);
      check_edge(3, e3, LINE("Swap"));
      check_edge(4, e2, LINE("Swap"));
    }
    
    void test_edges::test_partial_edge_shared_weight()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = partial_edge<int, shared, utilities::protective_wrapper<int>>;

      edge_t edge{1, 4};
      check_edge(1, 4, edge, LINE("Construction"));

      edge.weight(-1);
      check_edge(1, -1, edge, LINE("Set weight"));

      edge.target_node(2);
      check_edge(2, -1, edge, LINE("Change target node"));

      edge_t edge2{5, edge};
      check_edge(5, -1, edge2, LINE("Construction with shared weight"));

      check(edge != edge2, LINE("Inequality"));

      edge.target_node(1);
      check_edge(1, -1, edge, LINE(""));
      
      edge2.weight(-3);
      check_edge(1, -3, edge, "Implicit change of shared weight");
      check_edge(5, -3, edge2, "Explicit change of shared weight");
      std::swap(edge, edge2);
      check_edge(5, -3, edge, LINE("Swap edges with shared weight"));
      check_edge(1, -3, edge2, LINE("Swap edges with shared weight"));

      edge_t edge3(2, 8);
      check_edge(2, 8, edge3, LINE(""));

      std::swap(edge, edge3);
      check_edge(2, 8, edge, LINE("Swap edge with one of an edge sharing pair"));
      check_edge(1, -3, edge2, LINE("Swap edge with one of an edge sharing pair"));
      check_edge(5, -3, edge3, LINE("Swap edge with one of an edge sharing pair"));

      edge.mutate_weight([](int& a) { ++a; });
      check_edge(2, 9, edge, LINE("Mutate weight of edge which previously but no longer shares its weight"));
      check_edge(1, -3, edge2, LINE(""));
      check_edge(5, -3, edge3, LINE(""));

      edge2.weight(4);
      check_edge(2, 9, edge, LINE(""));
      check_edge(1, 4, edge2, LINE("Set weight which is now shared with a different weight"));
      check_edge(5, 4, edge3, LINE("Set weight which is now shared with a different weight"));

      edge3.mutate_weight([](int& a) { a = -a;});
      check_edge(2, 9, edge, LINE(""));
      check_edge(1, -4, edge2, LINE("Mutate shared weight"));
      check_edge(5, -4, edge3, LINE("Mutate shared weight"));

      edge_t edge4{edge};
      check_edge(2, 9, edge, LINE("Copy construction"));
      check_edge(1, -4, edge2, LINE(""));
      check_edge(5, -4, edge3, LINE(""));
      check_edge(2, 9, edge4);

      check(edge == edge4, LINE("Equality"));

      check(edge.weight() > edge2.weight(), LINE("operator>"));
      check(edge2.weight() < edge.weight(),  LINE("operator<"));

      edge.weight(10);
      check_edge(2, 10, edge, LINE(""));
      check_edge(1, -4, edge2, LINE(""));
      check_edge(5, -4, edge3, LINE(""));
      check_edge(2, 9, edge4, LINE(""));

      edge4 = edge;
      check_edge(2, 10, edge, LINE(""));
      check_edge(1, -4, edge2, LINE(""));
      check_edge(5, -4, edge3, LINE(""));
      check_edge(2, 10, edge4, LINE("Assigment"));
 
      edge4.weight(11);
      check_edge(2, 10, edge, LINE(""));
      check_edge(1, -4, edge2, LINE(""));
      check_edge(5, -4, edge3, LINE(""));
      check_edge(2, 11, edge4, LINE("Weight changed following assigment"));

      edge4 = edge;
      check_edge(2, 10, edge, LINE(""));
      check_edge(1, -4, edge2, LINE(""));
      check_edge(5, -4, edge3, LINE(""));
      check_edge(2, 10, edge4, LINE(""));

      check(edge4 == edge, LINE(""));

      edge4 = [](){ return edge_t{3,6}; }();
      check_edge(3, 6, edge4, LINE("Move assignment"));

      edge2 = edge4;
      check_edge(2, 10, edge, LINE(""));
      check_edge(3, 6, edge2, LINE("Assignment to edge with shared weight"));
      check_edge(5, -4, edge3, LINE(""));
      check_edge(3, 6, edge4, LINE(""));

      edge2.weight(5);
      check_edge(2, 10, edge, LINE(""));
      check_edge(3, 5, edge2, LINE("Setting of weight which was previously shared"));
      check_edge(5, -4, edge3, LINE(""));
      check_edge(3, 6, edge4, LINE(""));
    }

    void test_edges::test_partial_edge_independent_weight()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = partial_edge<int, independent, utilities::protective_wrapper<int>>;
      static_assert(2 * sizeof(std::size_t) == sizeof(edge_t));

      edge_t edge(2, 7);
      check_edge(2, 7, edge, LINE("Construction"));

      edge.target_node(3);
      check_edge(3, 7, edge, LINE("Change target node"));

      edge.weight(-5);
      check_edge(3, -5, edge, LINE("Set weight"));

      edge_t edge2(5, edge);
      check_edge(5, -5, edge2, LINE("Construction with independent weight"));

      check(edge != edge2, LINE(""));

      edge.mutate_weight([](int& a) { a *= 2;} );
      check_edge(3, -10, edge, LINE("Mutate weight"));
      check_edge(5, -5, edge2, LINE(""));

      constexpr int x{4};
      edge2.weight(x);
      check_edge(3, -10, edge, LINE("Set weight from an l-val"));
      check_edge(5, 4, edge2, LINE(""));

      std::swap(edge, edge2);
      check_edge(3, -10, edge2, LINE("Swap"));
      check_edge(5, 4, edge, LINE("Swap"));

      edge = edge2;
      
      check_edge(3, -10, edge, LINE("Assignment"));
      check_edge(3, -10, edge2, LINE(""));
      check(edge == edge2, LINE("Equality"));

      edge2 = [](){ return edge_t{5,7};}();
      check_edge(5, 7, edge2, LINE("Move assigment"));
      check(edge != edge2, LINE("Inequality"));
      
      auto edge3{[](){ return edge_t{0,1};}()};
      check_edge(0, 1, edge3, LINE("Move construction"));
    }

    void test_edges::test_embedded_partial_edge()
    {
      using namespace maths;
      using namespace data_sharing;

      using edge_t = embedded_partial_edge<double, independent, utilities::protective_wrapper<double>>;
      static_assert(2*sizeof(std::size_t) + sizeof(double) == sizeof(edge_t));
      
      constexpr edge_t edge1{1, 2, 5.0};
      check_embedded_edge(1, 2, 5.0, edge1);

      constexpr edge_t edge2{3, 7, edge1};
      check_embedded_edge(3, 7, 5.0, edge2);

      check(edge1 != edge2);

      auto edge3 = edge2;
      check_embedded_edge(3, 7, 5.0, edge3);
      check(edge2 == edge3);

      edge3.weight(6.5);
      check_embedded_edge(3, 7, 6.5, edge3);
      check(edge3 != edge2);
    }


    void test_edges::test_plain_edge()
    {
      using namespace maths;
      struct null_weight{};

      using edge_t = edge<null_weight, utilities::protective_wrapper<null_weight>>;
      check_equality(2*sizeof(std::size_t), sizeof(edge_t), "Full edge with null weight should be twice size of size_t");
      
      edge_t  
        e1(2, 3),
        e2(4, 6);
      
      check_edge(2, 3, false, e1, LINE(""));
      check_edge(4, 6, false, e2, LINE(""));
      check(e1 != e2);

      std::swap(e1, e2);

      check_edge(2, 3, false, e2, LINE(""));
      check_edge(4, 6, false, e1, LINE(""));

      e2.target_node(1);
      check_edge(2, 1, false, e2, LINE(""));

      e2.host_node(3);
      check_edge(3, 1, false, e2, LINE(""));

      e2.target_node(2);
      check_edge(3, 2, false, e2, LINE(""));

      edge_t e3{4, inversion_constant<false>{}}, e4{5, inversion_constant<true>{}};
      check_edge(4, 4, false, e3, LINE(""));
      check_edge(5, 5, true, e4, LINE(""));

      std::swap(e3, e4);
      check_edge(4, 4, false, e4, LINE(""));
      check_edge(5, 5, true, e3, LINE(""));

      e4.host_node(10);
      check_edge(10, 4, false, e4, LINE(""));

      // Changing host / target node for inverted edge implicitly changes host
      e3.target_node(9);
      check_edge(9, 9, true, e3, LINE(""));

      e3.host_node(4);
      check_edge(4, 4, true, e3, LINE(""));

      check(e3 != e4, LINE(""));

      auto e5{e3}, e6{e4};
      check_edge(4, 4, true, e5, LINE("Copy construction for inverted edge"));
      check_edge(10, 4, false , e6, LINE("Copy construction for non-inverted edge"));
    }    

    void test_edges::test_weighted_edge()
    {
      using namespace maths;

      {
        edge<double, utilities::protective_wrapper<double>>
          e1(0, 1, 1.2),
          e2(1, 0, -3.1);

        check_equality<size_t>(0, e1.host_node());
        check_equality<size_t>(1, e1.target_node());
        check_equality<double>(1.2, e1.weight());

        check_equality<size_t>(1, e2.host_node());
        check_equality<size_t>(0, e2.target_node());
        check_equality<double>(-3.1, e2.weight());

        e1.weight(2.3);
        check_equality<double>(2.3, e1.weight());

        check(e1.weight() > e2.weight());
        check(e2.weight() < e1.weight());

        std::swap(e1, e2);

        check_equality<size_t>(0, e2.host_node());
        check_equality<size_t>(1, e2.target_node());
        check_equality<double>(2.3, e2.weight());

        check_equality<size_t>(1, e1.host_node());
        check_equality<size_t>(0, e1.target_node());
        check_equality<double>(-3.1, e1.weight());

        check(e1 != e2);
        check(e1.weight() < e2.weight());
        check(e2.weight() > e1.weight());
        
        edge<double, utilities::protective_wrapper<double>> e3(e2);

        check(e3 == e2);
      }

      {
        using std::complex;

        edge<complex<float>, utilities::protective_wrapper<complex<float>>>
          e1(3, 2, 1.2f),
          e2(4, 5, -1.3f, -1.4f);

        swap(e1, e2);

        check_equality(complex<float>(-1.3f, -1.4f), e1.weight());
        check_equality(complex<float>(1.2f, 0), e2.weight());
      }

      {
        using std::vector;

        edge<vector<int>, utilities::protective_wrapper<vector<int>>>
          e1(0, 0, 5, 1),
          e2(1, 1);

        check_equality(vector<int>{5, 1}, e1.weight());
        check(e2.weight().empty());
        e1.weight(vector<int>{3, 2});

        swap(e1, e2);
        check_equality(vector<int>{3, 2}, e2.weight());
        check(e1.weight().empty());

        check(e1 != e2);

        e1.weight(vector<int>{3, 2});

        check(e1.weight() == e2.weight());
      }
    }

    void test_edges::test_embedded_edge()
    {
      using namespace maths;
      using namespace data_sharing;

      {
        using edge = embedded_edge<double, independent, utilities::protective_wrapper<double>>;
        check_equality(3*sizeof(std::size_t) + sizeof(double), sizeof(edge), "Non-planar Edge holding a double should be size of double plus thrice size_t");

        constexpr edge e{3, 4, 1, 4.2};
        check_embedded_edge(3, 4, 1, 4.2, e);
        check(!e.inverted());

        constexpr edge e2{e};
        check_embedded_edge(3, 4, 1, 4.2, e2);
        check(e == e2);

        constexpr edge e3{4, inversion_constant<true>{}, 1, 1.1};
        check_embedded_edge(4, 4, 1, 1.1, e3);
        check(e3.inverted(), LINE(""));

        constexpr edge e4{5, inversion_constant<false>{}, 2, -3.1};
        check_embedded_edge(5, 5, 2, -3.1, e4);
        check(!e4.inverted());
      }

      {
        using edge = embedded_edge<double, shared, utilities::protective_wrapper<double>>;

        edge e{10, 11, 0, -1.2};
        check_embedded_edge(10, 11, 0, -1.2, e);

        auto e2 = e;
        check_embedded_edge(10, 11, 0, -1.2, e2);
        check(e == e2);

        e.weight(5.2);
        check_embedded_edge(10, 11, 0, 5.2, e);
        check_embedded_edge(10, 11, 0, -1.2, e2);
        check(e != e2);

        e.host_node(9);
        check_embedded_edge(9, 11, 0, 5.2, e);        
      }
    }
  }
}
