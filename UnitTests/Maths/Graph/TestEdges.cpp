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
      test_partial_edge();
      test_weighted_edge_lVal(5.2);
      
      test_embedded_partial_edge();
      
      test_plain_edge();      
      test_weighted_edge();
      
      test_embedded_edge();
      
      test_edge_conversions();
    }
    
    void test_edges::test_partial_edge()
    {
      using namespace maths;
      using namespace data_sharing;

      partial_edge<int, shared, utilities::protective_wrapper<int>> edgeSharedWeight(1, 4);
      check_edge(1, 4, edgeSharedWeight);

      edgeSharedWeight.weight(-1);
      check_edge(1, -1, edgeSharedWeight);

      partial_edge<int, shared, utilities::protective_wrapper<int>> edgeSharedWeight2(5, edgeSharedWeight);
      check_edge(5, -1, edgeSharedWeight2);
      
      edgeSharedWeight2.weight(-3);
      check_edge(1, -3, edgeSharedWeight, "implicit change of shared weight");
      check_edge(5, -3, edgeSharedWeight2, "explicit change of shared weight");
      std::swap(edgeSharedWeight, edgeSharedWeight2);
      check_edge(5, -3, edgeSharedWeight);
      check_edge(1, -3, edgeSharedWeight2);

      partial_edge<int, shared, utilities::protective_wrapper<int>> edgeSharedWeight3(2, 8);
      check_edge(2, 8, edgeSharedWeight3);

      std::swap(edgeSharedWeight, edgeSharedWeight3);
      check_edge(2, 8, edgeSharedWeight);
      check_edge(1, -3, edgeSharedWeight2);
      check_edge(5, -3, edgeSharedWeight3);

      edgeSharedWeight.weight(9);
      check_edge(2, 9, edgeSharedWeight);
      check_edge(1, -3, edgeSharedWeight2);
      check_edge(5, -3, edgeSharedWeight3);

      edgeSharedWeight2.weight(4);
      check_edge(2, 9, edgeSharedWeight);
      check_edge(1, 4, edgeSharedWeight2);
      check_edge(5, 4, edgeSharedWeight3);

      edgeSharedWeight3.weight(-4);
      check_edge(2, 9, edgeSharedWeight);
      check_edge(1, -4, edgeSharedWeight2);
      check_edge(5, -4, edgeSharedWeight3);

      partial_edge<int, shared, utilities::protective_wrapper<int>> edgeSharedWeight4{edgeSharedWeight};
      check_edge(2, 9, edgeSharedWeight);
      check_edge(1, -4, edgeSharedWeight2);
      check_edge(5, -4, edgeSharedWeight3);
      check_edge(2, 9, edgeSharedWeight4);

      check(edgeSharedWeight == edgeSharedWeight4);
      check(edgeSharedWeight2 != edgeSharedWeight4);
      check(edgeSharedWeight.weight() > edgeSharedWeight2.weight());
      check(edgeSharedWeight2.weight() < edgeSharedWeight.weight());
      check(!(edgeSharedWeight2.weight() > edgeSharedWeight3.weight()));
      check(!(edgeSharedWeight2.weight() < edgeSharedWeight3.weight()));

      edgeSharedWeight.weight(10);
      check_edge(2, 10, edgeSharedWeight);
      check_edge(1, -4, edgeSharedWeight2);
      check_edge(5, -4, edgeSharedWeight3);
      check_edge(2, 9, edgeSharedWeight4);

      edgeSharedWeight4 = edgeSharedWeight;
      check_edge(2, 10, edgeSharedWeight);
      check_edge(1, -4, edgeSharedWeight2);
      check_edge(5, -4, edgeSharedWeight3);
      check_edge(2, 10, edgeSharedWeight4);
 
      edgeSharedWeight4.weight(11);
      check_edge(2, 10, edgeSharedWeight);
      check_edge(1, -4, edgeSharedWeight2);
      check_edge(5, -4, edgeSharedWeight3);
      check_edge(2, 11, edgeSharedWeight4);
   
      // Test independent weights
      
      partial_edge<int, independent, utilities::protective_wrapper<int>> edge(2, 7);
      check_edge(2, 7, edge);

      edge.weight(-5);
      check_edge(2, -5, edge);

      partial_edge<int, independent, utilities::protective_wrapper<int>> edge2(5, edge);
      check_edge(5, -5, edge2);

      edge2.weight(4);
      check_edge(2, -5, edge);
      check_edge(5, 4, edge2);

      std::swap(edge, edge2);
      check_edge(2, -5, edge2);
      check_edge(5, 4, edge);

      struct null_weight{};
      using edge_t = partial_edge<null_weight, independent, utilities::protective_wrapper<null_weight>>;
      check_equality(sizeof(std::size_t), sizeof(edge_t), "Plain edge should be size of a single index");

      using compact_edge_t = partial_edge<null_weight, independent, utilities::protective_wrapper<null_weight>, char>;
      check_equality(sizeof(char), sizeof(compact_edge_t), "Compact plain edge should be size of a char");
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

      check_equality<size_t>(2, e1.host_node());
      check_equality<size_t>(3, e1.target_node());
      check(!e1.inverted());

      check_equality<size_t>(4, e2.host_node());
      check_equality<size_t>(6, e2.target_node());

      std::swap(e1, e2);

      check_equality<size_t>(2, e2.host_node());
      check_equality<size_t>(3, e2.target_node());

      check_equality<size_t>(4, e1.host_node());
      check_equality<size_t>(6, e1.target_node());

      e2.target_node(1);
      check_equality<size_t>(2, e2.host_node());
      check_equality<size_t>(1, e2.target_node());

      edge_t e3{4, inverted_constant<false>{}}, e4{5, inverted_constant<true>{}};
      check_equality<size_t>(4, e3.host_node());
      check_equality<size_t>(4, e3.target_node());

      check_equality<size_t>(5, e4.host_node());
      check_equality<size_t>(5, e4.target_node());

      check(!e3.inverted(), LINE(""));
      check(e4.inverted(), LINE(""));
    }

    void test_edges::test_weighted_edge_lVal(const double val)
    {
      using namespace maths;

      edge<double, utilities::protective_wrapper<double>> e1{4, 6, 32.1};
      check_equality<double>(32.1, e1.weight());

      e1.weight(val);
      check_equality<double>(val, e1.weight());
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

        double lval{45.1};
        test_weighted_edge_lVal(lval);
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

    void test_edges::test_embedded_partial_edge()
    {
      using namespace maths;
      using namespace data_sharing;

      using npedge = embedded_partial_edge<double, independent, utilities::protective_wrapper<double>>;
      check_equality(2*sizeof(std::size_t) + sizeof(double), sizeof(npedge), "NP edge holding a double should be size of double plus twice size_t");
      
      constexpr npedge edge1{1, 2, 5.0};
      check_embedded_edge(1, 2, 5.0, edge1);

      constexpr npedge edge2{3, 7, edge1};
      check_embedded_edge(3, 7, 5.0, edge2);

      check(edge1 != edge2);

      auto edge3 = edge2;
      check_embedded_edge(3, 7, 5.0, edge3);
      check(edge2 == edge3);

      edge3.weight(6.5);
      check_embedded_edge(3, 7, 6.5, edge3);
      check(edge3 != edge2);
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

        constexpr edge e3{4, inverted_constant<true>{}, 1, 1.1};
        check_embedded_edge(4, 4, 1, 1.1, e3);
        check(e3.inverted(), LINE(""));

        constexpr edge e4{5, inverted_constant<false>{}, 2, -3.1};
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
      }
    }
     
    void test_edges::test_edge_conversions()
    {
      using namespace maths;
      using namespace data_sharing;
      
      {
        partial_edge<double, shared, utilities::protective_wrapper<double>> partialEdgeShared{5, 1.3};
        auto fullEdge = edge_converter<partial_edge<double, shared, utilities::protective_wrapper<double>>, edge<double, utilities::protective_wrapper<double>>>::convert(partialEdgeShared, 3);

        check_equality<size_t>(3, fullEdge.host_node());
        check_equality<size_t>(5, fullEdge.target_node());
        check_equality<double>(1.3, fullEdge.weight());

        fullEdge.weight(-2.2);
        partialEdgeShared = edge_converter<edge<double, utilities::protective_wrapper<double>>, partial_edge<double, shared, utilities::protective_wrapper<double>>>::convert(fullEdge, 5);

        check_equality<size_t>(3, partialEdgeShared.target_node());
        check_equality<double>(-2.2, partialEdgeShared.weight());

        fullEdge.weight(1.7);
        partialEdgeShared = edge_converter<edge<double, utilities::protective_wrapper<double>>, partial_edge<double, shared, utilities::protective_wrapper<double>>>::convert(fullEdge, 3);
      
        check_equality<size_t>(5, partialEdgeShared.target_node());
        check_equality<double>(1.7, partialEdgeShared.weight());

        fullEdge.weight(9.0);
        partialEdgeShared = edge_converter<edge<double, utilities::protective_wrapper<double>>, partial_edge<double, shared, utilities::protective_wrapper<double>>>::convert(fullEdge, 2);
      
        check_equality<size_t>(2, partialEdgeShared.target_node());
        check_equality<double>(9.0, partialEdgeShared.weight());
      }

      {
        constexpr partial_edge<int, independent, utilities::protective_wrapper<int>> partialEdgeUnshared{0, -5};
        constexpr auto fullEdge = edge_converter<partial_edge<int, independent, utilities::protective_wrapper<int>>, edge<int, utilities::protective_wrapper<int>>>::convert(partialEdgeUnshared, 9);

        check_equality<size_t>(9, fullEdge.host_node());
        check_equality<size_t>(0, fullEdge.target_node());
        check_equality<int>(-5, fullEdge.weight());

        constexpr auto partialEdgeU2 = edge_converter<edge<int, utilities::protective_wrapper<int>>, partial_edge<int, independent, utilities::protective_wrapper<int>>>::convert(fullEdge, 0);
        check_equality<size_t>(9, partialEdgeU2.target_node());
        check_equality<int>(-5, partialEdgeU2.weight());

        constexpr auto partialEdgeU3 = edge_converter<edge<int, utilities::protective_wrapper<int>>, partial_edge<int, independent, utilities::protective_wrapper<int>>>::convert(fullEdge, 9);
        check_equality<size_t>(0, partialEdgeU3.target_node());
        check_equality<int>(-5, partialEdgeU3.weight());

        constexpr auto partialEdgeU4 = edge_converter<edge<int, utilities::protective_wrapper<int>>, partial_edge<int, independent, utilities::protective_wrapper<int>>>::convert(fullEdge, 100);
        check_equality<size_t>(100, partialEdgeU4.target_node());
        check_equality<int>(-5, partialEdgeU4.weight());
      }
    }
  }
}
