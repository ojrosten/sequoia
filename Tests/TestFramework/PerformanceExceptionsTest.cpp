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
                                                  relativePerformanceCheck({.prediction{.lower{1.0}, .upper{2.0}},
                                                                            .trials{5}}));
    check_exception_thrown<std::invalid_argument>("Maximum speed-up of 1",
                                                  relativePerformanceCheck({.prediction{.lower{1.5}, .upper{1.0}},
                                                                            .trials{5}}));
    check_exception_thrown<std::invalid_argument>("Minimum speed-up of NaN",
                                                  relativePerformanceCheck({.prediction{.lower{nan}, .upper{2.0}},
                                                                            .trials{5}}));
    check_exception_thrown<std::invalid_argument>("Minimum speed-up exceeding the maximum",
                                                  relativePerformanceCheck({.prediction{.lower{2.5}, .upper{2.0}},
                                                                            .trials{5}}));
    check_exception_thrown<std::invalid_argument>("Four trials",
                                                  relativePerformanceCheck({.prediction{.lower{2.0}, .upper{3.0}},
                                                                            .trials{4}}));
  }

  void performance_exceptions_test::test_throwing_task()
  {
    auto checkWithThrowingFastTask{
      [this]() {
        return check_relative_performance("Relative performance with a throwing task",
                                          []() { throw std::runtime_error{"Fast task failure"}; },
                                          []() {},
                                          {.prediction{.lower{2.0}, .upper{3.0}},
                                           .trials{5}});
      }
    };

    check_exception_thrown<std::runtime_error>("Fast task throws", checkWithThrowingFastTask);
  }
}
