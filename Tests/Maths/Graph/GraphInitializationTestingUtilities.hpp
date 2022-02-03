////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "GraphTestingUtilities.hpp"

namespace sequoia::testing
{
  template<test_mode Mode, class... Extenders>
  class graph_init_checker : public checker<Mode, Extenders...>
  {
  public:
    using checker_t = checker<Mode, Extenders...>;

    using checker<Mode, Extenders...>::checker;

    graph_init_checker(const graph_init_checker&) = delete;
    graph_init_checker& operator=(const graph_init_checker&) = delete;

    template<class G, class... NodeWeights, class E=typename G::edge_init_type>
    void check_graph(std::string_view description, const G& graph, std::initializer_list<std::initializer_list<E>> edges, const std::tuple<NodeWeights...>& nodeWeights)
    {
      checker_t::check(weak_equivalence, description, graph, std::move(edges), nodeWeights);
    }

    template
    <
      class G,
      class E=typename G::edge_init_type
    >
      requires (!std::is_empty_v<typename G::node_weight_type>)
    void check_graph(std::string_view description, const G& graph, std::initializer_list<std::initializer_list<E>> edges, std::initializer_list<typename G::node_weight_type> nodeWeights)
    {
      checker_t::check(weak_equivalence, description, graph, edges, nodeWeights);
    }

    template
    <
      class G,
      class E=typename G::edge_init_type
    >
      requires std::is_empty_v<typename G::node_weight_type>
    void check_graph(std::string_view description, const G& graph, std::initializer_list<std::initializer_list<E>> edges)
    {
      checker_t::check(weak_equivalence, description, graph, edges);
    }
  protected:
    graph_init_checker(graph_init_checker&&)            noexcept = default;
    graph_init_checker& operator=(graph_init_checker&&) noexcept = default;

    ~graph_init_checker() = default;
  };

  template<test_mode Mode, class... Extenders>
  class graph_init_basic_test : public basic_test<graph_init_checker<Mode, Extenders...>>
  {
  public:
    using base_t = basic_test<graph_init_checker<Mode, Extenders...>>;

    using base_t::base_t;
  };

  template<test_mode mode>
  using regular_graph_init_test = graph_init_basic_test<mode, regular_extender<mode>>;

  using graph_init_test = regular_graph_init_test<test_mode::standard>;
  using graph_init_false_positive_test = regular_graph_init_test<test_mode::false_positive>;
}
