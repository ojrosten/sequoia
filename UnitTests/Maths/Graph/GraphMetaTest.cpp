////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "GraphMetaTest.hpp"

#include "DynamicGraph.hpp"

namespace sequoia::unit_testing
{  
  void test_graph_meta::run_tests()
  {
    using namespace maths;

    test_method_detectors();
    test_weight_makers();
    test_static_edge_index_generator();
    
    test_undirected<graph_flavour::undirected, partial_edge>();
    test_undirected<graph_flavour::undirected_embedded, embedded_partial_edge>();
    test_directed();
    test_directed_embedded();        
  }

  void test_graph_meta::test_method_detectors()
  {
    using namespace maths::graph_impl;

    using namespace data_structures;

    static_assert(has_reservable_partitions_v<bucketed_storage<int>>);
    static_assert(!has_reservable_partitions_v<contiguous_storage<int>>);
  }

  void test_graph_meta::test_weight_makers()
  {
    using namespace maths::graph_impl;
    using namespace data_sharing;

    {
      weight_maker<unpooled<int>> maker;
      auto proxy = maker.make(2);
      check_equality(LINE(""), proxy.get(), 2);
    }

    {
      weight_maker<unpooled<float>> maker;

      auto proxy = maker.make(3.0f);
      check_equality(LINE(""), proxy.get(), 3.0f);
    }

    {
      weight_maker<data_pool<int>> maker;
      auto proxy = maker.make(2);
      check_equality(LINE(""), proxy.get(), 2);
    }

    {
      weight_maker<data_pool<float>> maker;
      auto proxy = maker.make(3.0f);
      check_equality(LINE(""), proxy.get(), 3.0f);
    }
  }

  void test_graph_meta::test_static_edge_index_generator()
  {
    using namespace maths::graph_impl;

    static_assert(std::is_same_v<unsigned char, typename static_edge_index_type_generator<10, 12, false>::index_type>);
    static_assert(std::is_same_v<unsigned char, typename static_edge_index_type_generator<254, 12, false>::index_type>);
    static_assert(std::is_same_v<unsigned char, typename static_edge_index_type_generator<255, 254, false>::index_type>);
    static_assert(std::is_same_v<unsigned short, typename static_edge_index_type_generator<127, 255, false>::index_type>);
    static_assert(std::is_same_v<unsigned short, typename static_edge_index_type_generator<65535, 255, false>::index_type>);
    static_assert(std::is_same_v<unsigned short, typename static_edge_index_type_generator<65535, 65534, false>::index_type>);
    static_assert(std::is_same_v<std::size_t, typename static_edge_index_type_generator<65535, 65535, false>::index_type>);

    static_assert(std::is_same_v<unsigned char, typename static_edge_index_type_generator<10, 12, true>::index_type>);
    static_assert(std::is_same_v<unsigned char, typename static_edge_index_type_generator<254, 12, true>::index_type>);
    static_assert(std::is_same_v<unsigned short, typename static_edge_index_type_generator<255, 254, true>::index_type>);
    static_assert(std::is_same_v<unsigned short, typename static_edge_index_type_generator<127, 255, true>::index_type>);
    static_assert(std::is_same_v<std::size_t, typename static_edge_index_type_generator<65535, 255, true>::index_type>);
    static_assert(std::is_same_v<std::size_t, typename static_edge_index_type_generator<255, 65535, true>::index_type>);
  }
  
  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class EdgeWeightPooling,
    template<class, template<class> class, class, class> class EdgeType
  >
  void test_graph_meta::test_undirected_unshared()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;
    
    static_assert(!big_proxy<EdgeWeight, EdgeWeightPooling>());
    static_assert(!sharing_traits<GraphFlavour, edge_sharing_preference::independent, EdgeWeight, EdgeWeightPooling>::shared_weight_v);
      
    using gen_t = dynamic_edge_traits<GraphFlavour, EdgeWeight, EdgeWeightPooling, contiguous_edge_storage_traits, std::size_t>;
    using edge_t = typename gen_t::edge_type;
    using proxy = typename EdgeWeightPooling::proxy;
    static_assert(std::is_same_v<edge_t, EdgeType<EdgeWeight, sharing_v_to_type<false>::template policy, proxy, std::size_t>>, "");
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class EdgeWeightPooling,
    template<class, template<class> class, class, class> class EdgeType
  >
  void test_graph_meta::test_undirected_shared()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;
    
    static_assert(big_proxy<EdgeWeight, EdgeWeightPooling>());
    static_assert(sharing_traits<GraphFlavour, edge_sharing_preference::agnostic, EdgeWeight, EdgeWeightPooling>::shared_weight_v);
      
