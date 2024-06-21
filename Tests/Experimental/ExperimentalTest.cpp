////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ExperimentalTest.hpp"

#include "sequoia/Maths/Graph/DynamicGraph.hpp"

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    template<network Connectivity, class Nodes>
    class const_graph_dereference_policy
    {
    public:
      using graph_type = graph_primitive<Connectivity, Nodes>;
      using index_type = typename graph_type::edge_index_type;
      using difference_type = std::make_signed_t<index_type>;
      using value_type = index_type;
      using reference = index_type;

      [[nodiscard]]
      constexpr const graph_type& graph() const noexcept
      {
        return *m_Graph;
      }

      [[nodiscard]]
      constexpr static reference get(index_type i) { return i; }
    protected:
      constexpr const_graph_dereference_policy() = default;

      constexpr const_graph_dereference_policy(const graph_type& g) : m_Graph{&g} {}

      constexpr const_graph_dereference_policy(const const_graph_dereference_policy&)     = default;
      constexpr const_graph_dereference_policy(const_graph_dereference_policy&&) noexcept = default;

      ~const_graph_dereference_policy() = default;

      constexpr const_graph_dereference_policy& operator=(const const_graph_dereference_policy&)     = default;
      constexpr const_graph_dereference_policy& operator=(const_graph_dereference_policy&&) noexcept = default;

    private:
      const graph_type* m_Graph{};
    };


    template<network Connectivity, class Nodes>
    using const_pseudo_iterator = utilities::iterator<typename graph_primitive<Connectivity, Nodes>::edge_index_type, const_graph_dereference_policy<Connectivity, Nodes>>;

    template<network G>
    constexpr bool has_nodes_type{ requires { typename G::nodes_type; }};

    // Overload for my graph
    // Problem: for graphs with a null edge weight, I don't define vertices
    template<network G>
      requires has_nodes_type<G>
    [[nodiscard]]
    auto vertices(const G& g)
    {
      using connectivity_t = typename G::connectivity_type;
      using nodes_t        = typename G::nodes_type;
      using iter_t         = const_pseudo_iterator<connectivity_t, nodes_t>;
      using index_t        = typename G::edge_index_type;
      return std::ranges::subrange{iter_t{index_t{}, g}, iter_t{g.order(), g}};
    }

    template<class G>
    using vertex_range_t = std::invoke_result_t<decltype(vertices<G>), G>;

    template<class G>
    using vertex_iterator_t = std::ranges::iterator_t<vertex_range_t<G>>; // paper missing ranges::

    // Overload for my graph
    template<network G>
      requires has_nodes_type<G>
    [[nodiscard]]
    typename G::edge_index_type vertex_id(const G& g, vertex_iterator_t<G> ui)
    {
      return ui.base_iterator();
    }

    template<class G>
    using vertex_id_t = std::invoke_result_t<decltype(vertex_id<G>), G, vertex_iterator_t<G>>;

    // Overload for my graph
    template<network G>
      requires has_nodes_type<G>
    [[nodiscard]]
    typename G::const_edges_range edges(const G& g, vertex_id_t<G> uid) { return g.cedges(uid); }

    template<class G>
    using vertex_edge_range_t = std::invoke_result_t<decltype(edges<G>), G, vertex_id_t<G>>;

    template<class G>
    using edge_reference_t = std::ranges::range_reference_t<vertex_edge_range_t<G>>; // paper missing ranges::

    // Overload for my graph
    template<network G>
      requires has_nodes_type<G>
    [[nodiscard]]
    typename G::edge_index_type target_id(const G&, typename G::edge_type const& e)
    {
      return e.target_node();
    }

    template <class G>
    concept basic_targeted_edge = requires(G&& g, edge_reference_t<G> uv) { target_id(g, uv); }; // -> std::size_t ??
    // Does paper mean to std::forward<G>(g)? 
  }

  [[nodiscard]]
  std::filesystem::path experimental_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void experimental_test::run_tests()
  {
    using graph_t = directed_graph<int, null_weight>;
    graph_t g{{{0, 1}}};

    auto v = vertices(g);

    using node_iter_t = const_pseudo_iterator<graph_t::connectivity_type, graph_t::nodes_type>;
    auto vid = vertex_id(g, node_iter_t{});

    static_assert(std::is_same_v<vertex_id_t<graph_t>, std::size_t>);

    auto es = edges(g, std::size_t{});

    auto target = target_id(g, es.front());

    static_assert(basic_targeted_edge<graph_t>);
  }
}
