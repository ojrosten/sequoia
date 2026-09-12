////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "GraphMetaTest.hpp"

#include "sequoia/Maths/Graph/DynamicGraph.hpp"
#include "sequoia/Maths/Graph/StaticGraph.hpp"

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path test_graph_meta::source_file()
  {
    return std::source_location::current().file_name();
  }

  void test_graph_meta::run_tests()
  {
    test_method_detectors();
    test_static_edge_index_type();

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

  void test_graph_meta::test_static_edge_index_type()
  {
    using namespace maths;
    using namespace maths::graph_impl;

    // Each type's boundary, reached through the order and through the number of edges separately

    STATIC_CHECK((std::is_same_v<unsigned char,  static_edge_index_type<10, 12>>));
    STATIC_CHECK((std::is_same_v<unsigned char,  static_edge_index_type<255, 255>>));
    STATIC_CHECK((std::is_same_v<unsigned short, static_edge_index_type<256, 0>>));
    STATIC_CHECK((std::is_same_v<unsigned short, static_edge_index_type<0, 256>>));
    STATIC_CHECK((std::is_same_v<unsigned short, static_edge_index_type<3, 300>>));
    STATIC_CHECK((std::is_same_v<unsigned short, static_edge_index_type<65535, 65535>>));
    STATIC_CHECK((std::is_same_v<unsigned int,   static_edge_index_type<65536, 0>>));
    STATIC_CHECK((std::is_same_v<unsigned int,   static_edge_index_type<0, 65536>>));
    STATIC_CHECK((std::is_same_v<unsigned int,   static_edge_index_type<4294967295, 4294967295>>));
    STATIC_CHECK((std::is_same_v<std::size_t,    static_edge_index_type<4294967296, 0>>));
    STATIC_CHECK((std::is_same_v<std::size_t,    static_edge_index_type<0, 4294967296>>));

    // Through the configuration, every flavour but `directed` stores two edges for each one declared

    STATIC_CHECK((std::is_same_v<unsigned char,  static_edge_storage_config<graph_flavour::directed, 255, 3>::index_type>));
    STATIC_CHECK((std::is_same_v<unsigned short, static_edge_storage_config<graph_flavour::directed, 256, 3>::index_type>));
    STATIC_CHECK((std::is_same_v<unsigned short, static_edge_storage_config<graph_flavour::directed, 300, 3>::index_type>));

    STATIC_CHECK((std::is_same_v<unsigned char,  static_edge_storage_config<graph_flavour::undirected, 127, 3>::index_type>));
    STATIC_CHECK((std::is_same_v<unsigned short, static_edge_storage_config<graph_flavour::undirected, 128, 3>::index_type>));

    STATIC_CHECK((std::is_same_v<unsigned char,  static_edge_storage_config<graph_flavour::undirected_embedded, 127, 3>::index_type>));
    STATIC_CHECK((std::is_same_v<unsigned short, static_edge_storage_config<graph_flavour::undirected_embedded, 128, 3>::index_type>));
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class EdgeMetaData,
    template<class, class, class> class EdgeType
  >
  void test_graph_meta::test_undirected_unshared()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;

    static_assert(!big_weight<EdgeWeight>());

    using gen_t        = edge_storage_generator<GraphFlavour, EdgeWeight, EdgeMetaData, std::size_t, contiguous_edge_storage_config>;
    using edge_t       = gen_t::edge_type;
    using handler_type = shared_to_handler_t<false, EdgeWeight>;
    static_assert(std::is_same_v<edge_t, EdgeType<handler_type, EdgeMetaData, std::size_t>>);
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class EdgeMetaData,
    template<class, class, class> class EdgeType
  >
  void test_graph_meta::test_undirected_shared()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;

    static_assert(big_weight<EdgeWeight>());

    using gen_t = edge_storage_generator<GraphFlavour, EdgeWeight, EdgeMetaData, std::size_t, contiguous_edge_storage_config>;
    using edge_t       = gen_t::edge_type;
    using handler_type = shared_to_handler_t<true, EdgeWeight>;
    static_assert(std::is_same_v<edge_t, EdgeType<handler_type, EdgeMetaData, std::size_t>>);
  }


  template
  <
    maths::graph_flavour GraphFlavour,
    template<class, class, class> class EdgeType
  >
  void test_graph_meta::test_undirected()
  {
    test_undirected_unshared<GraphFlavour, int, null_meta_data, EdgeType>();
    test_undirected_unshared<GraphFlavour, wrapper<int>, null_meta_data, EdgeType>();

    test_undirected_unshared<GraphFlavour, double, null_meta_data, EdgeType>();
    test_undirected_unshared<GraphFlavour, wrapper<double>, null_meta_data, EdgeType>();

    test_undirected_unshared<GraphFlavour, std::tuple<double, double>, null_meta_data, EdgeType>();

    test_undirected_shared<GraphFlavour, std::tuple<double, double, double>, null_meta_data, EdgeType>();
    test_undirected_shared<GraphFlavour, std::vector<int>, null_meta_data, EdgeType>();
  }

  template<class EdgeWeight>
  void test_graph_meta::test_directed_impl()
  {
    using namespace maths;
    using namespace graph_impl;
    using namespace data_structures;
    using namespace object;

    using gen_t        = edge_storage_generator<graph_flavour::directed, EdgeWeight, null_meta_data, std::size_t, contiguous_edge_storage_config>;
    using edge_t       = gen_t::edge_type;
    using handler_type = shared_to_handler_t<false, EdgeWeight>;
    static_assert(std::is_same_v<edge_t, partial_edge<handler_type, null_meta_data>>);
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
