////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "GraphMetaTest.hpp"

#include "sequoia/Maths/Graph/DynamicGraph.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_graph_meta::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_graph_meta::run_tests()
  {
    using namespace maths;

    test_method_detectors();
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

    static_assert(has_reservable_partitions<bucketed_storage<int>>);
    static_assert(!has_reservable_partitions<partitioned_sequence<int>>);
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
    class EdgeWeightCreator,
    template<class, class> class EdgeType
  >
  void test_graph_meta::test_undirected_unshared()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;

    static_assert(!big_proxy<EdgeWeightCreator>());
    static_assert(!sharing_traits<GraphFlavour, edge_sharing_preference::independent, EdgeWeightCreator>::shared_weight_v);

    using gen_t = dynamic_edge_traits<GraphFlavour, EdgeWeightCreator, contiguous_edge_storage_traits, std::size_t>;
    using edge_t       = typename gen_t::edge_type;
    using proxy        = typename EdgeWeightCreator::proxy;
    using handler_type = shared_to_handler_t<false, proxy>;
    static_assert(std::is_same_v<edge_t, EdgeType<handler_type, std::size_t>>);
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class EdgeWeightCreator,
    template<class, class> class EdgeType
  >
  void test_graph_meta::test_undirected_shared()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;

    static_assert(big_proxy<EdgeWeightCreator>());
    static_assert(sharing_traits<GraphFlavour, edge_sharing_preference::agnostic, EdgeWeightCreator>::shared_weight_v);

    using gen_t = dynamic_edge_traits<GraphFlavour, EdgeWeightCreator, contiguous_edge_storage_traits, std::size_t>;
    using edge_t       = typename gen_t::edge_type;
    using proxy        = typename EdgeWeightCreator::proxy;
    using handler_type = shared_to_handler_t<true, proxy>;
    static_assert(std::is_same_v<edge_t, EdgeType<handler_type, std::size_t>>);
  }


  template
  <
    maths::graph_flavour GraphFlavour,
    template<class, class> class EdgeType
  >
  void test_graph_meta::test_undirected()
  {
    test_undirected_unshared<GraphFlavour, int, ownership::spawner<int>, EdgeType>();
    test_undirected_unshared<GraphFlavour, int, ownership::data_pool<int>, EdgeType>();
    test_undirected_unshared<GraphFlavour, wrapper<int>, ownership::spawner<wrapper<int>>, EdgeType>();
    test_undirected_unshared<GraphFlavour, wrapper<int>, ownership::data_pool<wrapper<int>>, EdgeType>();

    test_undirected_unshared<GraphFlavour, double, ownership::spawner<double>, EdgeType>();
    test_undirected_unshared<GraphFlavour, double, ownership::data_pool<double>, EdgeType>();
    test_undirected_unshared<GraphFlavour, wrapper<double>, ownership::spawner<wrapper<double>>, EdgeType>();
    test_undirected_unshared<GraphFlavour, wrapper<double>, ownership::data_pool<wrapper<double>>, EdgeType>();

    test_undirected_unshared<GraphFlavour, std::tuple<double, double>, ownership::spawner<std::tuple<double, double>>, EdgeType>();
    test_undirected_unshared<GraphFlavour, std::tuple<double, double>, ownership::data_pool<std::tuple<double, double>>, EdgeType>();

    test_undirected_shared<GraphFlavour, std::tuple<double, double, double>, ownership::spawner<std::tuple<double, double, double>>, EdgeType>();
    // For the data pool, the sizeof the proxy is just the size of a shared_ptr
    test_undirected_unshared<GraphFlavour, std::tuple<double, double, double>, ownership::data_pool<std::tuple<double, double, double>>, EdgeType>();

    test_undirected_shared<GraphFlavour, std::vector<int>, ownership::spawner<std::vector<int>>, EdgeType>();
    test_undirected_unshared<GraphFlavour, std::vector<int>, ownership::data_pool<std::vector<int>>, EdgeType>();
  }

  template
  <
    class EdgeWeight,
    class EdgeWeightCreator
  >
  void test_graph_meta::test_directed_impl()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;
    using namespace ownership;

    using gen_t = dynamic_edge_traits<graph_flavour::directed, EdgeWeightCreator, contiguous_edge_storage_traits, std::size_t>;
    using edge_t       = typename gen_t::edge_type;
    using proxy        = edge_weight_proxy_t<EdgeWeightCreator>;
    using handler_type = shared_to_handler_t<false, proxy>;
    static_assert(std::is_same_v<edge_t, partial_edge<handler_type>>);
  }

  template
  <
    class EdgeWeight,
    class EdgeWeightCreator
  >
  void test_graph_meta::test_directed_embedded_impl()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;
    using namespace ownership;

    using gen_t = dynamic_edge_traits<graph_flavour::directed_embedded, EdgeWeightCreator, contiguous_edge_storage_traits, std::size_t>;
    using edge_t       = typename gen_t::edge_type;
    using proxy        = edge_weight_proxy_t<EdgeWeightCreator>;
    using handler_type = shared_to_handler_t<false, proxy>;
    static_assert(std::is_same_v<edge_t, edge<handler_type>>);
  }

  void test_graph_meta::test_directed()
  {
    using namespace maths;

    test_directed_impl<int, ownership::spawner<int>>();
    test_directed_impl<int, ownership::data_pool<int>>();

    test_directed_impl<std::tuple<double,double,double>, ownership::spawner<std::tuple<double,double,double>>>();
    test_directed_impl<std::tuple<double,double,double>, ownership::data_pool<std::tuple<double,double,double>>>();
  }


  void test_graph_meta::test_directed_embedded()
  {
    using namespace maths;

    test_directed_embedded_impl<int, ownership::spawner<int>>();
    test_directed_embedded_impl<int, ownership::data_pool<int>>();

    test_directed_embedded_impl<std::tuple<double,double,double>, ownership::spawner<std::tuple<double,double,double>>>();
    test_directed_embedded_impl<std::tuple<double,double,double>, ownership::data_pool<std::tuple<double,double,double>>>();
  }
}
