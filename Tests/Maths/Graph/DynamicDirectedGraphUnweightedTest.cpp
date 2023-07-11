////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicDirectedGraphUnweightedTest.hpp"

namespace sequoia::testing
{

  [[nodiscard]]
  std::filesystem::path dynamic_directed_graph_unweighted_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void dynamic_directed_graph_unweighted_test::run_tests()
  {
    using namespace maths;
    graph_test_helper<null_weight, null_weight, dynamic_directed_graph_unweighted_test> helper{*this};

    helper.run_tests<graph_flavour::directed>();
  }
}