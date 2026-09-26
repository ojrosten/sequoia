////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphExceptionSafetyFreeTest.hpp"
#include "Maths/Graph/GraphTestingUtilities.hpp"

#include "sequoia/Core/Meta/TypeName.hpp"
#include "sequoia/Maths/Graph/DynamicGraph.hpp"

#include <format>
#include <optional>

namespace sequoia::testing
{
  namespace
  {
    struct injected_failure : std::runtime_error
    {
      using std::runtime_error::runtime_error;
    };

    /** \brief Counts the fallible steps taken during its lifetime, and makes the one numbered
               `failingStep`, counting from zero, throw `injected_failure`.

        A fallible step is whatever calls `take_fallible_step`: here, a copy of a
        `fallible_weight` and an allocation by a `fallible_allocator`. Outside the
        lifetime of a monitor, neither can fail.
     */
    class [[nodiscard]] fallible_step_monitor
    {
    public:
      explicit fallible_step_monitor(std::optional<std::size_t> failingStep) noexcept
      {
        progress() = steps{.failing_step{failingStep}};
      }

      ~fallible_step_monitor() { progress().reset(); }

      fallible_step_monitor(const fallible_step_monitor&)            = delete;
      fallible_step_monitor& operator=(const fallible_step_monitor&) = delete;

      [[nodiscard]]
      std::size_t steps_taken() const noexcept { return progress()->taken; }

      static void take_fallible_step()
      {
        if(auto& current{progress()})
        {
          const auto step{current->taken++};
          if(step == current->failing_step)
          {
            current->failing_step.reset();
            throw injected_failure{std::format("Injected failure at fallible step {}", step)};
          }
        }
      }
    private:
      struct steps
      {
        std::optional<std::size_t> failing_step;
        std::size_t taken{};
      };

      [[nodiscard]]
      static std::optional<steps>& progress() noexcept
      {
        thread_local std::optional<steps> current{};
        return current;
      }
    };

    struct fallible_copy
    {
      fallible_copy() = default;

      fallible_copy(const fallible_copy&) { fallible_step_monitor::take_fallible_step(); }

      fallible_copy(fallible_copy&&) noexcept = default;

      fallible_copy& operator=(const fallible_copy&)
      {
        fallible_step_monitor::take_fallible_step();
        return *this;
      }

      fallible_copy& operator=(fallible_copy&&) noexcept = default;

      [[nodiscard]]
      friend constexpr auto operator<=>(const fallible_copy&, const fallible_copy&) noexcept = default;
    };

    struct fallible_weight
    {
      int value{};

      [[no_unique_address]] fallible_copy copy{};

      [[nodiscard]]
      friend auto operator<=>(const fallible_weight&, const fallible_weight&) = default;

      template<class Stream>
      friend Stream& operator<<(Stream& s, const fallible_weight& w)
      {
        s << w.value;
        return s;
      }
    };

    template<class T>
    struct fallible_allocator
    {
      using value_type = T;

      fallible_allocator() = default;

      template<class U>
      constexpr fallible_allocator(const fallible_allocator<U>&) noexcept {}

      [[nodiscard]]
      T* allocate(std::size_t n)
      {
        fallible_step_monitor::take_fallible_step();
        return std::allocator<T>{}.allocate(n);
      }

      void deallocate(T* p, std::size_t n) noexcept { std::allocator<T>{}.deallocate(p, n); }

      [[nodiscard]]
      friend constexpr bool operator==(const fallible_allocator&, const fallible_allocator&) noexcept = default;
    };

    struct fallible_partitions_edge_storage_config
    {
      template<class T>
      using storage_type = data_structures::bucketed_sequence<T, std::vector<std::vector<T>, fallible_allocator<std::vector<T>>>>;

      constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::agnostic};
    };

