////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicGraphUnweightedBootstrappedTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view unweighted_graph_bootstrapped_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void unweighted_graph_bootstrapped_test::run_tests()
  {
    execute_operations();
  }

  void unweighted_graph_bootstrapped_test::execute_operations()
  {

  }
}