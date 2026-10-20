////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerConcurrencyTestingUtilities.hpp"

#include "sequoia/PlatformSpecific/Helpers.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <numeric>
#include <ranges>
#include <source_location>
#include <utility>
#include <vector>

namespace sequoia::testing
{
  namespace
  {
    /** Each slow test is a class of its own, derived from this, since the
        runner names a test after its class. A test may not be a class
        template, and the runner refuses two registrations of one class.
     */
    class slow_test_base : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::filesystem::path source_file()
      {
        return std::source_location::current().file_name();
      }

      [[nodiscard]]
      static std::span<const sleep_interval, slow_test_count> sleep_intervals() noexcept
      {
        return st_SleepIntervals;
      }

      static void clear_sleep_intervals() noexcept { st_SleepIntervals = {}; }

      static void set_sleep_duration(std::chrono::milliseconds duration) noexcept { st_SleepDuration = duration; }
    protected:
      ~slow_test_base() = default;

      slow_test_base(slow_test_base&&)            noexcept = default;
      slow_test_base& operator=(slow_test_base&&) noexcept = default;

      /** Sleeps for the duration last passed to `set_sleep_duration`,
          records the sleep's `sleep_interval` at `index`, and makes one
          check.
       */
      void sleep_and_check(std::size_t index)
      {
        const auto start{std::chrono::steady_clock::now()};
        precise_sleep_for(st_SleepDuration);
        st_SleepIntervals[index] = {start, std::chrono::steady_clock::now()};
        check(equality, "Integer equality", index, index);
      }
    private:
      // In an anonymous namespace, the members below are not weak symbols.
      // gcc on macOS may address a translation unit's std::source_location
      // data relative to a weak symbol, and the linker may take that symbol
      // from another unit (GCC PR 126723).

      // Each slow test writes only its own element. A runner's execute waits
      // for every test, so the elements may be read once it returns.
      inline static std::array<sleep_interval, slow_test_count> st_SleepIntervals{};

      // Set before a run begins, and read only by its tests
      inline static std::chrono::milliseconds st_SleepDuration{};
    };

    class slow_test_0 final : public slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(0); }
    };

    class slow_test_1 final : public slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(1); }
    };

    class slow_test_2 final : public slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(2); }
    };

    class slow_test_3 final : public slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(3); }
    };

    class slow_test_4 final : public slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(4); }
    };

    class slow_test_5 final : public slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(5); }
    };

    class slow_test_6 final : public slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(6); }
    };

    class slow_test_7 final : public slow_test_base
    {
    public:
      using slow_test_base::slow_test_base;

      void run_tests() { sleep_and_check(7); }
    };
  }

  [[nodiscard]]
  std::size_t peak_overlap(std::span<const sleep_interval> intervals)
  {
    using change_t = std::pair<std::chrono::steady_clock::time_point, std::ptrdiff_t>;
    auto endpoints{
      [](const sleep_interval& interval){
        return std::array{change_t{interval.start, 1}, change_t{interval.end, -1}};
      }
    };

    auto changes{
        intervals
      | std::views::transform(endpoints)
      | std::views::join
      | std::ranges::to<std::vector>()
    };

    // At one instant an end, -1, sorts before a start, +1
    std::ranges::sort(changes);

    // The overlap just after each change is the sum of the changes up to it
    auto overlaps{changes | std::views::values | std::ranges::to<std::vector>()};
    std::inclusive_scan(overlaps.begin(), overlaps.end(), overlaps.begin());

    return static_cast<std::size_t>(std::ranges::fold_left(overlaps, std::ptrdiff_t{}, std::ranges::max));
  }

  [[nodiscard]]
  std::string peak_overlap_line(std::size_t peak)
  {
    return std::format("[Peak Overlap: {}]\n", peak);
  }

  [[nodiscard]]
  std::span<const sleep_interval, slow_test_count> slow_test_sleep_intervals() noexcept
  {
    return slow_test_base::sleep_intervals();
  }

  [[nodiscard]]
  test_runner impl::make_slow_test_runner(commandline_arguments& args,
                                          std::stringstream& outputStream,
                                          std::chrono::milliseconds sleepDuration)
  {
    auto runner{make_fake_project_runner(args, outputStream)};
    slow_test_base::clear_sleep_intervals();
    slow_test_base::set_sleep_duration(sleepDuration);

    runner.register_test<slow_test_0>();
    runner.register_test<slow_test_1>();
    runner.register_test<slow_test_2>();
    runner.register_test<slow_test_3>();
    runner.register_test<slow_test_4>();
    runner.register_test<slow_test_5>();
    runner.register_test<slow_test_6>();
    runner.register_test<slow_test_7>();

    return runner;
  }
}
