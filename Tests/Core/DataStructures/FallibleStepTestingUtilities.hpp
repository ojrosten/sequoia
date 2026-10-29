////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Utilities for testing what a failure at each fallible step of an
           operation leaves behind.
 */

#include "sequoia/Core/Numeric/IntegerConversions.hpp"
#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <concepts>
#include <cstddef>
#include <format>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>

namespace sequoia::testing
{
  struct injected_failure : std::runtime_error
  {
    using std::runtime_error::runtime_error;
  };

  /** \brief An RAII wrapper to count the fallible steps that its thread takes
             during its lifetime.

      A fallible step is a call of `take_fallible_step`. Steps are numbered
      from zero. If `failingStep` holds a value, the step with that number
      throws `injected_failure`. A step fails only while a monitor is alive on
      the thread that takes the step.
   */
  class [[nodiscard]] fallible_step_monitor
  {
  public:
    explicit fallible_step_monitor(std::optional<std::size_t> failingStep) noexcept
    {
      current_monitoring() = monitoring{.failing_step{failingStep}};
    }

    ~fallible_step_monitor() { current_monitoring().reset(); }

    fallible_step_monitor(const fallible_step_monitor&)            = delete;
    fallible_step_monitor& operator=(const fallible_step_monitor&) = delete;

    [[nodiscard]]
    std::size_t steps_taken() const noexcept { return current_monitoring()->steps_taken; }

    static void take_fallible_step()
    {
      if(auto& current{current_monitoring()})
      {
        const auto step{current->steps_taken++};
        if(step == current->failing_step)
        {
          current->failing_step.reset();
          throw injected_failure{std::format("Injected failure at fallible step {}", step)};
        }
      }
    }
  private:
    struct monitoring
    {
      std::optional<std::size_t> failing_step;
      std::size_t steps_taken{};
    };

    [[nodiscard]]
    static std::optional<monitoring>& current_monitoring() noexcept
    {
      thread_local std::optional<monitoring> current{};
      return current;
    }
  };

  /** \brief A member whose copy construction and copy assignment each take a
             fallible step, and whose moves take none.
   */
  struct fallible_copy
  {
    fallible_copy() = default;

    fallible_copy(const fallible_copy&) { fallible_step_monitor::take_fallible_step(); }

    fallible_copy(fallible_copy&&) noexcept = default;

    fallible_copy& operator=(const fallible_copy&)
    {
      fallible_step_monitor::take_fallible_step();
      return *this;
    }

    fallible_copy& operator=(fallible_copy&&) noexcept = default;

    [[nodiscard]]
    friend constexpr auto operator<=>(const fallible_copy&, const fallible_copy&) noexcept = default;
  };

  /** \brief Joins `host` to `target` in `trg` by `mutation`, checking that
             `mutation` takes `predictedFallibleSteps` fallible steps, and
             adds a self-loop at `host` for each step `mutation` takes, on
             which that step throws `injected_failure`.

      To count the steps, `mutation` runs once on `host`'s state during the
      call.
   */
  template<class T, class Extender, class Mutation>
    requires std::invocable<const Mutation&, T&>
  void add_fallible_transition(basic_test<test_mode::standard, Extender>& test,
                               typename transition_checker<T>::transition_graph& trg,
                               std::string_view context,
                               std::size_t host,
                               std::size_t target,
                               std::string description,
                               std::size_t predictedFallibleSteps,
                               Mutation mutation)
  {
    auto locate{
      [context](std::size_t from, std::size_t to, std::string_view what) {
        return append_lines(context, std::format("Transition from node {} to {}", from, to), what);
      }
    };

    const auto stepsMessage{locate(host, target, std::format("{}: fallible steps", description))};
    trg.join(
      host,
      target,
      description,
      [&test, stepsMessage, predictedFallibleSteps, mutation](T t) -> T {
        const fallible_step_monitor monitor{std::nullopt};
        mutation(t);
        test.check(equality, {stepsMessage, no_source_location}, monitor.steps_taken(), predictedFallibleSteps);
        return t;
      }
    );

    // Counting the steps here, rather than trusting the prediction, gives a
    // step beyond the prediction a loop of its own
    const auto fallibleStepsTaken{
      [&trg, host, &mutation]() {
        T t{trg.cbegin_node_weights()[numeric::unchecked_conversion_to<std::ptrdiff_t>(host)]()};
        const fallible_step_monitor monitor{std::nullopt};
        mutation(t);
        return monitor.steps_taken();
      }()
    };

    for(const auto step : std::views::iota(0uz, fallibleStepsTaken))
    {
      const auto failureMessage{locate(host, host, std::format("{}: failure at fallible step {}", description, step))};
      trg.join(
        host,
        host,
        std::format("{}: unchanged by the failure at fallible step {}", description, step),
        [&test, failureMessage, step, mutation](T t) -> T {
          test.template check_exception_thrown<injected_failure>(
            {failureMessage, no_source_location},
            [&t, &mutation, step]() {
              const fallible_step_monitor monitor{step};
              mutation(t);
            }
          );

          return t;
        }
      );
    }
  }
}
