////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  class dynamic_graph_exception_safety_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:
    void test_weight_update_constraints();

    void test_join_constraints();

    template<class EdgeStorageConfig>
    void test_undirected_edge_mutations();

    template<class EdgeStorageConfig>
    void test_embedded_edge_mutations();

    template<class EdgeStorageConfig>
    void test_shared_weight_edge_mutations();

    void test_node_insertion();

    void test_shared_move_only_weights();

    void test_move_only_meta_data();

    template<class Graph>
    using transition_graph_type = transition_checker<Graph>::transition_graph;

    template<class Graph, class Mutation>
    void add_fallible_transition(transition_graph_type<Graph>& trg,
                                 std::string_view context,
                                 std::size_t host,
                                 std::size_t target,
                                 std::string description,
                                 std::size_t predictedFallibleSteps,
                                 Mutation mutation);

    template<class Graph>
    void check_transitions(std::string_view context, const transition_graph_type<Graph>& trg);
  };
}
