////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "GraphMetaTest.hpp"

#include "sequoia/Maths/Graph/DynamicGraph.hpp"
#include "sequoia/Maths/Graph/StaticGraph.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path test_graph_meta::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void test_graph_meta::run_tests()
  {
    using namespace maths;

    test_method_detectors();
    test_static_edge_index_generator();

    test_undirected<graph_flavour::undirected, partial_edge>();
    test_undirected<graph_flavour::undirected_embedded, embedded_partial_edge>();
    test_directed();
  }

  void test_graph_meta::test_method_detectors()
  {
    using namespace maths::graph_impl;

    using namespace data_structures;

    static_assert(has_reservable_partitions<bucketed_sequence<int>>);
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
    template<class, class> class EdgeType
  >
  void test_graph_meta::test_undirected_unshared()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;

    static_assert(!big_weight<EdgeWeight>());

    using gen_t = dynamic_edge_traits<GraphFlavour, EdgeWeight, contiguous_edge_storage_traits, std::size_t>;
    using edge_t       = typename gen_t::edge_type;
    using handler_type = shared_to_handler_t<false, EdgeWeight>;
    static_assert(std::is_same_v<edge_t, EdgeType<handler_type, std::size_t>>);
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    template<class, class> class EdgeType
  >
  void test_graph_meta::test_undirected_shared()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;

    static_assert(big_weight<EdgeWeight>());

    using gen_t = dynamic_edge_traits<GraphFlavour, EdgeWeight, contiguous_edge_storage_traits, std::size_t>;
    using edge_t       = typename gen_t::edge_type;
    using handler_type = shared_to_handler_t<true, EdgeWeight>;
    static_assert(std::is_same_v<edge_t, EdgeType<handler_type, std::size_t>>);
  }


  template
  <
    maths::graph_flavour GraphFlavour,
    template<class, class> class EdgeType
  >
  void test_graph_meta::test_undirected()
  {
    test_undirected_unshared<GraphFlavour, int, EdgeType>();
    test_undirected_unshared<GraphFlavour, wrapper<int>, EdgeType>();

    test_undirected_unshared<GraphFlavour, double, EdgeType>();
    test_undirected_unshared<GraphFlavour, wrapper<double>, EdgeType>();

    test_undirected_unshared<GraphFlavour, std::tuple<double, double>, EdgeType>();

    test_undirected_shared<GraphFlavour, std::tuple<double, double, double>, EdgeType>();
    test_undirected_shared<GraphFlavour, std::vector<int>, EdgeType>();
  }

  template<class EdgeWeight>
  void test_graph_meta::test_directed_impl()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;
    using namespace object;

    using gen_t = dynamic_edge_traits<graph_flavour::directed, EdgeWeight, contiguous_edge_storage_traits, std::size_t>;
    using edge_t       = typename gen_t::edge_type;
    using handler_type = shared_to_handler_t<false, EdgeWeight>;
    static_assert(std::is_same_v<edge_t, partial_edge<handler_type>>);
  }

  void test_graph_meta::test_directed()
  {
    using namespace maths;

    test_directed_impl<int>();
    test_directed_impl<int>();

    test_directed_impl<std::tuple<double,double,double>>();
    test_directed_impl<std::tuple<double,double,double>>();
  }
}
