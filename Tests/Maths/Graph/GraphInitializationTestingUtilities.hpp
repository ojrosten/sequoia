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
  template<test_mode Mode>
  class graph_init_extender
  {
  public:
    constexpr static test_mode mode{Mode};

    graph_init_extender() = default;

    template<class G, class... NodeWeights, class E=typename G::edge_init_type, class Self>
    void check_graph(this Self&& self, const reporter& description, const G& graph, std::initializer_list<std::initializer_list<E>> edges, const std::tuple<NodeWeights...>& nodeWeights)
    {
      testing::check(weak_equivalence, self.report_line(description), self.m_Logger, graph, edges, nodeWeights);
    }

    template<class G, class E=typename G::edge_init_typ, class Self>
      requires (!std::is_empty_v<typename G::node_weight_type>)
    void check_graph(this Self&& self, const reporter& description, const G& graph, std::initializer_list<std::initializer_list<E>> edges, std::initializer_list<typename G::node_weight_type> nodeWeights)
    {
      testing::check(weak_equivalence, self.report_line(description), self.m_Logger, graph, edges, nodeWeights);
    }

    template<class G, class E=typename G::edge_init_type, class Self>
      requires std::is_empty_v<typename G::node_weight_type>
    void check_graph(this Self&& self, const reporter& description, const G& graph, std::initializer_list<std::initializer_list<E>> edges)
    {
      testing::check(weak_equivalence, self.report_line(description), self.m_Logger, graph, edges);
    }
  protected:
    ~graph_init_extender() = default;

    graph_init_extender(graph_init_extender&&)            noexcept = default;
    graph_init_extender& operator=(graph_init_extender&&) noexcept = default;
  };

  template<test_mode Mode>
  using basic_graph_init_test = basic_test<Mode, graph_init_extender<Mode>>;

  using graph_init_test                = basic_graph_init_test<test_mode::standard>;
  using graph_init_false_positive_test = basic_graph_init_test<test_mode::false_positive>;
}
