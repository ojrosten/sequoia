////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "MemOrderedTupleTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  using namespace datastructures;
  
  [[nodiscard]]
  std::filesystem::path mem_ordered_tuple_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void mem_ordered_tuple_test::run_tests()
  {
    check_regular_semantics();
    check_transitions();
  }

  void mem_ordered_tuple_test::check_regular_semantics()
  {
    check_semantics(
      "",
      mem_ordered_tuple<int>{42},
      mem_ordered_tuple<int>{7},
      std::tuple{42},
      std::tuple{7},
      std::weak_ordering::greater);

    check_semantics(
      "",      
      mem_ordered_tuple<int, int>{7, 6},
      mem_ordered_tuple<int, int>{42, 5},
      std::tuple{7, 6},
      std::tuple{42, 5},
      std::weak_ordering::less);
  }

  void mem_ordered_tuple_test::check_transitions()
  {
    using tuple_t    = mem_ordered_tuple<int, int>;
    using graph_type = transition_checker<tuple_t>::transition_graph;
    using edge_t     = transition_checker<tuple_t>::edge;

    graph_type g{
      {
        { edge_t{1, "", [] (tuple_t t) -> tuple_t { get<0>(t) += 1; return t; }, std::weak_ordering::greater},
          edge_t{2, "", [] (tuple_t t) -> tuple_t { get<1>(t) += 2; return t; }, std::weak_ordering::greater}
        },
        {},
        {}
      },
      {tuple_t{0, 0}, tuple_t{1, 0}, tuple_t{0, 2}}
    };

    auto checker{
      [this](std::string_view description, const tuple_t& obtained, const tuple_t& prediction, const tuple_t& parent, std::weak_ordering ordering) {
        this->check(equality, description, obtained, prediction);
        if(ordering != std::weak_ordering::equivalent)
          this->check_semantics(description, prediction, parent, ordering);
      }
    };

    transition_checker<tuple_t>::check(report(""), g, checker);
  }
}
