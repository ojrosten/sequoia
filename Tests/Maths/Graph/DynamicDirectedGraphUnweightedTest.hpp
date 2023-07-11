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
  class dynamic_directed_graph_unweighted_test final : public dynamic_directed_graph_operations
  {
  public:
    using dynamic_directed_graph_operations::dynamic_directed_graph_operations;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    template <class, class, concrete_test>
    friend class graph_test_helper;
  };
}