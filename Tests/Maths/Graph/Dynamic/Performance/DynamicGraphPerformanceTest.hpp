////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/Maths/Graph/GraphTraits.hpp"
#include "sequoia/TestFramework/PerformanceTestCore.hpp"

namespace sequoia::testing
{
  class dynamic_graph_performance_test final : public performance_test
  {
  public:
    using performance_test::performance_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:
    template<maths::dynamic_network Graph>
    void test_sort_nodes(std::string_view graphName);

    template<maths::dynamic_network Graph>
    void test_sort_nodes_is_stable(std::string_view graphName);

    template<maths::dynamic_network Graph>
    void test_build_time_scales_linearly(std::string_view graphName);
  };
}
