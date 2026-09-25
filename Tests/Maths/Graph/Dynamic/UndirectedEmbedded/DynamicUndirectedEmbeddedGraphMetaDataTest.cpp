////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicUndirectedEmbeddedGraphMetaDataTest.hpp"
#include "DynamicUndirectedEmbeddedGraphMetaDataTestingUtilities.hpp"

#include <complex>

namespace sequoia::testing
{
  namespace
  {
    template<class Graph, class... Args>
    concept insert_joinable_by_iterators
      = requires(Graph g, Args... args) {
          g.insert_join(g.cbegin_edges(0), g.cbegin_edges(0), args...);
        };

    template<class Graph, class... Args>
    concept insert_joinable_by_index
      = requires(Graph g, typename Graph::edge_index_type pos, Args... args) {
          g.insert_join(g.cbegin_edges(0), pos, args...);
        };
  }

  [[nodiscard]]
  std::filesystem::path dynamic_undirected_embedded_graph_meta_data_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void dynamic_undirected_embedded_graph_meta_data_test::run_tests()
  {
    test_insert_join_constraints();

    using namespace maths;
    dynamic_undirected_embedded_graph_meta_data_operations<null_weight, null_weight, float, independent_bucketed_edge_storage_config, node_storage<null_weight>>::execute_operations(*this);
    dynamic_undirected_embedded_graph_meta_data_operations<null_weight, null_weight, std::complex<float>, independent_bucketed_edge_storage_config, node_storage<null_weight>>::execute_operations(*this);
  }

  void dynamic_undirected_embedded_graph_meta_data_test::test_insert_join_constraints()
  {
    using namespace maths;

    using graph_without_meta_data_t = embedded_graph<null_weight, null_weight>;
    using graph_with_meta_data_t    = embedded_graph<null_weight, null_weight, float>;
    using empty_meta_data_t         = graph_without_meta_data_t::edge_meta_data_type;
    using meta_data_t               = graph_with_meta_data_t::edge_meta_data_type;

    STATIC_CHECK( insert_joinable_by_iterators<graph_without_meta_data_t>);
    STATIC_CHECK(!insert_joinable_by_iterators<graph_without_meta_data_t, empty_meta_data_t, empty_meta_data_t>);
    STATIC_CHECK( insert_joinable_by_iterators<graph_with_meta_data_t, meta_data_t, meta_data_t>);
    STATIC_CHECK(!insert_joinable_by_iterators<graph_with_meta_data_t>);

    STATIC_CHECK( insert_joinable_by_index<graph_without_meta_data_t>);
    STATIC_CHECK(!insert_joinable_by_index<graph_without_meta_data_t, empty_meta_data_t, empty_meta_data_t>);
    STATIC_CHECK( insert_joinable_by_index<graph_with_meta_data_t, meta_data_t, meta_data_t>);
    STATIC_CHECK(!insert_joinable_by_index<graph_with_meta_data_t>);
  }
}