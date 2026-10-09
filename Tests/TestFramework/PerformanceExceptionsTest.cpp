////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PerformanceExceptionsTest.hpp"

#include <limits>
#include <stdexcept>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path performance_exceptions_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_exceptions_test::run_tests()
  {
    test_invalid_arguments();
    test_throwing_task();
  }

  void performance_exceptions_test::test_invalid_arguments()
  {
    constexpr double nan{std::numeric_limits<double>::quiet_NaN()};

    auto relativePerformanceCheck{
      [this](const relative_performance_parameters& parameters) {
        return [this, parameters]() {
          return check_relative_performance("Relative performance with invalid arguments",
                                            []() {},
                                            []() {},
                                            parameters);
        };
      }
    };

    check_exception_thrown<std::invalid_argument>("Minimum speed-up of 1",
                                                  relativePerformanceCheck({.min_speedup{1.0},
                                                                            .max_speedup{2.0},
                                                                            .trials{5},
                                                                            .num_sds{4.0},
                                                                            .max_attempts{3}}));
    check_exception_thrown<std::invalid_argument>("Maximum speed-up of 1",
                                                  relativePerformanceCheck({.min_speedup{1.5},
                                                                            .max_speedup{1.0},
                                                                            .trials{5},
                                                                            .num_sds{4.0},
                                                                            .max_attempts{3}}));
    check_exception_thrown<std::invalid_argument>("Minimum speed-up of NaN",
                                                  relativePerformanceCheck({.min_speedup{nan},
                                                                            .max_speedup{2.0},
                                                                            .trials{5},
                                                                            .num_sds{4.0},
                                                                            .max_attempts{3}}));
    check_exception_thrown<std::invalid_argument>("Minimum speed-up exceeding the maximum",
                                                  relativePerformanceCheck({.min_speedup{2.5},
                                                                            .max_speedup{2.0},
                                                                            .trials{5},
                                                                            .num_sds{4.0},
                                                                            .max_attempts{3}}));
    check_exception_thrown<std::invalid_argument>("One standard deviation",
                                                  relativePerformanceCheck({.min_speedup{2.0},
                                                                            .max_speedup{3.0},
                                                                            .trials{5},
                                                                            .num_sds{1.0},
                                                                            .max_attempts{3}}));
    check_exception_thrown<std::invalid_argument>("NaN standard deviations",
                                                  relativePerformanceCheck({.min_speedup{2.0},
                                                                            .max_speedup{3.0},
                                                                            .trials{5},
                                                                            .num_sds{nan},
                                                                            .max_attempts{3}}));
    check_exception_thrown<std::invalid_argument>("No attempts",
                                                  relativePerformanceCheck({.min_speedup{2.0},
                                                                            .max_speedup{3.0},
                                                                            .trials{5},
                                                                            .num_sds{4.0},
                                                                            .max_attempts{0}}));
    check_exception_thrown<std::invalid_argument>("Four trials",
                                                  relativePerformanceCheck({.min_speedup{2.0},
                                                                            .max_speedup{3.0},
                                                                            .trials{4},
                                                                            .num_sds{4.0},
                                                                            .max_attempts{3}}));
  }

  void performance_exceptions_test::test_throwing_task()
  {
    auto checkWithThrowingFastTask{
      [this]() {
        return check_relative_performance("Relative performance with a throwing task",
                                          []() { throw std::runtime_error{"Fast task failure"}; },
                                          []() {},
                                          {.min_speedup{2.0},
                                           .max_speedup{3.0},
                                           .trials{5},
                                           .num_sds{4.0},
                                           .max_attempts{3}});
      }
    };

    check_exception_thrown<std::runtime_error>("Fast task throws", checkWithThrowingFastTask);
  }
}
