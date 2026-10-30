////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CoarseSleepsFreeTest.hpp"

import std;
import sequoia.streaming;
import sequoia.test_framework;

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path coarse_sleeps_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void coarse_sleeps_free_test::run_tests()
  {
    test_coarse_sleep();
    test_combination_of_timer_request_outcomes();
    test_coarse_sleep_messages();
    test_outcomes_without_a_request();
  }

  void coarse_sleeps_free_test::test_coarse_sleep()
  {
    constexpr double_precision_milliseconds target{5.0};

    check("Rounded up to Windows' default tick", is_coarse_sleep(double_precision_milliseconds{15.2}, target));
    check("Exactly twice the target", is_coarse_sleep(double_precision_milliseconds{10.0}, target));
    check("Just under twice the target", !is_coarse_sleep(double_precision_milliseconds{9.9}, target));
    check("A 1 ms timer resolution in effect", !is_coarse_sleep(double_precision_milliseconds{5.4}, target));
  }

  void coarse_sleeps_free_test::test_combination_of_timer_request_outcomes()
  {
    using enum timer_request_outcome;

    auto combine{
      [](timer_request_outcome lhs, timer_request_outcome rhs) { return combine_timer_request_outcomes(lhs, rhs); }
    };

    check("Neither request made", combine(not_made, not_made) == not_made);
    check("An unobserved request, then none made", combine(not_observed, not_made) == not_observed);
    check("No request made, then an unobserved one", combine(not_made, not_observed) == not_observed);
    check("Both requests unobserved", combine(not_observed, not_observed) == not_observed);
    check("A granted request, then none made", combine(granted, not_made) == granted);
    check("No request made, then a granted one", combine(not_made, granted) == granted);
    check("A granted request, then an unobserved one", combine(granted, not_observed) == granted);
    check("An unobserved request, then a granted one", combine(not_observed, granted) == granted);
    check("Both requests granted", combine(granted, granted) == granted);
    check("A failed request, then none made", combine(failed, not_made) == failed);
    check("No request made, then a failed one", combine(not_made, failed) == failed);
    check("A failed request, then an unobserved one", combine(failed, not_observed) == failed);
    check("An unobserved request, then a failed one", combine(not_observed, failed) == failed);
    check("A failed request, then a granted one", combine(failed, granted) == failed);
    check("A granted request, then a failed one", combine(granted, failed) == failed);
    check("Both requests failed", combine(failed, failed) == failed);
  }

  void coarse_sleeps_free_test::test_coarse_sleep_messages()
  {
    const double_precision_milliseconds target{5.0};
    const double_precision_milliseconds slept{15.2};

    auto checkMessage{
      [this, target, slept](const reporter& description,
                            timer_request request,
                            timer_request_outcome outcome,
                            std::string_view fileName) {
        write_to_file(working_materials() /= fileName,
                      coarse_sleep_message(slept, target, request, outcome),
                      std::ios_base::out);
        check(equivalence, description, working_materials() /= fileName, predictive_materials() /= fileName);
      }
    };

    checkMessage("The request for the finest timer resolution failed",
                 timer_request::finest_timer_resolution,
                 timer_request_outcome::failed,
                 "FinestTimerResolutionRequestFailed.txt");

    checkMessage("The request for the finest timer resolution was granted",
                 timer_request::finest_timer_resolution,
                 timer_request_outcome::granted,
                 "FinestTimerResolutionRequestGranted.txt");

    checkMessage("The outcome of the request for the finest timer resolution was not observed",
                 timer_request::finest_timer_resolution,
                 timer_request_outcome::not_observed,
                 "FinestTimerResolutionRequestNotObserved.txt");

    checkMessage("No request for the finest timer resolution was made",
                 timer_request::finest_timer_resolution,
                 timer_request_outcome::not_made,
                 "FinestTimerResolutionRequestNotMade.txt");

    checkMessage("A request for a precise timer failed",
                 timer_request::precise_timer,
                 timer_request_outcome::failed,
                 "PreciseTimerRequestFailed.txt");

    checkMessage("The requests for a precise timer were granted",
                 timer_request::precise_timer,
                 timer_request_outcome::granted,
                 "PreciseTimerRequestGranted.txt");

    checkMessage("The outcome of a request for a precise timer was not observed",
                 timer_request::precise_timer,
                 timer_request_outcome::not_observed,
                 "PreciseTimerRequestNotObserved.txt");

    checkMessage("No request for a precise timer was made",
                 timer_request::precise_timer,
                 timer_request_outcome::not_made,
                 "PreciseTimerRequestNotMade.txt");

    checkMessage("The platform makes no request",
                 timer_request::none,
                 timer_request_outcome::not_made,
                 "NoTimerRequestMade.txt");
  }

  void coarse_sleeps_free_test::test_outcomes_without_a_request()
  {
    const double_precision_milliseconds target{5.0};
    const double_precision_milliseconds slept{15.2};

    auto checkRefusal{
      [this, target, slept](const reporter& description, timer_request_outcome outcome) {
        check_exception_thrown<std::logic_error>(
          description,
          [target, slept, outcome]() { return coarse_sleep_message(slept, target, timer_request::none, outcome); }
        );
      }
    };

    checkRefusal("An unobserved outcome without a request", timer_request_outcome::not_observed);
    checkRefusal("A failed outcome without a request", timer_request_outcome::failed);
    checkRefusal("A granted outcome without a request", timer_request_outcome::granted);
  }
}
