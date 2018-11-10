#pragma once

#include "GraphTestingUtils.hpp"

namespace sequoia::unit_testing
{
  class test_static_graph_traversals : public graph_unit_test
  {
  public:
    using graph_unit_test::graph_unit_test;

  private:
    void run_tests() override;

    constexpr static auto topological_sort();

    constexpr static auto bfs();

    constexpr static auto priority_search();
  };
}
