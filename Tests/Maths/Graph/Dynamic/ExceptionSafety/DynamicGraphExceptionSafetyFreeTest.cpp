////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphExceptionSafetyFreeTest.hpp"
#include "Maths/Graph/GraphTestingUtilities.hpp"
#include "Maths/Graph/Dynamic/DynamicGraphTestingUtilities.hpp"

#include "sequoia/Core/Meta/TypeName.hpp"
#include "sequoia/Maths/Graph/DynamicGraph.hpp"

#include <format>
#include <optional>
#include <ranges>
#include <utility>
#include <vector>

namespace sequoia::testing
{
  namespace
  {
    struct injected_failure : std::runtime_error
    {
      using std::runtime_error::runtime_error;
    };

    /** \brief An RAII wrapper to count the fallible steps taken during its lifetime.

        A fallible step is a call of `take_fallible_step`. Steps are numbered from zero.
        If `failingStep` holds a value, the step with that number throws `injected_failure`.
        No step fails outside the lifetime of a monitor.
     */
    class [[nodiscard]] fallible_step_monitor
    {
    public:
      explicit fallible_step_monitor(std::optional<std::size_t> failingStep) noexcept
      {
        current_monitoring() = monitoring{.failing_step{failingStep}};
      }

      ~fallible_step_monitor() { current_monitoring().reset(); }

      fallible_step_monitor(const fallible_step_monitor&)            = delete;
      fallible_step_monitor& operator=(const fallible_step_monitor&) = delete;

      [[nodiscard]]
      std::size_t steps_taken() const noexcept { return current_monitoring()->steps_taken; }

      static void take_fallible_step()
      {
        if(auto& current{current_monitoring()})
        {
          const auto step{current->steps_taken++};
          if(step == current->failing_step)
          {
            current->failing_step.reset();
            throw injected_failure{std::format("Injected failure at fallible step {}", step)};
          }
        }
      }
    private:
      struct monitoring
      {
        std::optional<std::size_t> failing_step;
        std::size_t steps_taken{};
      };

      [[nodiscard]]
      static std::optional<monitoring>& current_monitoring() noexcept
      {
        static std::optional<monitoring> current{};
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

      fallible_copy copy{};

      void increment() { ++value; }

      [[nodiscard]]
      friend auto operator<=>(const fallible_weight&, const fallible_weight&) = default;

      template<class Stream>
      friend Stream& operator<<(Stream& s, const fallible_weight& w)
      {
        s << w.value;
        return s;
      }
    };

    [[nodiscard]]
    int set_value_to_seven_fallibly(fallible_weight& w)
    {
      const int previous{std::exchange(w.value, 7)};
      fallible_step_monitor::take_fallible_step();
      return previous;
    }

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
      using storage_type
        = data_structures::bucketed_sequence<T, std::vector<std::vector<T>, fallible_allocator<std::vector<T>>>>;

      constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::agnostic};
    };
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
    test_shared_weight_edge_mutations<shared_weight_bucketed_edge_storage_config>();
    test_shared_weight_edge_mutations<shared_weight_contiguous_edge_storage_config>();
    test_node_insertion();
  }

  template<class EdgeStorageConfig>
  void dynamic_graph_exception_safety_free_test::test_undirected_edge_mutations()
  {
    using namespace maths;
    using graph_t     = undirected_graph<fallible_weight, null_weight, null_meta_data, EdgeStorageConfig>;
    using edge_init_t = graph_t::edge_init_type;

    STATIC_CHECK(!graph_impl::has_shared_weight_v<typename graph_t::edge_type>);

    const auto context{
      std::format("Undirected graph with {}", meta::tidy_type_name(meta::type_name<EdgeStorageConfig>()))
    };

    // No mutation reaches edge_5 or loop_5: they are inputs
    enum state : std::size_t { edge_5, edge_6, edge_7, loop_5, loop_7, edges_5_7, edges_5_5, edges_7_7 };

    transition_graph_type<graph_t> trg{};
    trg.add_node(graph_t{{edge_init_t{1, 5}}, {edge_init_t{0, 5}}});
    trg.add_node(graph_t{{edge_init_t{1, 6}}, {edge_init_t{0, 6}}});
    trg.add_node(graph_t{{edge_init_t{1, 7}}, {edge_init_t{0, 7}}});
    trg.add_node(graph_t{{edge_init_t{0, 5}, edge_init_t{0, 5}}});
    trg.add_node(graph_t{{edge_init_t{0, 7}, edge_init_t{0, 7}}});
    trg.add_node(graph_t{
      {edge_init_t{1, 5}, edge_init_t{1, 7}},
      {edge_init_t{0, 5}, edge_init_t{0, 7}}
    });
    trg.add_node(graph_t{
      {edge_init_t{1, 5}, edge_init_t{1, 5}},
      {edge_init_t{0, 5}, edge_init_t{0, 5}}
    });
    trg.add_node(graph_t{
      {edge_init_t{1, 7}, edge_init_t{1, 7}},
      {edge_init_t{0, 7}, edge_init_t{0, 7}}
    });

    add_fallible_transition<graph_t>(
      trg, context, edge_5, edge_7, "Set edge weight", 1,
      [](graph_t& g) { g.set_edge_weight(g.cbegin_edges(0), 7); }
    );

    add_fallible_transition<graph_t>(
      trg, context, edge_5, edge_7, "Mutate edge weight", 3,
      [](graph_t& g) { g.mutate_edge_weight(g.cbegin_edges(0), set_value_to_seven_fallibly); }
    );

    add_fallible_transition<graph_t>(
      trg, context, edge_5, edge_6, "Mutate edge weight by a member function", 2,
      [](graph_t& g) { g.mutate_edge_weight(g.cbegin_edges(0), &fallible_weight::increment); }
    );

    add_fallible_transition<graph_t>(
      trg, context, edge_5, edges_5_7, "Join", 1,
      [](graph_t& g) { g.join(0, 1, 7); }
    );

    add_fallible_transition<graph_t>(
      trg, context, edges_5_7, edges_5_5, "Set the weight of the second of two parallel edges", 1,
      [](graph_t& g) { g.set_edge_weight(g.cbegin_edges(0) + 1, 5); }
    );

    add_fallible_transition<graph_t>(
      trg, context, edges_5_7, edges_7_7, "Mutate the weight of an edge parallel to one already of the new weight", 3,
      [](graph_t& g) { g.mutate_edge_weight(g.cbegin_edges(0), set_value_to_seven_fallibly); }
    );

    add_fallible_transition<graph_t>(
      trg, context, loop_5, loop_7, "Set loop weight", 1,
      [](graph_t& g) { g.set_edge_weight(g.cbegin_edges(0), 7); }
    );

    add_fallible_transition<graph_t>(
      trg, context, loop_5, loop_7, "Mutate loop weight", 3,
      [](graph_t& g) { g.mutate_edge_weight(g.cbegin_edges(0), set_value_to_seven_fallibly); }
    );

    check_transitions<graph_t>(context, trg);
  }

  template<class EdgeStorageConfig>
  void dynamic_graph_exception_safety_free_test::test_embedded_edge_mutations()
  {
    using namespace maths;
    using graph_t     = embedded_graph<fallible_weight, null_weight, null_meta_data, EdgeStorageConfig>;
    using edge_init_t = graph_t::edge_init_type;

    STATIC_CHECK(!graph_impl::has_shared_weight_v<typename graph_t::edge_type>);

    const auto context{
      std::format("Embedded graph with {}", meta::tidy_type_name(meta::type_name<EdgeStorageConfig>()))
    };

    // No mutation reaches edges_5_6 or loop_4_edge_5: they are inputs
    enum state : std::size_t {
      edges_5_6,
      edges_7_6,
      edges_5_6_7,
      edges_9_5_6,
      loop_9_edges_5_6,
      loop_4_edge_5,
      loop_7_edge_5,
      edge_9_loop_4_edge_5,
      edge_9_loop_7_edge_5,
      edges_9_8_5_6,
      edges_6_6
    };

    transition_graph_type<graph_t> trg{};
    trg.add_node(graph_t{
      {edge_init_t{1, 0, 5}, edge_init_t{1, 1, 6}},
      {edge_init_t{0, 0, 5}, edge_init_t{0, 1, 6}}
    });

    trg.add_node(graph_t{
      {edge_init_t{1, 0, 7}, edge_init_t{1, 1, 6}},
      {edge_init_t{0, 0, 7}, edge_init_t{0, 1, 6}}
    });

    trg.add_node(graph_t{
      {edge_init_t{1, 0, 5}, edge_init_t{1, 1, 6}, edge_init_t{1, 2, 7}},
      {edge_init_t{0, 0, 5}, edge_init_t{0, 1, 6}, edge_init_t{0, 2, 7}}
    });

    trg.add_node(graph_t{
      {edge_init_t{1, 0, 9}, edge_init_t{1, 1, 5}, edge_init_t{1, 2, 6}},
      {edge_init_t{0, 0, 9}, edge_init_t{0, 1, 5}, edge_init_t{0, 2, 6}}
    });

    trg.add_node(graph_t{
      {edge_init_t{0, 1, 9}, edge_init_t{0, 0, 9}, edge_init_t{1, 0, 5}, edge_init_t{1, 1, 6}},
      {edge_init_t{0, 2, 5}, edge_init_t{0, 3, 6}}
    });

    trg.add_node(graph_t{
      {edge_init_t{0, 1, 4}, edge_init_t{0, 0, 4}, edge_init_t{1, 0, 5}},
      {edge_init_t{0, 2, 5}}
    });

    trg.add_node(graph_t{
      {edge_init_t{0, 1, 7}, edge_init_t{0, 0, 7}, edge_init_t{1, 0, 5}},
      {edge_init_t{0, 2, 5}}
    });

    trg.add_node(graph_t{
      {edge_init_t{1, 0, 9}, edge_init_t{0, 2, 4}, edge_init_t{0, 1, 4}, edge_init_t{1, 1, 5}},
      {edge_init_t{0, 0, 9}, edge_init_t{0, 3, 5}}
    });

    trg.add_node(graph_t{
      {edge_init_t{1, 0, 9}, edge_init_t{0, 2, 7}, edge_init_t{0, 1, 7}, edge_init_t{1, 1, 5}},
      {edge_init_t{0, 0, 9}, edge_init_t{0, 3, 5}}
    });

    trg.add_node(graph_t{
      {edge_init_t{1, 0, 9}, edge_init_t{1, 3, 8}, edge_init_t{1, 1, 5}, edge_init_t{1, 2, 6}},
      {edge_init_t{0, 0, 9}, edge_init_t{0, 2, 5}, edge_init_t{0, 3, 6}, edge_init_t{0, 1, 8}}
    });

    trg.add_node(graph_t{
      {edge_init_t{1, 0, 6}, edge_init_t{1, 1, 6}},
      {edge_init_t{0, 0, 6}, edge_init_t{0, 1, 6}}
    });

    add_fallible_transition<graph_t>(
      trg, context, edges_5_6, edges_7_6, "Set edge weight", 1,
      [](graph_t& g) { g.set_edge_weight(g.cbegin_edges(0), 7); }
    );

    add_fallible_transition<graph_t>(
      trg, context, edges_5_6, edges_7_6, "Mutate edge weight", 3,
      [](graph_t& g) { g.mutate_edge_weight(g.cbegin_edges(0), set_value_to_seven_fallibly); }
    );

    add_fallible_transition<graph_t>(
      trg, context, edges_5_6, edges_6_6, "Mutate edge weight by a member function", 2,
      [](graph_t& g) { g.mutate_edge_weight(g.cbegin_edges(0), &fallible_weight::increment); }
    );

    add_fallible_transition<graph_t>(
      trg, context, edges_5_6, edges_5_6_7, "Join", 1,
      [](graph_t& g) { g.join(0, 1, 7); }
    );

    add_fallible_transition<graph_t>(
      trg, context, edges_5_6, edges_9_5_6, "Insert join ahead of edges whose partners are on another node", 1,
      [](graph_t& g) { g.insert_join(g.cbegin_edges(0), g.cbegin_edges(1), 9); }
    );

    add_fallible_transition<graph_t>(
      trg, context, edges_5_6, loop_9_edges_5_6, "Insert loop ahead of edges whose partners are on another node", 1,
      [](graph_t& g) { g.insert_join(g.cbegin_edges(0), 0, 9); }
    );

    add_fallible_transition<graph_t>(
      trg, context, loop_4_edge_5, loop_7_edge_5, "Set loop weight", 1,
      [](graph_t& g) { g.set_edge_weight(g.cbegin_edges(0), 7); }
    );

    add_fallible_transition<graph_t>(
      trg, context, loop_4_edge_5, loop_7_edge_5, "Mutate loop weight", 3,
      [](graph_t& g) { g.mutate_edge_weight(g.cbegin_edges(0), set_value_to_seven_fallibly); }
    );

    add_fallible_transition<graph_t>(
      trg, context, loop_4_edge_5, edge_9_loop_4_edge_5, "Insert join ahead of a loop", 1,
      [](graph_t& g) { g.insert_join(g.cbegin_edges(0), g.cbegin_edges(1), 9); }
    );

    add_fallible_transition<graph_t>(
      trg, context, edge_9_loop_4_edge_5, edge_9_loop_7_edge_5, "Set the weight of a loop behind an edge", 1,
      [](graph_t& g) { g.set_edge_weight(g.cbegin_edges(0) + 1, 7); }
    );

    add_fallible_transition<graph_t>(
      trg, context, edges_9_5_6, edges_9_8_5_6, "Insert join mid-node on one side and at the end on the other", 1,
      [](graph_t& g) { g.insert_join(g.cbegin_edges(0) + 1, g.cbegin_edges(1) + 3, 8); }
    );

    check_transitions<graph_t>(context, trg);
  }

  template<class EdgeStorageConfig>
  void dynamic_graph_exception_safety_free_test::test_shared_weight_edge_mutations()
  {
    using namespace maths;
    using graph_t     = undirected_graph<fallible_weight, null_weight, null_meta_data, EdgeStorageConfig>;
    using edge_init_t = graph_t::edge_init_type;

    STATIC_CHECK(graph_impl::has_shared_weight_v<typename graph_t::edge_type>);

    const auto context{
      std::format("Undirected graph with {}", meta::tidy_type_name(meta::type_name<EdgeStorageConfig>()))
    };

    // No mutation reaches edge_5 or loop_5: they are inputs
    enum state : std::size_t { edge_5, edge_6, edge_7, loop_5, loop_7, edges_5_7 };

    transition_graph_type<graph_t> trg{};
    trg.add_node(graph_t{{edge_init_t{1, 5}}, {edge_init_t{0, 5}}});
    trg.add_node(graph_t{{edge_init_t{1, 6}}, {edge_init_t{0, 6}}});
    trg.add_node(graph_t{{edge_init_t{1, 7}}, {edge_init_t{0, 7}}});
    trg.add_node(graph_t{{edge_init_t{0, 5}, edge_init_t{0, 5}}});
    trg.add_node(graph_t{{edge_init_t{0, 7}, edge_init_t{0, 7}}});
    trg.add_node(graph_t{
      {edge_init_t{1, 5}, edge_init_t{1, 7}},
      {edge_init_t{0, 5}, edge_init_t{0, 7}}
    });

    // A weight built in place from an int takes no fallible step, so the transitions copy this one
    const fallible_weight seven{7};

    add_fallible_transition<graph_t>(
      trg, context, edge_5, edge_7, "Set edge weight", 1,
      [seven](graph_t& g) { g.set_edge_weight(g.cbegin_edges(0), seven); }
    );

    // With shared weights, `mutate_edge_weight` applies the mutation to the weight in place,
    // so the mutation here takes no fallible step
    add_fallible_transition<graph_t>(
      trg, context, edge_5, edge_6, "Mutate edge weight by a member function", 0,
      [](graph_t& g) { g.mutate_edge_weight(g.cbegin_edges(0), &fallible_weight::increment); }
    );

    add_fallible_transition<graph_t>(
      trg, context, edge_5, edges_5_7, "Join", 1,
      [seven](graph_t& g) { g.join(0, 1, seven); }
    );

    add_fallible_transition<graph_t>(
      trg, context, loop_5, loop_7, "Set loop weight", 1,
      [seven](graph_t& g) { g.set_edge_weight(g.cbegin_edges(0), seven); }
    );

    check_transitions<graph_t>(context, trg);
  }

  void dynamic_graph_exception_safety_free_test::test_node_insertion()
  {
    using namespace maths;
    using graph_t      = directed_graph<null_weight, int, fallible_partitions_edge_storage_config>;
    using edge_init_t  = graph_t::edge_init_type;
    using node_weights = std::initializer_list<int>;

    const std::string context{"Directed graph"};

    // No mutation reaches two_nodes: it is an input
    enum state : std::size_t { two_nodes, inserted_ahead, inserted_at_end, inserted_ahead_then_between };

    transition_graph_type<graph_t> trg{};
    trg.add_node(graph_t{{{edge_init_t{1}}, {}}, node_weights{1, 2}});
    trg.add_node(graph_t{{{}, {edge_init_t{2}}, {}}, node_weights{3, 1, 2}});
    trg.add_node(graph_t{{{edge_init_t{1}}, {}, {}}, node_weights{1, 2, 3}});
    trg.add_node(graph_t{{{}, {edge_init_t{3}}, {}, {}}, node_weights{3, 1, 4, 2}});

    add_fallible_transition<graph_t>(
      trg, context, two_nodes, inserted_ahead, "Insert node ahead of the others", 1,
      [](graph_t& g) { g.insert_node(0, 3); }
    );

    add_fallible_transition<graph_t>(
      trg, context, two_nodes, inserted_at_end, "Insert node at the end", 1,
      [](graph_t& g) { g.insert_node(2, 3); }
    );

    trg.join(
      two_nodes,
      two_nodes,
      "Insert node beyond the end",
      [this, context](graph_t g) -> graph_t {
        check_exception_thrown<std::out_of_range>(
          append_lines(
            context,
            std::format("Transition from node {} to {}", std::to_underlying(two_nodes), std::to_underlying(two_nodes)),
            "Insert node beyond the end"
          ),
          [&g]() { g.insert_node(3, 3); }
        );

        return g;
      }
    );

    add_fallible_transition<graph_t>(
      trg, context, inserted_ahead, inserted_ahead_then_between, "Insert node between an edge's ends", 1,
      [](graph_t& g) { g.insert_node(2, 4); }
    );

    check_transitions<graph_t>(context, trg);
  }

  template<class Graph, class Mutation>
  void dynamic_graph_exception_safety_free_test::add_fallible_transition(transition_graph_type<Graph>& trg,
                                                                         std::string_view context,
                                                                         std::size_t host,
                                                                         std::size_t target,
                                                                         std::string description,
                                                                         std::size_t predictedFallibleSteps,
                                                                         Mutation mutation)
  {
    auto locate{
      [context](std::size_t from, std::size_t to, std::string_view what) {
        return append_lines(context, std::format("Transition from node {} to {}", from, to), what);
      }
    };

    const auto stepsMessage{locate(host, target, std::format("{}: fallible steps", description))};
    trg.join(
      host,
      target,
      description,
      [this, stepsMessage, predictedFallibleSteps, mutation](Graph g) -> Graph {
        const fallible_step_monitor monitor{std::nullopt};
        mutation(g);
        check(equality, stepsMessage, monitor.steps_taken(), predictedFallibleSteps);
        return g;
      }
    );

    // A loop for every step the mutation takes, so a step beyond the prediction is checked by a loop of its own
    const auto fallibleStepsTaken{
      [&trg, host, &mutation]() {
        Graph g{trg.cbegin_node_weights()[host]()};
        const fallible_step_monitor monitor{std::nullopt};
        mutation(g);
        return monitor.steps_taken();
      }()
    };

    for(const auto step : std::views::iota(0uz, fallibleStepsTaken))
    {
      const auto failureMessage{locate(host, host, std::format("{}: failure at fallible step {}", description, step))};
      trg.join(
        host,
        host,
        std::format("{}: unchanged by the failure at fallible step {}", description, step),
        [this, failureMessage, step, mutation](Graph g) -> Graph {
          check_exception_thrown<injected_failure>(
            failureMessage,
            [&g, &mutation, step]() {
              const fallible_step_monitor monitor{step};
              mutation(g);
            }
          );

          return g;
        }
      );
    }
  }

  template<class Graph>
  void dynamic_graph_exception_safety_free_test::check_transitions(std::string_view context,
                                                                   const transition_graph_type<Graph>& trg)
  {
    transition_checker<Graph>::check(
      context,
      trg,
      [this](std::string_view message, const Graph& obtained, const Graph& prediction) {
        check(equality, message, obtained, prediction);
      }
    );
  }
}
