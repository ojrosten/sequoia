////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicDirectedGraphUnweightedBucketedTest.hpp"

namespace sequoia::testing
{

  [[nodiscard]]
  std::filesystem::path dynamic_directed_graph_unweighted_bucketed_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void dynamic_directed_graph_unweighted_bucketed_test::run_tests()
  {
    execute_operations();
  }

  void dynamic_directed_graph_unweighted_bucketed_test::execute_operations()
  {
    auto trg{this->make_transition_graph()};

    auto checker{
        [this](std::string_view description, const graph_t& obtained, const graph_t& prediction, const graph_t& parent, std::size_t host, std::size_t target) {
          check(equality, description, obtained, prediction);
          if(host != target) this->check_semantics(description, prediction, parent);
        }
    };

    transition_checker<graph_t>::check(report_line(""), trg, checker);
  }

}