    template<class Graph>
    inline constexpr bool weights_shared_v{maths::graph_impl::has_shared_weight_v<typename Graph::edge_type>};
  }

  [[nodiscard]]
  std::filesystem::path dynamic_graph_exception_safety_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void dynamic_graph_exception_safety_free_test::run_tests()
  {
    test_undirected_edge_mutations<maths::bucketed_edge_storage_config>();
    test_undirected_edge_mutations<maths::contiguous_edge_storage_config>();
    test_embedded_edge_mutations<maths::bucketed_edge_storage_config>();
    test_embedded_edge_mutations<maths::contiguous_edge_storage_config>();
    test_node_insertion();
  }

  template<class EdgeStorageConfig>
  void dynamic_graph_exception_safety_free_test::test_undirected_edge_mutations()
  {
    using graph_type     = maths::undirected_graph<fallible_weight, maths::null_weight, maths::null_meta_data, EdgeStorageConfig>;
    using edge_init_type = graph_type::edge_init_type;

    STATIC_CHECK(!weights_shared_v<graph_type>);

    const auto describe{
      [](std::string_view operation) { return std::format("{} in an undirected graph with {}", operation, meta::tidy_type_name(meta::type_name<EdgeStorageConfig>())); }
    };

    const graph_type graph{{edge_init_type{1, 5}}, {edge_init_type{0, 5}}};

    check_strong_guarantee(
      describe("Set edge weight"),
      graph,
      graph_type{{edge_init_type{1, 7}}, {edge_init_type{0, 7}}},
      2,
      [](graph_type& g) { g.set_edge_weight(g.cbegin_edges(0), 7); }
    );

    check_strong_guarantee(
      describe("Join"),
      graph,
      graph_type{{edge_init_type{1, 5}, edge_init_type{1, 7}}, {edge_init_type{0, 5}, edge_init_type{0, 7}}},
      1,
      [](graph_type& g) { g.join(0, 1, 7); }
    );
  }

  template<class EdgeStorageConfig>
  void dynamic_graph_exception_safety_free_test::test_embedded_edge_mutations()
  {
    using graph_type     = maths::embedded_graph<fallible_weight, maths::null_weight, maths::null_meta_data, EdgeStorageConfig>;
    using edge_init_type = graph_type::edge_init_type;

    STATIC_CHECK(!weights_shared_v<graph_type>);

    const auto describe{
      [](std::string_view operation) { return std::format("{} in an embedded graph with {}", operation, meta::tidy_type_name(meta::type_name<EdgeStorageConfig>())); }
    };

    const graph_type graph{
      {edge_init_type{1, 0, 5}, edge_init_type{1, 1, 6}},
      {edge_init_type{0, 0, 5}, edge_init_type{0, 1, 6}}
    };

    check_strong_guarantee(
      describe("Set edge weight"),
      graph,
      graph_type{
        {edge_init_type{1, 0, 7}, edge_init_type{1, 1, 6}},
        {edge_init_type{0, 0, 7}, edge_init_type{0, 1, 6}}
      },
      2,
      [](graph_type& g) { g.set_edge_weight(g.cbegin_edges(0), 7); }
    );

    check_strong_guarantee(
      describe("Join"),
      graph,
      graph_type{
        {edge_init_type{1, 0, 5}, edge_init_type{1, 1, 6}, edge_init_type{1, 2, 7}},
        {edge_init_type{0, 0, 5}, edge_init_type{0, 1, 6}, edge_init_type{0, 2, 7}}
      },
      1,
      [](graph_type& g) { g.join(0, 1, 7); }
    );

    check_strong_guarantee(
      describe("Insert join ahead of edges whose partners are on another node"),
      graph,
      graph_type{
        {edge_init_type{1, 0, 9}, edge_init_type{1, 1, 5}, edge_init_type{1, 2, 6}},
        {edge_init_type{0, 0, 9}, edge_init_type{0, 1, 5}, edge_init_type{0, 2, 6}}
      },
      1,
      [](graph_type& g) { g.insert_join(g.cbegin_edges(0), g.cbegin_edges(1), 9); }
    );

    check_strong_guarantee(
      describe("Insert loop ahead of edges whose partners are on another node"),
      graph,
      graph_type{
        {edge_init_type{0, 1, 9}, edge_init_type{0, 0, 9}, edge_init_type{1, 0, 5}, edge_init_type{1, 1, 6}},
        {edge_init_type{0, 2, 5}, edge_init_type{0, 3, 6}}
      },
      1,
      [](graph_type& g) { g.insert_join(g.cbegin_edges(0), 0, 9); }
    );

    const graph_type graphWithLoop{
      {edge_init_type{0, 1, 4}, edge_init_type{0, 0, 4}, edge_init_type{1, 0, 5}},
      {edge_init_type{0, 2, 5}}
    };

    check_strong_guarantee(
      describe("Insert join ahead of a loop"),
      graphWithLoop,
      graph_type{
        {edge_init_type{1, 0, 9}, edge_init_type{0, 2, 4}, edge_init_type{0, 1, 4}, edge_init_type{1, 1, 5}},
        {edge_init_type{0, 0, 9}, edge_init_type{0, 3, 5}}
      },
      1,
      [](graph_type& g) { g.insert_join(g.cbegin_edges(0), g.cbegin_edges(1), 9); }
    );
  }

  void dynamic_graph_exception_safety_free_test::test_node_insertion()
  {
    using graph_type     = maths::directed_graph<maths::null_weight, int, fallible_partitions_edge_storage_config>;
    using edge_init_type = graph_type::edge_init_type;

    const graph_type graph{{{edge_init_type{1}}, {}}, {1, 2}};

    check_strong_guarantee(
      "Insert node ahead of the others in a directed graph",
      graph,
      graph_type{{{}, {edge_init_type{2}}, {}}, {3, 1, 2}},
      1,
      [](graph_type& g) { g.insert_node(0, 3); }
    );

    check_strong_guarantee(
      "Insert node beyond the end of a directed graph",
      graph,
      graph_type{{{edge_init_type{1}}, {}, {}}, {1, 2, 3}},
      1,
      [](graph_type& g) { g.insert_node(5, 3); }
    );
  }

  template<class Graph, class Mutation>
  void dynamic_graph_exception_safety_free_test::check_strong_guarantee(std::string_view description, const Graph& graph, const Graph& prediction, std::size_t numFallibleSteps, Mutation mutation)
  {
    {
      Graph g{graph};
      const fallible_step_monitor monitor{std::nullopt};
      mutation(g);
      check(equality, std::format("{}: fallible steps", description), monitor.steps_taken(), numFallibleSteps);
      check(equality, std::format("{}: completed", description), g, prediction);
    }

    for(std::size_t step{}; step < numFallibleSteps; ++step)
    {
      Graph g{graph};
      check_exception_thrown<injected_failure>(
        std::format("{}: failure at fallible step {}", description, step),
        [&g, &mutation, step]() {
          const fallible_step_monitor monitor{step};
          mutation(g);
        }
      );

      check(equality, std::format("{}: unchanged by the failure at fallible step {}", description, step), g, graph);
    }
  }
}
