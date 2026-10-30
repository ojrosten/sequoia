////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PerformanceRealClockDiagnostics.hpp"

import std;

namespace sequoia::testing
{
  namespace
  {
    /** \brief Returns the state of Marsaglia's 64-bit xorshift generator
               after `steps` steps from `seed`.

        Each step depends on the one before, so the computation takes time in
        proportion to `steps`.
     */
    [[nodiscard]]
    constexpr std::uint64_t xorshift_state(std::uint64_t seed, std::size_t steps) noexcept
    {
      auto nextState{
        [](std::uint64_t state, std::size_t) {
          state ^= state << 13;
          state ^= state >> 7;
          return state ^ (state << 17);
        }
      };

      return std::ranges::fold_left(std::views::iota(0uz, steps), seed, nextState);
    }
  }

  [[nodiscard]]
  std::filesystem::path performance_real_clock_false_positive_diagnostics::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_real_clock_false_positive_diagnostics::run_tests()
  {
    test_computations_not_optimized_away();
  }

  void performance_real_clock_false_positive_diagnostics::test_computations_not_optimized_away()
  {
    // Nothing but do_not_optimize_away uses the result of either task's
    // computation. Without do_not_optimize_away, an optimized build removes
    // the computation. A trial then times the tasks at or near zero, and the
    // check fails or throws. An unoptimized build removes nothing, so in such
    // a build these checks pass with or without do_not_optimize_away. The
    // seed is drawn at run time, so that no build can compute the result at
    // compile time.
    const std::uint64_t seed{std::random_device{}()};
    constexpr std::size_t steps{500'000};

    check_relative_performance("A speed-up of 2 in computing a value a task passes to do_not_optimize_away, "
                               "predicted [1.5, 2.5]",
                               [seed]() { do_not_optimize_away(xorshift_state(seed,     steps)); },
                               [seed]() { do_not_optimize_away(xorshift_state(seed, 2 * steps)); },
                               {.prediction{.lower{1.5}, .upper{2.5}}});

    check_relative_performance("A speed-up of 2 in computing the value a task returns, predicted [1.5, 2.5]",
                               [seed]() { return xorshift_state(seed,     steps); },
                               [seed]() { return xorshift_state(seed, 2 * steps); },
                               {.prediction{.lower{1.5}, .upper{2.5}}});
  }
}
