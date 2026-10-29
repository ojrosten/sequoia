////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ExceptionSafetyTestUtilitiesFreeTest.hpp"
#include "sequoia/TestFramework/ExceptionSafetyTestUtilities.hpp"

#include <future>
#include <optional>
#include <stdexcept>
#include <utility>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path exception_safety_test_utilities_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void exception_safety_test_utilities_free_test::run_tests()
  {
    test_steps_counted();
    test_chosen_step_fails();
    test_monitor_counts_only_its_lifetime();
    test_each_thread_monitored_apart();
    test_nested_monitor_refused();
    test_fallible_copy();
  }

  void exception_safety_test_utilities_free_test::test_steps_counted()
  {
    const fallible_step_monitor monitor{std::nullopt};
    fallible_step_monitor::take_fallible_step();
    fallible_step_monitor::take_fallible_step();
    fallible_step_monitor::take_fallible_step();

    check(equality, "Three steps taken under a monitor which fails none", monitor.steps_taken(), 3uz);
  }

  void exception_safety_test_utilities_free_test::test_chosen_step_fails()
  {
    const fallible_step_monitor monitor{1};
    fallible_step_monitor::take_fallible_step();
    check_exception_thrown<injected_failure>("Step 1, chosen to fail", fallible_step_monitor::take_fallible_step);
    fallible_step_monitor::take_fallible_step();

    check(equality, "Every step is counted, the failing one included", monitor.steps_taken(), 3uz);
  }

  void exception_safety_test_utilities_free_test::test_monitor_counts_only_its_lifetime()
  {
    {
      const fallible_step_monitor earlier{0};
    }

    // Would throw were the earlier monitor's choice of step 0 still in force
    fallible_step_monitor::take_fallible_step();

    const fallible_step_monitor monitor{std::nullopt};
    fallible_step_monitor::take_fallible_step();
    check(equality, "A monitor counts only the steps taken in its lifetime", monitor.steps_taken(), 1uz);
  }

  void exception_safety_test_utilities_free_test::test_each_thread_monitored_apart()
  {
    const fallible_step_monitor monitor{0};

    // An exception on the other thread reaches this test through get
    const auto stepsTakenOnOtherThread{
      std::async(std::launch::async,
                 []() {
                   const fallible_step_monitor otherMonitor{std::nullopt};
                   fallible_step_monitor::take_fallible_step();
                   return otherMonitor.steps_taken();
                 }).get()
    };

    check(equality,
          "A step on another thread, under that thread's own monitor, is counted there",
          stepsTakenOnOtherThread,
          1uz);
    check(equality, "A step on another thread is not counted here", monitor.steps_taken(), 0uz);
  }

  void exception_safety_test_utilities_free_test::test_nested_monitor_refused()
  {
    const fallible_step_monitor monitor{std::nullopt};
    check_exception_thrown<std::logic_error>(
      "A second monitor on the same thread",
      []() { const fallible_step_monitor nested{std::nullopt}; }
    );

    fallible_step_monitor::take_fallible_step();
    check(equality, "The first monitor still counts after the refusal", monitor.steps_taken(), 1uz);
  }

  void exception_safety_test_utilities_free_test::test_fallible_copy()
  {
    const fallible_step_monitor monitor{std::nullopt};

    fallible_copy original{};
    fallible_copy copy{original};
    check(equality, "Copy construction takes a step", monitor.steps_taken(), 1uz);

    copy = original;
    check(equality, "Copy assignment takes a step", monitor.steps_taken(), 2uz);

    fallible_copy moved{std::move(copy)};
    moved = std::move(original);
    check(equality, "Moves take no step", monitor.steps_taken(), 2uz);
  }
}
