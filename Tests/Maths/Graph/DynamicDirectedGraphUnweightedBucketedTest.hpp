////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "DynamicDirectedGraphTestingUtilities.hpp"

namespace sequoia::testing
{
  class dynamic_directed_graph_unweighted_bucketed_test final
    : public dynamic_directed_graph_operations<maths::null_weight, maths::null_weight, object::faithful_producer<maths::null_weight>, object::faithful_producer<maths::null_weight>, independent_bucketed_edge_storage_traits, maths::node_weight_storage_traits<maths::null_weight>>
  {
  public:
    using dynamic_directed_graph_operations<maths::null_weight, maths::null_weight, object::faithful_producer<maths::null_weight>, object::faithful_producer<maths::null_weight>, independent_bucketed_edge_storage_traits, maths::node_weight_storage_traits<maths::null_weight>>::dynamic_directed_graph_operations;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    void execute_operations();
  };
}