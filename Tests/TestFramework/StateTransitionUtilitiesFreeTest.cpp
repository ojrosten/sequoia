////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "StateTransitionUtilitiesFreeTest.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  namespace
  {
    /// A walk written as a function of a logger: given this test's, it runs; given a fresh one, it is a constant evaluation
    template<test_mode Mode>
    constexpr void walk_doubles(test_logger<Mode>& logger)
    {
      using checker = transition_checker<double, check_ordering::no>;
      using graph_t = checker::transition_graph;
      using edge_t  = graph_t::edge_type;

      // Captured by the transitions and by a generator, so that the walk exercises what a
      // function pointer could not carry into a constant evaluation
      const double step{1.5};

      const graph_t g{
        { { edge_t{1, "Adding the step", [step](const double& f) { return f + step; }} },

          { edge_t{0, "Subtracting the step", [step](const double& f) { return f - step; }},
            edge_t{2, "Multiplying by 2", [](const double& f) { return f * 2; }}
          },

          { edge_t{1, "Dividing by 2", [](const double& f) { return f / 2; }} }
        },
        {0.0, step, [step]() { return 2 * step; }}
      };

      const auto checkerFn{
        [&logger](std::string_view description, double obtained, double prediction) {
          check(equality, std::string{description}, logger, obtained, prediction);
        }
      };

      checker::check("Doubles", g, checkerFn);
    }
  }

  [[nodiscard]]
  std::filesystem::path state_transition_utilities_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void state_transition_utilities_free_test::run_tests()
  {
    test_walk_at_both_times();
  }

  void state_transition_utilities_free_test::test_walk_at_both_times()
  {
    EVALUATE_STATICALLY_AND_DYNAMICALLY([](auto& logger){ walk_doubles(logger); });
  }
}
