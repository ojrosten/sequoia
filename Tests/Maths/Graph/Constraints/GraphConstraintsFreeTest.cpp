////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "GraphConstraintsFreeTest.hpp"
#include "Maths/Graph/GraphTestingUtilities.hpp"

#include "sequoia/Core/Meta/TypeName.hpp"
#include "sequoia/Maths/Graph/DynamicGraph.hpp"
#include "sequoia/Maths/Graph/DynamicTree.hpp"
#include "sequoia/Maths/Graph/HeterogeneousStaticGraph.hpp"
#include "sequoia/Maths/Graph/StaticGraph.hpp"

#include <any>
#include <format>
#include <utility>
#include <vector>

namespace sequoia::testing
{
  namespace
  {
    struct copyable_weight
    {
      int value{};

      void increment() { ++value; }

      [[nodiscard]]
      friend auto operator<=>(const copyable_weight&, const copyable_weight&) = default;

      template<class Stream>
      friend Stream& operator<<(Stream& s, const copyable_weight& w)
      {
        s << w.value;
        return s;
      }
    };

    struct move_only_weight
    {
      int value{};

      move_only_weight() = default;

      explicit move_only_weight(int v) : value{v} {}

      move_only_weight(move_only_weight&&) noexcept = default;

      move_only_weight& operator=(move_only_weight&&) noexcept = default;

      void increment() { ++value; }

      [[nodiscard]]
      friend auto operator<=>(const move_only_weight&, const move_only_weight&) = default;
    };

    struct move_only_meta_data
    {
      int value{};

      move_only_meta_data() = default;

      explicit move_only_meta_data(int v) : value{v} {}

      move_only_meta_data(move_only_meta_data&&) noexcept = default;

      move_only_meta_data& operator=(move_only_meta_data&&) noexcept = default;

      [[nodiscard]]
      friend auto operator<=>(const move_only_meta_data&, const move_only_meta_data&) = default;
    };

    /** \brief A mutation callable only on an rvalue, which `mutate_edge_weight` does not make of its argument. */
    template<class Weight>
    struct rvalue_only_mutation
    {
      void operator()(Weight&) &&;
    };

    struct non_movable
    {
      non_movable() = default;
      non_movable(non_movable&&) = delete;
    };

    struct independent_contiguous_edge_storage_config
    {
      template<class T>
      using storage_type = data_structures::partitioned_sequence<T>;

      constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::independent};
    };

    struct shared_edge_storage_config
    {
      template<class T>
      using storage_type = data_structures::bucketed_sequence<T>;

      constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::shared_weight};
    };

    struct independent_edge_storage_config
    {
      template<class T>
      using storage_type = data_structures::bucketed_sequence<T>;

      constexpr static maths::edge_sharing_preference edge_sharing{maths::edge_sharing_preference::independent};
    };

    template<class Weight>
    using unshared_undirected_graph
      = maths::undirected_graph<Weight, maths::null_weight, maths::null_meta_data, independent_edge_storage_config>;

    template<class Weight>
    using unshared_embedded_graph
      = maths::embedded_graph<Weight, maths::null_weight, maths::null_meta_data, independent_edge_storage_config>;

    using unshared_move_only_graph          = unshared_undirected_graph<move_only_weight>;
    using unshared_move_only_embedded_graph = unshared_embedded_graph<move_only_weight>;
    using unshared_copyable_graph           = unshared_undirected_graph<copyable_weight>;
    using shared_move_only_graph            = maths::undirected_graph<move_only_weight, maths::null_weight>;
    using shared_move_only_embedded_graph   = maths::embedded_graph<move_only_weight, maths::null_weight>;
    using directed_move_only_graph          = maths::directed_graph<move_only_weight, maths::null_weight>;

    template<class Graph, class EdgeIterator = Graph::const_edge_iterator>
    concept edge_weight_settable
      = requires(Graph& g, EdgeIterator citer, typename Graph::edge_weight_type w) {
          g.set_edge_weight(citer, std::move(w));
        };

    template<class Graph>
    concept joinable = requires(Graph& g) { g.join(0, 1); };

    template<class Graph>
    concept insert_joinable
      = requires(Graph& g, typename Graph::const_edge_iterator citer) { g.insert_join(citer, citer); };

    template<class Graph, class Fn>
    concept edge_weight_mutable_by
      = requires(Graph& g, typename Graph::const_edge_iterator citer, Fn fn) {
          g.mutate_edge_weight(citer, std::move(fn));
        };

    template<class Edge, class Fn>
    concept edge_weight_mutable_by_value = requires(Edge& e, Fn fn) { e.mutate_weight(std::move(fn)); };

    template<class Edge, class Fn>
    concept edge_meta_data_mutable_by = requires(Edge& e, Fn fn) { e.mutate_meta_data(std::move(fn)); };

    template<class Graph, class Fn, class EdgeIterator = Graph::const_edge_iterator>
    concept graph_edge_meta_data_mutable_by
      = requires(Graph& g, EdgeIterator citer, Fn fn) { g.mutate_edge_meta_data(citer, std::move(fn)); };

    template<class Graph, class Fn>
    concept node_weight_mutable_by
      = requires(Graph& g, Fn fn) { g.mutate_node_weight(g.cbegin_node_weights(), std::move(fn)); };

    template<class Graph, std::size_t I, class Fn>
    concept node_weight_at_mutable_by = requires(Graph& g, Fn fn) { g.template mutate_node_weight<I>(std::move(fn)); };

    template<class Graph, class T, class Fn>
    concept node_weight_of_type_mutable_by
      = requires(Graph& g, Fn fn) { g.template mutate_node_weight<T>(std::move(fn)); };

    template<class Tree, class Fn>
    concept root_weight_mutable_by
      = requires(maths::basic_tree_adaptor<Tree>& adaptor, Fn fn) { mutate_root_weight(adaptor, std::move(fn)); };

    template<class Graph, class Result, class EdgeIterator = Graph::const_edge_iterator>
    concept edge_weight_mutable_returning
      = requires(Graph& g, EdgeIterator citer, Result(*fn)(typename Graph::edge_weight_type&)) {
          g.mutate_edge_weight(citer, fn);
        };
  }

  [[nodiscard]]
  std::filesystem::path graph_constraints_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void graph_constraints_free_test::run_tests()
  {
    test_copyability();
    test_weight_update_constraints();
    test_join_constraints();
    test_mutator_constraints();
    test_mutation_results<maths::bucketed_edge_storage_config>();
    test_mutation_results<maths::contiguous_edge_storage_config>();
    test_shared_move_only_weights();
    test_move_only_meta_data();
    test_shared_weight_copies();
    test_mutation_by_member_functions();
  }

  void graph_constraints_free_test::test_copyability()
  {
    using namespace maths;

    using unshared_move_only_contiguous_graph
      = undirected_graph<move_only_weight, null_weight, null_meta_data, independent_contiguous_edge_storage_config>;
    using move_only_node_graph = directed_graph<null_weight, move_only_weight>;
    using move_only_tree       = undirected_tree<tree_link_direction::symmetric, move_only_weight, null_weight>;
    using move_only_static     = static_undirected_graph<1, 2, move_only_weight, null_weight>;
    using shared_copyable_graph
      = undirected_graph<copyable_weight, null_weight, null_meta_data, shared_edge_storage_config>;

    // A move-only edge weight, whether the halves of an edge hold it independently or share it
    STATIC_CHECK(!std::is_copy_constructible_v<unshared_move_only_graph>);
    STATIC_CHECK(!std::is_copy_constructible_v<unshared_move_only_contiguous_graph>);
    STATIC_CHECK(!std::is_copy_constructible_v<unshared_move_only_embedded_graph>);
    STATIC_CHECK(!std::is_copy_constructible_v<shared_move_only_graph>);
    STATIC_CHECK(!std::is_copy_constructible_v<shared_move_only_embedded_graph>);
    STATIC_CHECK(!std::is_copy_constructible_v<directed_move_only_graph>);
    STATIC_CHECK(!std::is_copy_assignable_v<unshared_move_only_graph>);
    STATIC_CHECK(!std::is_copy_assignable_v<shared_move_only_graph>);
    STATIC_CHECK(!std::is_constructible_v<directed_move_only_graph,
                                          const directed_move_only_graph&,
                                          directed_move_only_graph::edge_allocator_type>);

    // A move-only node weight
    STATIC_CHECK(!std::is_copy_constructible_v<move_only_node_graph>);
    STATIC_CHECK(!std::is_copy_assignable_v<move_only_node_graph>);
    STATIC_CHECK(!std::is_constructible_v<move_only_node_graph,
                                          const move_only_node_graph&,
                                          move_only_node_graph::edge_allocator_type,
                                          move_only_node_graph::node_weight_allocator_type>);

    // Trees and static graphs are copied as graphs are
    STATIC_CHECK(!std::is_copy_constructible_v<move_only_tree>);
    STATIC_CHECK(!std::is_copy_constructible_v<move_only_static>);

    // Copyable weights
    STATIC_CHECK(std::is_copy_constructible_v<unshared_copyable_graph>);
    STATIC_CHECK(std::is_copy_constructible_v<shared_copyable_graph>);
    STATIC_CHECK(std::is_copy_assignable_v<shared_copyable_graph>);
    STATIC_CHECK(std::is_constructible_v<unshared_copyable_graph,
                                         const unshared_copyable_graph&,
                                         unshared_copyable_graph::edge_allocator_type>);
    STATIC_CHECK(std::is_copy_constructible_v<directed_graph<null_weight, copyable_weight>>);
  }

  void graph_constraints_free_test::test_weight_update_constraints()
  {
    using namespace maths;

    STATIC_CHECK(!graph_impl::has_shared_weight_v<typename unshared_move_only_graph::edge_type>);
    STATIC_CHECK( graph_impl::has_shared_weight_v<typename shared_move_only_graph::edge_type>);

    STATIC_CHECK(!edge_weight_settable<unshared_move_only_graph>);
    STATIC_CHECK(!edge_weight_mutable_returning<unshared_move_only_graph, void>);
    STATIC_CHECK(!edge_weight_settable<unshared_move_only_graph,
                                       unshared_move_only_graph::const_reverse_edge_iterator>);
    STATIC_CHECK(!edge_weight_mutable_returning<unshared_move_only_graph,
                                                void,
                                                unshared_move_only_graph::const_reverse_edge_iterator>);

    STATIC_CHECK( edge_weight_settable<unshared_copyable_graph>);
    STATIC_CHECK( edge_weight_mutable_returning<unshared_copyable_graph, void>);
    STATIC_CHECK( edge_weight_mutable_returning<unshared_copyable_graph, int>);
    STATIC_CHECK(!edge_weight_mutable_returning<unshared_copyable_graph, copyable_weight&>);
    STATIC_CHECK(!edge_weight_mutable_returning<unshared_copyable_graph,
                                                copyable_weight&,
                                                unshared_copyable_graph::const_reverse_edge_iterator>);
    STATIC_CHECK(!edge_weight_mutable_returning<unshared_copyable_graph, non_movable>);

    STATIC_CHECK( edge_weight_settable<shared_move_only_graph>);
    STATIC_CHECK( edge_weight_mutable_returning<shared_move_only_graph, void>);
    STATIC_CHECK( edge_weight_mutable_returning<shared_move_only_graph, int>);
    STATIC_CHECK(!edge_weight_mutable_returning<shared_move_only_graph, move_only_weight&>);
    STATIC_CHECK(!edge_weight_mutable_returning<shared_move_only_graph,
                                                move_only_weight&,
                                                shared_move_only_graph::const_reverse_edge_iterator>);
    STATIC_CHECK(!edge_weight_mutable_returning<shared_move_only_graph, non_movable>);

    STATIC_CHECK( edge_weight_settable<directed_move_only_graph>);
    STATIC_CHECK( edge_weight_mutable_returning<directed_move_only_graph, void>);
    STATIC_CHECK(!edge_weight_mutable_returning<directed_move_only_graph, move_only_weight&>);
    STATIC_CHECK(!edge_weight_mutable_returning<directed_move_only_graph, non_movable>);

    STATIC_CHECK(!edge_weight_mutable_by<unshared_copyable_graph, rvalue_only_mutation<copyable_weight>>);
    STATIC_CHECK(!edge_weight_mutable_by<shared_move_only_graph, rvalue_only_mutation<move_only_weight>>);
  }

  void graph_constraints_free_test::test_join_constraints()
  {
    using namespace maths;

    STATIC_CHECK(!joinable<unshared_move_only_graph>);
    STATIC_CHECK(!joinable<unshared_move_only_embedded_graph>);
    STATIC_CHECK(!insert_joinable<unshared_move_only_embedded_graph>);

    STATIC_CHECK( joinable<unshared_copyable_graph>);
    STATIC_CHECK( joinable<shared_move_only_graph>);
    STATIC_CHECK( joinable<shared_move_only_embedded_graph>);
    STATIC_CHECK( insert_joinable<shared_move_only_embedded_graph>);
    STATIC_CHECK( joinable<directed_move_only_graph>);
  }

  void graph_constraints_free_test::test_mutator_constraints()
  {
    using namespace maths;
    using namespace object;

    using rvalue_only = rvalue_only_mutation<copyable_weight>;
    using weighted_edge        = partial_edge<by_value<copyable_weight>, null_meta_data>;
    using decorated_edge       = partial_edge<by_value<null_weight>, copyable_weight>;
    using meta_data_graph      = undirected_graph<null_weight, null_weight, copyable_weight>;
    using node_weighted_graph  = directed_graph<null_weight, copyable_weight>;
    using heterogeneous_graph  = heterogeneous_directed_graph<0, 1, null_weight, copyable_weight>;
    using tree                 = directed_tree<tree_link_direction::forward, null_weight, copyable_weight>;
    using member_function      = void (copyable_weight::*)();

    STATIC_CHECK(!edge_weight_mutable_by_value<weighted_edge, rvalue_only>);
    STATIC_CHECK( edge_weight_mutable_by_value<weighted_edge, member_function>);

    STATIC_CHECK(!edge_meta_data_mutable_by<decorated_edge, rvalue_only>);
    STATIC_CHECK( edge_meta_data_mutable_by<decorated_edge, member_function>);

    STATIC_CHECK(!graph_edge_meta_data_mutable_by<meta_data_graph, rvalue_only>);
    STATIC_CHECK(!graph_edge_meta_data_mutable_by<meta_data_graph,
                                                  rvalue_only,
                                                  meta_data_graph::const_reverse_edge_iterator>);
    STATIC_CHECK( graph_edge_meta_data_mutable_by<meta_data_graph, member_function>);

    STATIC_CHECK(!node_weight_mutable_by<node_weighted_graph, rvalue_only>);
    STATIC_CHECK( node_weight_mutable_by<node_weighted_graph, member_function>);

    STATIC_CHECK(!node_weight_at_mutable_by<heterogeneous_graph, 0, rvalue_only>);
    STATIC_CHECK(!node_weight_of_type_mutable_by<heterogeneous_graph, copyable_weight, rvalue_only>);
    STATIC_CHECK( node_weight_at_mutable_by<heterogeneous_graph, 0, member_function>);

    STATIC_CHECK(!root_weight_mutable_by<tree, rvalue_only>);
    STATIC_CHECK( root_weight_mutable_by<tree, member_function>);
  }

  template<class EdgeStorageConfig>
  void graph_constraints_free_test::test_mutation_results()
  {
    using namespace maths;

    const auto describe{
      [](std::string_view graphKind, std::string_view outcome) {
        return std::format("Mutate edge weight in {} with {}: {}",
                           graphKind,
                           meta::tidy_type_name(meta::type_name<EdgeStorageConfig>()),
                           outcome);
      }
    };

    const auto replaceWithSeven{
      [](copyable_weight& w) { return std::exchange(w.value, 7); }
    };

    {
      using graph_type     = undirected_graph<copyable_weight, null_weight, null_meta_data, EdgeStorageConfig>;
      using edge_init_type = graph_type::edge_init_type;

      const graph_type graph{{edge_init_type{1, 5}}, {edge_init_type{0, 5}}};

      {
        graph_type g{graph};
        check(equality,
              describe("an undirected graph", "returns the result of the mutation"),
              g.mutate_edge_weight(g.cbegin_edges(0), replaceWithSeven),
              5);
      }

      {
        graph_type g{graph};
        const auto returnList{[](copyable_weight&) { return std::vector<std::any>{1, 2, 3}; }};
        check(equality,
              describe("an undirected graph", "returns a result with an initializer-list constructor unchanged"),
              g.mutate_edge_weight(g.cbegin_edges(0), returnList).size(),
              std::size_t{3});
      }

      {
        graph_type g{graph};
        g.mutate_edge_weight(g.cbegin_edges(0), &copyable_weight::increment);
        check(equality,
              describe("an undirected graph", "a member function mutates both halves"),
              g,
              graph_type{{edge_init_type{1, 6}}, {edge_init_type{0, 6}}});
      }
    }

    {
      using graph_type     = embedded_graph<copyable_weight, null_weight, null_meta_data, EdgeStorageConfig>;
      using edge_init_type = graph_type::edge_init_type;

      graph_type g{{edge_init_type{1, 0, 5}}, {edge_init_type{0, 0, 5}}};
      check(equality,
            describe("an embedded graph", "returns the result of the mutation"),
            g.mutate_edge_weight(g.cbegin_edges(0), replaceWithSeven),
            5);
    }
  }

  void graph_constraints_free_test::test_shared_move_only_weights()
  {
    using namespace maths;

    {
      undirected_graph<move_only_weight, null_weight> g{};
      g.add_node();
      g.add_node();
      g.join(0, 1, move_only_weight{5});
      g.mutate_edge_weight(g.cbegin_edges(0), [](move_only_weight& w) { w.value = 7; });

      check("An undirected graph joins with a shared, move-only weight, which both halves hold",
            &g.cbegin_edges(1)->weight() == &g.cbegin_edges(0)->weight());
      check(equality, "The shared, move-only weight is mutated", g.cbegin_edges(1)->weight().value, 7);

      g.mutate_edge_weight(g.cbegin_edges(0), &move_only_weight::increment);
      check(equality,
            "The shared, move-only weight is mutated by a member function",
            g.cbegin_edges(1)->weight().value,
            8);
    }

    {
      embedded_graph<move_only_weight, null_weight> g{};
      g.add_node();
      g.add_node();
      g.join(0, 1, move_only_weight{5});
      g.insert_join(g.cbegin_edges(0), g.cbegin_edges(1), move_only_weight{6});

      check("An embedded graph inserts a join with a shared, move-only weight, which both halves hold",
            &g.cbegin_edges(1)->weight() == &g.cbegin_edges(0)->weight());
      check(equality, "The inserted join carries its weight", g.cbegin_edges(1)->weight().value, 6);
    }
  }

  void graph_constraints_free_test::test_move_only_meta_data()
  {
    using namespace maths;

    {
      undirected_graph<null_weight, null_weight, move_only_meta_data> g{};
      g.add_node();
      g.add_node();
      g.join(0, 1, move_only_meta_data{1}, move_only_meta_data{2});

      check(equality, "An undirected graph joins with move-only meta-data", g.cbegin_edges(0)->meta_data().value, 1);
      check(equality, "The partner half takes the second meta-data", g.cbegin_edges(1)->meta_data().value, 2);
    }

    {
      embedded_graph<null_weight, null_weight, move_only_meta_data> g{};
      g.add_node();
      g.add_node();
      g.join(0, 1, move_only_meta_data{1}, move_only_meta_data{2});
      g.insert_join(g.cbegin_edges(0), g.cbegin_edges(1), move_only_meta_data{3}, move_only_meta_data{4});
      g.insert_join(g.cbegin_edges(0), 0, move_only_meta_data{5}, move_only_meta_data{6});

      check(equality, "An embedded graph joins and inserts joins with move-only meta-data", g.size(), 3uz);
      check(equality,
            "The inserted join's partner half takes the second meta-data",
            g.cbegin_edges(1)->meta_data().value,
            4);
    }
  }

  void graph_constraints_free_test::test_shared_weight_copies()
  {
    using namespace maths;

    const auto checkCopy{
      [this]<class Graph>(std::string_view description, const Graph& graph) {
        const Graph copy{graph};
        check(equality, std::format("{}: the copy is equal", description), copy, graph);
        check(std::format("{}: the halves of the copy share one weight", description),
              &copy.cbegin_edges(0)->weight() == &copy.cbegin_edges(1)->weight());
        check(std::format("{}: the copy's weight is not the original's", description),
              &copy.cbegin_edges(0)->weight() != &graph.cbegin_edges(0)->weight());
      }
    };

    {
      undirected_graph<copyable_weight, null_weight, int, shared_edge_storage_config> g{};
      g.add_node();
      g.add_node();
      g.join(0, 1, 7, 8, copyable_weight{5});
      checkCopy("An undirected graph with shared weights and meta-data", g);
    }

    {
      embedded_graph<copyable_weight, null_weight, int, shared_edge_storage_config> g{};
      g.add_node();
      g.add_node();
      g.join(0, 1, 7, 8, copyable_weight{5});
      checkCopy("An embedded graph with shared weights and meta-data", g);
    }
  }

  void graph_constraints_free_test::test_mutation_by_member_functions()
  {
    using namespace maths;
    using namespace object;

    {
      partial_edge<by_value<copyable_weight>, null_meta_data> e{0, copyable_weight{1}};
      e.mutate_weight(&copyable_weight::increment);
      check(equality, "An edge's weight is mutated by a member function", e.weight().value, 2);
    }

    {
      partial_edge<by_value<null_weight>, copyable_weight> e{0, copyable_weight{1}};
      e.mutate_meta_data(&copyable_weight::increment);
      check(equality, "An edge's meta-data is mutated by a member function", e.meta_data().value, 2);
    }

    {
      undirected_graph<null_weight, null_weight, copyable_weight> g{};
      g.add_node();
      g.add_node();
      g.join(0, 1, copyable_weight{1}, copyable_weight{5});
      g.mutate_edge_meta_data(g.cbegin_edges(0), &copyable_weight::increment);
      check(equality,
            "A graph's edge meta-data is mutated by a member function",
            g.cbegin_edges(0)->meta_data().value,
            2);
      check(equality, "The partner half's meta-data is its own", g.cbegin_edges(1)->meta_data().value, 5);
    }

    {
      directed_graph<null_weight, copyable_weight> g{};
      g.add_node(copyable_weight{1});
      g.mutate_node_weight(g.cbegin_node_weights(), &copyable_weight::increment);
      check(equality, "A node weight is mutated by a member function", g.cbegin_node_weights()->value, 2);
    }

    {
      heterogeneous_directed_graph<0, 1, null_weight, copyable_weight> g{{{}}, copyable_weight{1}};
      g.mutate_node_weight<0>(&copyable_weight::increment);
      g.mutate_node_weight<copyable_weight>(&copyable_weight::increment);
      check(equality,
            "A heterogeneous node weight is mutated by a member function, by index and by type",
            g.get_node_weight<0>().value,
            3);
    }
  }
}
