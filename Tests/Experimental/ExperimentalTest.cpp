////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ExperimentalTest.hpp"

#include "sequoia/Maths/Graph/DynamicGraph.hpp"

namespace graph_proposal
{
  namespace vertices_impl
  {
    template<class G>
    inline constexpr bool has_vertices_adl{ requires(const G& g) { vertices(g); } };

    struct cpo
    {
      template<class G>
      auto operator()(const G& g) const
      {
        if constexpr(has_vertices_adl<G>)
        {
          return vertices(g);
        }
        else
        {
          static_assert(sequoia::dependent_false<G>::value);
        }
      }
    };
  }

  inline constexpr vertices_impl::cpo vertices{};

  template<class G>
  using vertex_range_t = decltype(vertices(std::declval<G>()));

  template<class G>
  using vertex_iterator_t = std::ranges::iterator_t<vertex_range_t<G>>; // paper missing ranges::

  namespace vertex_id_impl
  {
    template<class G>
    inline constexpr bool has_vertex_id_adl{ requires(const G& g, vertex_iterator_t<G> ui) { vertex_id(g, ui); } };

    struct cpo
    {
      template<class G>
      auto operator()(const G& g, vertex_iterator_t<G> ui) const
      {
        if constexpr(has_vertex_id_adl<G>)
        {
          return vertex_id(g, ui);
        }
        else
        {
          static_assert(sequoia::dependent_false<G>::value);
        }
      }
    };
  }

  inline constexpr vertex_id_impl::cpo vertex_id{};

  template<class G>
  using vertex_id_t = decltype(vertex_id(std::declval<G>(), std::declval<vertex_iterator_t<G>>()));

  namespace edges_impl
  {
    template<class G>
    inline constexpr bool has_edges_adl{ requires(const G & g, vertex_id_t<G> uid) { edges(g, uid); } };

    struct cpo
    {
      template<class G>
      auto operator()(const G& g, vertex_id_t<G> uid) const
      {
        if constexpr(has_edges_adl<G>)
        {
          return edges(g, uid);
        }
        else
        {
          static_assert(sequoia::dependent_false<G>::value);
        }
      }
    };
  }

  inline constexpr edges_impl::cpo edges{};

  template<class G>
  using vertex_edge_range_t = decltype(edges(std::declval<G>(), std::declval<vertex_id_t<G>>()));

  template<class G>
  using edge_reference_t = std::ranges::range_reference_t<vertex_edge_range_t<G>>; // paper missing ranges::

  namespace target_id_impl
  {
    template<class G>
    inline constexpr bool has_target_id_adl{ requires(const G& g, edge_reference_t<G> uv) { target_id(g, uv); } };

    struct cpo
    {
      template<class G>
      auto operator()(const G& g, edge_reference_t<G> uv) const
      {
        if constexpr(has_target_id_adl<G>)
        {
          return target_id(g, uv);
        }
        else
        {
          static_assert(sequoia::dependent_false<G>::value);
        }
      }
    };
  }

  inline constexpr target_id_impl::cpo target_id{};

  template <class G>
  concept basic_targeted_edge = requires(G&& g, edge_reference_t<G> uv) { target_id(g, uv); }; // -> std::size_t ??
  // Does paper mean to std::forward<G>(g)? 
}

namespace sequoia::maths
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

    constexpr const_graph_dereference_policy(const const_graph_dereference_policy&) = default;
    constexpr const_graph_dereference_policy(const_graph_dereference_policy&&) noexcept = default;

    ~const_graph_dereference_policy() = default;

    constexpr const_graph_dereference_policy& operator=(const const_graph_dereference_policy&) = default;
    constexpr const_graph_dereference_policy& operator=(const_graph_dereference_policy&&) noexcept = default;

  private:
    const graph_type* m_Graph{};
  };


  template<network Connectivity, class Nodes>
  using const_pseudo_iterator = utilities::iterator<typename graph_primitive<Connectivity, Nodes>::edge_index_type, const_graph_dereference_policy<Connectivity, Nodes>>;

  template<network G>
  constexpr bool has_nodes_type{requires { typename G::nodes_type; }};

  // Overload for my graph
  // Problem: for graphs with a null edge weight, I don't define vertices
  template<network G>
    requires has_nodes_type<G>
  [[nodiscard]]
  auto vertices(const G& g)
  {
    using connectivity_t = typename G::connectivity_type;
    using nodes_t = typename G::nodes_type;
    using iter_t = const_pseudo_iterator<connectivity_t, nodes_t>;
    using index_t = typename G::edge_index_type;
    return std::ranges::subrange{iter_t{index_t{}, g}, iter_t{g.order(), g}};
  }

  // Overload for my graph
  template<network G>
    requires has_nodes_type<G>
  [[nodiscard]]
  typename G::edge_index_type vertex_id(const G&, graph_proposal::vertex_iterator_t<G> ui)
  {
    return ui.base_iterator();
  }


  // Overload for my graph
  template<network G>
    requires has_nodes_type<G>
  [[nodiscard]]
  typename G::const_edges_range edges(const G& g, graph_proposal::vertex_id_t<G> uid) { return g.cedges(uid); }


  // Overload for my graph
  template<network G>
    requires has_nodes_type<G>
  [[nodiscard]]
  typename G::edge_index_type target_id(const G&, typename G::edge_type const& e)
  {
    return e.target_node();
  }
}

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path experimental_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void experimental_test::run_tests()
  {
    using graph_t     = maths::directed_graph<int, maths::null_weight>;
    using node_iter_t = maths::const_pseudo_iterator<graph_t::connectivity_type, graph_t::nodes_type>;

    graph_t g{{{0, 42}}};

    auto v{graph_proposal::vertices(g)};

    auto vid{graph_proposal::vertex_id(g, node_iter_t{})};
    check(equality, report_line(""), vid, std::size_t{});

    static_assert(std::is_same_v<graph_proposal::vertex_id_t<graph_t>, std::size_t>);

    auto es{graph_proposal::edges(g, std::size_t{})};
    auto target{graph_proposal::target_id(g, es.front())};
    check(equality, report_line(""), target, std::size_t{});
    
    static_assert(graph_proposal::basic_targeted_edge<graph_t>);
  }
}
