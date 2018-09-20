#include "TestGraphMeta.hpp"

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
      weight_maker<unpooled<int>, unpooled<int>> maker;
      auto nproxy = maker.make_node_weight(2);
      check_equality<int>(2, nproxy.get());

      auto eproxy = maker.make_edge_weight(3);
      check_equality<int>(3, eproxy.get());
    }

    {
      weight_maker<unpooled<int>, unpooled<float>> maker;
      auto nproxy = maker.make_node_weight(2);
      check_equality<int>(2, nproxy.get());

      auto eproxy = maker.make_edge_weight(3.0f);
      check_equality<float>(3.0f, eproxy.get());
    }

    {
      weight_maker<unpooled<int>, data_pool<int>> maker;
      auto nproxy = maker.make_node_weight(2);
      check_equality<int>(2, nproxy.get());

      auto eproxy = maker.make_edge_weight(3);
      check_equality<int>(3, eproxy.get());
    }

    {
      weight_maker<data_pool<int>, unpooled<int>> maker;
      auto nproxy = maker.make_node_weight(2);
      check_equality<int>(2, nproxy.get());

      auto eproxy = maker.make_edge_weight(3);
      check_equality<int>(3, eproxy.get());
    }

    {
      weight_maker<data_pool<int>, data_pool<int>> maker;
      auto nproxy = maker.make_node_weight(2);
      check_equality<int>(2, nproxy.get());

      auto eproxy = maker.make_edge_weight(3);
      check_equality<int>(3, eproxy.get());
    }

    {
      weight_maker<data_pool<int>, data_pool<float>> maker;
      auto nproxy = maker.make_node_weight(2);
      check_equality<int>(2, nproxy.get());

      auto eproxy = maker.make_edge_weight(3.0f);
      check_equality<float>(3.0f, eproxy.get());
    }

    {
      weight_maker<unpooled<double>, unpooled<int>> maker;
      auto proxy = maker.make_edge_weight(2);
      check_equality<int>(2, proxy.get());
    }

    {
      weight_maker<data_pool<double>, data_pool<int>> maker;
      auto proxy = maker.make_edge_weight(2);
      check_equality<int>(2, proxy.get());
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
  
  template<maths::graph_flavour GraphFlavour, class EdgeWeight, template<class> class EdgeWeightStorage, template<class, template<class> class, class, class> class EdgeType>
  void test_graph_meta::test_undirected_unshared()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;
    
    static_assert(!big_proxy<EdgeWeight, EdgeWeightStorage>());
    static_assert(!sharing_traits<GraphFlavour, sharing_preference::independent, EdgeWeight, EdgeWeightStorage>::shared_weight_v);
      
    using gen_t = dynamic_edge_traits<GraphFlavour, EdgeWeight, EdgeWeightStorage, contiguous_edge_storage_traits, std::size_t>;
    using edge_t = typename gen_t::edge_type;
    using proxy = typename EdgeWeightStorage<EdgeWeight>::proxy;
    static_assert(std::is_same_v<edge_t, EdgeType<EdgeWeight, sharing_v_to_type<false>::template policy, proxy, std::size_t>>, "");
  }

  template<maths::graph_flavour GraphFlavour, class EdgeWeight, template<class> class EdgeWeightStorage, template<class, template<class> class, class, class> class EdgeType>
  void test_graph_meta::test_undirected_shared()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;
    
    static_assert(big_proxy<EdgeWeight, EdgeWeightStorage>());
    static_assert(sharing_traits<GraphFlavour, sharing_preference::agnostic, EdgeWeight, EdgeWeightStorage>::shared_weight_v);
      
    using gen_t = dynamic_edge_traits<GraphFlavour, EdgeWeight, EdgeWeightStorage, contiguous_edge_storage_traits, std::size_t>;
    using edge_t = typename gen_t::edge_type;
    using proxy = typename EdgeWeightStorage<EdgeWeight>::proxy;
    static_assert(std::is_same_v<edge_t, EdgeType<EdgeWeight, sharing_v_to_type<true>::template policy, proxy, std::size_t>>, "");
  }


  template<
    maths::graph_flavour GraphFlavour,
    template<class, template<class> class, class, class> class EdgeType
  >
  void test_graph_meta::test_undirected()
  {    
    test_undirected_unshared<GraphFlavour, int, data_sharing::unpooled, EdgeType>();
    test_undirected_unshared<GraphFlavour, int, data_sharing::data_pool, EdgeType>();
    test_undirected_unshared<GraphFlavour, wrapper<int>, data_sharing::unpooled, EdgeType>();
    test_undirected_unshared<GraphFlavour, wrapper<int>, data_sharing::data_pool, EdgeType>();

    test_undirected_unshared<GraphFlavour, double, data_sharing::unpooled, EdgeType>();
    test_undirected_unshared<GraphFlavour, double, data_sharing::data_pool, EdgeType>();
    test_undirected_unshared<GraphFlavour, wrapper<double>, data_sharing::unpooled, EdgeType>();
    test_undirected_unshared<GraphFlavour, wrapper<double>, data_sharing::data_pool, EdgeType>();

    test_undirected_unshared<GraphFlavour, std::tuple<double, double>, data_sharing::unpooled, EdgeType>();
    test_undirected_unshared<GraphFlavour, std::tuple<double, double>, data_sharing::data_pool, EdgeType>();

    test_undirected_shared<GraphFlavour, std::tuple<double, double, double>, data_sharing::unpooled, EdgeType>();
    // For the data pool, the sizeof the proxy is just the size of a shared_ptr
    test_undirected_unshared<GraphFlavour, std::tuple<double, double, double>, data_sharing::data_pool, EdgeType>();

    test_undirected_shared<GraphFlavour, std::vector<int>, data_sharing::unpooled, EdgeType>();
    test_undirected_unshared<GraphFlavour, std::vector<int>, data_sharing::data_pool, EdgeType>();
  }

  template<
    class EdgeWeight,
    template<class> class EdgeWeightStorage
  >
  void test_graph_meta::test_directed_impl()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;
    using namespace data_sharing;

    using gen_t = dynamic_edge_traits<graph_flavour::directed, EdgeWeight, EdgeWeightStorage, contiguous_edge_storage_traits, std::size_t>;
    using edge_t = typename gen_t::edge_type;
    using proxy = typename EdgeWeightStorage<EdgeWeight>::proxy;
    static_assert(std::is_same_v<edge_t, partial_edge<EdgeWeight, sharing_v_to_type<false>::template policy, proxy>>, "");
  }

  template<
    class EdgeWeight,
    template<class> class EdgeWeightStorage
  >
  void test_graph_meta::test_directed_embedded_impl()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;
    using namespace data_sharing;

    using gen_t = dynamic_edge_traits<graph_flavour::directed_embedded, EdgeWeight, EdgeWeightStorage, contiguous_edge_storage_traits, std::size_t>;
    using edge_t = typename gen_t::edge_type;
    using proxy = typename EdgeWeightStorage<EdgeWeight>::proxy;
    static_assert(std::is_same_v<edge_t, edge<EdgeWeight, proxy>>, "");
  }

  void test_graph_meta::test_directed()
  {
    using namespace maths;

    test_directed_impl<int, data_sharing::unpooled>();
    test_directed_impl<int, data_sharing::data_pool>();

    test_directed_impl<std::tuple<double,double,double>, data_sharing::unpooled>();
    test_directed_impl<std::tuple<double,double,double>, data_sharing::data_pool>();
  }


  void test_graph_meta::test_directed_embedded()
  {
    using namespace maths;

    test_directed_embedded_impl<int, data_sharing::unpooled>();
    test_directed_embedded_impl<int, data_sharing::data_pool>();

    test_directed_embedded_impl<std::tuple<double,double,double>, data_sharing::unpooled>();
    test_directed_embedded_impl<std::tuple<double,double,double>, data_sharing::data_pool>();
  }  
}
