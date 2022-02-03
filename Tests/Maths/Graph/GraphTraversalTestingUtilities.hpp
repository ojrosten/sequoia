////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "GraphTestingUtilities.hpp"
#include "sequoia/Maths/Graph/DynamicGraphTraversals.hpp"

namespace sequoia::testing
{
  enum class traversal_flavour{ BFS, DFS, PDFS, PRS};

  [[nodiscard]]
  std::string to_string(const traversal_flavour f);

  template<traversal_flavour F>
  struct traversal_constant : std::integral_constant<traversal_flavour, F> {};

  using bfs_type  = traversal_constant<traversal_flavour::BFS>;
  using dfs_type  = traversal_constant<traversal_flavour::DFS>;
  using pdfs_type = traversal_constant<traversal_flavour::PDFS>;
  using prs_type  = traversal_constant<traversal_flavour::PRS>;


  template<traversal_flavour F>
  struct Traverser;

  template<> struct Traverser<traversal_flavour::BFS>
  {
    constexpr static auto flavour{traversal_flavour::BFS};

    template<class G, maths::disconnected_discovery_mode Mode, class... Fn>
    static void traverse(const G& g, const maths::traversal_conditions<Mode> conditions, Fn&&... fn)
    {
      maths::breadth_first_search(g, conditions, std::forward<Fn>(fn)...);
    }

    constexpr static bool uses_forward_iterator() noexcept { return true; }

    static std::string iterator_description() noexcept { return "forward"; }
  };

  template<> struct Traverser<traversal_flavour::DFS>
  {
    constexpr static auto flavour{traversal_flavour::DFS};

    template<class G, maths::disconnected_discovery_mode Mode, class... Fn>
    static void traverse(const G& g, const maths::traversal_conditions<Mode> conditions, Fn&&... fn)
    {
      maths::depth_first_search(g, conditions, std::forward<Fn>(fn)...);
    }

    constexpr static bool uses_forward_iterator() noexcept { return true; }

    static std::string iterator_description() noexcept { return "forward"; }
  };

  template<> struct Traverser<traversal_flavour::PDFS>
  {
    constexpr static auto flavour{traversal_flavour::PDFS};

    template<class G, maths::disconnected_discovery_mode Mode, class... Fn>
    static void traverse(const G& g, const maths::traversal_conditions<Mode> conditions, Fn&&... fn)
    {
      maths::pseudo_depth_first_search(g, conditions, std::forward<Fn>(fn)...);
    }

    constexpr static bool uses_forward_iterator() noexcept { return false; }

    static std::string iterator_description() noexcept { return "reverse"; }
  };

  template<> struct Traverser<traversal_flavour::PRS>
  {
    constexpr static auto flavour{traversal_flavour::PRS};

    template<class G, maths::disconnected_discovery_mode Mode, class... Fn>
    static void traverse(const G& g, const maths::traversal_conditions<Mode> conditions, Fn&&... fn)
    {
      maths::priority_search(g, conditions, std::forward<Fn>(fn)...);
    }

    constexpr static bool uses_forward_iterator() noexcept { return true; }

    static std::string iterator_description() noexcept { return "forward"; }
  };

  class node_tracker
  {
  public:
    void clear() noexcept { m_Order.clear(); }

    void operator()(const std::size_t index) { m_Order.push_back(index); }

    [[nodiscard]]
    auto begin() const noexcept
    {
      return m_Order.begin();
    }

    [[nodiscard]]
    auto end() const noexcept
    {
      return m_Order.end();
    }
  private:
    std::vector<std::size_t> m_Order;
  };

  template<maths::network G, traversal_flavour Flavour>
  class edge_tracker
  {
  public:
    using result_type = std::vector<std::pair<std::size_t, std::size_t>>;

    edge_tracker(const G& graph) : m_Graph{graph} {}

    void clear() noexcept { m_Order.clear(); }

    template<std::input_or_output_iterator I> void operator()(I iter)
    {
      const auto pos{dist(traversal_constant<Flavour>{}, iter)};
      m_Order.emplace_back(iter.partition_index(), static_cast<std::size_t>(pos));
    }

    [[nodiscard]]
    auto begin() const noexcept
    {
      return m_Order.begin();
    }

    [[nodiscard]]
    auto end() const noexcept
    {
      return m_Order.end();
    }
  private:
    result_type m_Order;
    const G& m_Graph;

    template<std::input_or_output_iterator I> [[nodiscard]] auto dist(pdfs_type, I iter)
    {
      return distance(m_Graph.crbegin_edges(iter.partition_index()), iter);
    }

    template<std::input_or_output_iterator I> [[nodiscard]] auto dist(bfs_type, I iter)
    {
      return distance(m_Graph.cbegin_edges(iter.partition_index()), iter);
    }

    template<std::input_or_output_iterator I> [[nodiscard]] auto dist(dfs_type, I iter)
    {
      return distance(m_Graph.cbegin_edges(iter.partition_index()), iter);
    }
  };

  template<>
  struct value_tester<node_tracker>
  {
    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const node_tracker& tracker, const std::vector<std::size_t>& prediction)
    {
      check(equality, "Visitation Order", logger, tracker.begin(), tracker.end(), prediction.begin(), prediction.end());
    }
  };

  template<maths::network G, traversal_flavour Flavour>
  struct value_tester<edge_tracker<G, Flavour>>
  {
    using type = edge_tracker<G, Flavour>;
    using prediction_type = typename type::result_type;

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& tracker, const prediction_type& prediction)
    {
      check(equality, "Visitation Order", logger, tracker.begin(), tracker.end(), prediction.begin(), prediction.end());
    }
  };
}
