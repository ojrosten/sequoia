#include "TestGraphMeta.hpp"

#include <complex>

namespace sequoia::unit_testing
{
  template<maths::graph_flavour GraphFlavour, class EdgeWeight, template<class> class EdgeWeightStorage, template<class, template<class> class, class, class> class EdgeType>
  void test_graph_meta::test_undirected_unshared()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;
    
    static_assert(!big_proxy<EdgeWeight, EdgeWeightStorage>());
    static_assert(!shared_edge_weight_v<false, GraphFlavour, EdgeWeight, EdgeWeightStorage>);
      
    using gen_t = edge_traits<GraphFlavour, EdgeWeight, true, EdgeWeightStorage, contiguous_storage, std::size_t>;
    using edge_t = typename gen_t::edge_type;
    using proxy = typename EdgeWeightStorage<EdgeWeight>::proxy;
    static_assert(std::is_same_v<edge_t, EdgeType<EdgeWeight, sharing<false>::template policy, proxy, std::size_t>>, "");
  }

  template<maths::graph_flavour GraphFlavour, class EdgeWeight, template<class> class EdgeWeightStorage, template<class, template<class> class, class, class> class EdgeType>
  void test_graph_meta::test_undirected_shared()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;
    
    static_assert(big_proxy<EdgeWeight, EdgeWeightStorage>());
    static_assert(shared_edge_weight_v<false, GraphFlavour, EdgeWeight, EdgeWeightStorage>);
      
    using gen_t = edge_traits<GraphFlavour, EdgeWeight, true, EdgeWeightStorage, contiguous_storage, std::size_t>;
    using edge_t = typename gen_t::edge_type;
    using proxy = typename EdgeWeightStorage<EdgeWeight>::proxy;
    static_assert(std::is_same_v<edge_t, EdgeType<EdgeWeight, sharing<true>::template policy, proxy, std::size_t>>, "");
  }


  template<
    maths::graph_flavour GraphFlavour,
    template<class, template<class> class, class, class> class EdgeType
  >
  void test_graph_meta::test_undirected()
  {    
    static_assert(!maths::graph_impl::shared_edge_v<false, GraphFlavour>);
    static_assert(!maths::graph_impl::shared_edge_v<true, GraphFlavour>);

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

    using gen_t = edge_traits<graph_flavour::directed, EdgeWeight, true, EdgeWeightStorage, contiguous_storage, std::size_t>;
    using edge_t = typename gen_t::edge_type;
    using proxy = typename EdgeWeightStorage<EdgeWeight>::proxy;
    static_assert(std::is_same_v<edge_t, partial_edge<EdgeWeight, sharing<false>::template policy, proxy>>, "");
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

    using gen_t = edge_traits<graph_flavour::directed_embedded, EdgeWeight, true, EdgeWeightStorage, contiguous_storage, std::size_t>;
    using edge_t = typename gen_t::edge_type;
    using proxy = typename EdgeWeightStorage<EdgeWeight>::proxy;
    static_assert(std::is_same_v<edge_t, edge<EdgeWeight, proxy>>, "");
  }

  void test_graph_meta::test_directed()
  {
    using namespace maths;
    
    static_assert(!graph_impl::shared_edge_v<false, graph_flavour::directed>);
    static_assert(!graph_impl::shared_edge_v<true, graph_flavour::directed>);

    test_directed_impl<int, data_sharing::unpooled>();
    test_directed_impl<int, data_sharing::data_pool>();

    test_directed_impl<std::tuple<double,double,double>, data_sharing::unpooled>();
    test_directed_impl<std::tuple<double,double,double>, data_sharing::data_pool>();
  }


  void test_graph_meta::test_directed_embedded()
  {
    using namespace maths;
    
    static_assert(graph_impl::shared_edge_v<false, graph_flavour::directed_embedded>);
    static_assert(!graph_impl::shared_edge_v<true, graph_flavour::directed_embedded>);

    test_directed_embedded_impl<int, data_sharing::unpooled>();
    test_directed_embedded_impl<int, data_sharing::data_pool>();

    test_directed_embedded_impl<std::tuple<double,double,double>, data_sharing::unpooled>();
    test_directed_embedded_impl<std::tuple<double,double,double>, data_sharing::data_pool>();
  }
  
  void test_graph_meta::run_tests()
  {
    using namespace maths;
    test_undirected<graph_flavour::undirected, partial_edge>();
    test_undirected<graph_flavour::undirected_embedded, embedded_partial_edge>();
    test_directed();
    test_directed_embedded();
    
    using namespace graph_impl;
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
}