    using gen_t = dynamic_edge_traits<GraphFlavour, EdgeWeight, EdgeWeightPooling, contiguous_edge_storage_traits, std::size_t>;
    using edge_t = typename gen_t::edge_type;
    using proxy = typename EdgeWeightPooling::proxy;
    static_assert(std::is_same_v<edge_t, EdgeType<EdgeWeight, sharing_v_to_type<true>::template policy, proxy, std::size_t>>, "");
  }


  template
  <
    maths::graph_flavour GraphFlavour,
    template<class, template<class> class, class, class> class EdgeType
  >
  void test_graph_meta::test_undirected()
  {    
    test_undirected_unshared<GraphFlavour, int, data_sharing::unpooled<int>, EdgeType>();
    test_undirected_unshared<GraphFlavour, int, data_sharing::data_pool<int>, EdgeType>();
    test_undirected_unshared<GraphFlavour, wrapper<int>, data_sharing::unpooled<wrapper<int>>, EdgeType>();
    test_undirected_unshared<GraphFlavour, wrapper<int>, data_sharing::data_pool<wrapper<int>>, EdgeType>();

    test_undirected_unshared<GraphFlavour, double, data_sharing::unpooled<double>, EdgeType>();
    test_undirected_unshared<GraphFlavour, double, data_sharing::data_pool<double>, EdgeType>();
    test_undirected_unshared<GraphFlavour, wrapper<double>, data_sharing::unpooled<wrapper<double>>, EdgeType>();
    test_undirected_unshared<GraphFlavour, wrapper<double>, data_sharing::data_pool<wrapper<double>>, EdgeType>();

    test_undirected_unshared<GraphFlavour, std::tuple<double, double>, data_sharing::unpooled<std::tuple<double, double>>, EdgeType>();
    test_undirected_unshared<GraphFlavour, std::tuple<double, double>, data_sharing::data_pool<std::tuple<double, double>>, EdgeType>();

    test_undirected_shared<GraphFlavour, std::tuple<double, double, double>, data_sharing::unpooled<std::tuple<double, double, double>>, EdgeType>();
    // For the data pool, the sizeof the proxy is just the size of a shared_ptr
    test_undirected_unshared<GraphFlavour, std::tuple<double, double, double>, data_sharing::data_pool<std::tuple<double, double, double>>, EdgeType>();

    test_undirected_shared<GraphFlavour, std::vector<int>, data_sharing::unpooled<std::vector<int>>, EdgeType>();
    test_undirected_unshared<GraphFlavour, std::vector<int>, data_sharing::data_pool<std::vector<int>>, EdgeType>();
  }

  template
  <
    class EdgeWeight,
    class EdgeWeightPooling
  >
  void test_graph_meta::test_directed_impl()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;
    using namespace data_sharing;

    using gen_t = dynamic_edge_traits<graph_flavour::directed, EdgeWeight, EdgeWeightPooling, contiguous_edge_storage_traits, std::size_t>;
    using edge_t = typename gen_t::edge_type;
    using proxy = typename EdgeWeightPooling::proxy;
    static_assert(std::is_same_v<edge_t, partial_edge<EdgeWeight, sharing_v_to_type<false>::template policy, proxy>>, "");
  }

  template
  <
    class EdgeWeight,
    class EdgeWeightPooling
  >
  void test_graph_meta::test_directed_embedded_impl()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;
    using namespace data_sharing;

    using gen_t = dynamic_edge_traits<graph_flavour::directed_embedded, EdgeWeight, EdgeWeightPooling, contiguous_edge_storage_traits, std::size_t>;
    using edge_t = typename gen_t::edge_type;
    using proxy = typename EdgeWeightPooling::proxy;
    static_assert(std::is_same_v<edge_t, edge<EdgeWeight, proxy>>, "");
  }

  void test_graph_meta::test_directed()
  {
    using namespace maths;

    test_directed_impl<int, data_sharing::unpooled<int>>();
    test_directed_impl<int, data_sharing::data_pool<int>>();

    test_directed_impl<std::tuple<double,double,double>, data_sharing::unpooled<std::tuple<double,double,double>>>();
    test_directed_impl<std::tuple<double,double,double>, data_sharing::data_pool<std::tuple<double,double,double>>>();
  }


  void test_graph_meta::test_directed_embedded()
  {
    using namespace maths;

    test_directed_embedded_impl<int, data_sharing::unpooled<int>>();
    test_directed_embedded_impl<int, data_sharing::data_pool<int>>();

    test_directed_embedded_impl<std::tuple<double,double,double>, data_sharing::unpooled<std::tuple<double,double,double>>>();
    test_directed_embedded_impl<std::tuple<double,double,double>, data_sharing::data_pool<std::tuple<double,double,double>>>();
  }  
}
