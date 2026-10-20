////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CoarseSleepsFreeTest.hpp"
#include "sequoia/TestFramework/CoarseSleeps.hpp"
#include "sequoia/Streaming/Streaming.hpp"

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
    test_choice_of_sleep_timing_evidence();
    test_coarse_sleep_messages();
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

    check("Neither request made", combine_timer_request_outcomes(not_made, not_made) == not_made);
    check("A granted request, then none made", combine_timer_request_outcomes(granted, not_made) == granted);
    check("No request made, then a granted one", combine_timer_request_outcomes(not_made, granted) == granted);
    check("Both requests granted", combine_timer_request_outcomes(granted, granted) == granted);
    check("A failed request, then none made", combine_timer_request_outcomes(failed, not_made) == failed);
    check("No request made, then a failed one", combine_timer_request_outcomes(not_made, failed) == failed);
    check("A failed request, then a granted one", combine_timer_request_outcomes(failed, granted) == failed);
    check("A granted request, then a failed one", combine_timer_request_outcomes(granted, failed) == failed);
    check("Both requests failed", combine_timer_request_outcomes(failed, failed) == failed);
  }

  void coarse_sleeps_free_test::test_choice_of_sleep_timing_evidence()
  {
    using enum timer_request_outcome;
    using evidence = sleep_timing_evidence;

    check("No request is made",
          choose_sleep_timing_evidence({.finest_timer_resolution{not_made}, .precise_timer{not_made}})
            == evidence::no_timer_request_made);

    check("The request for the finest timer resolution failed",
          choose_sleep_timing_evidence({.finest_timer_resolution{failed}, .precise_timer{not_made}})
            == evidence::finest_timer_resolution_request_failed);

    check("The request for the finest timer resolution was granted",
          choose_sleep_timing_evidence({.finest_timer_resolution{granted}, .precise_timer{not_made}})
            == evidence::finest_timer_resolution_request_granted);

    check("A request for a precise timer failed",
          choose_sleep_timing_evidence({.finest_timer_resolution{not_made}, .precise_timer{failed}})
            == evidence::precise_timer_request_failed);

    check("The requests for a precise timer were granted",
          choose_sleep_timing_evidence({.finest_timer_resolution{not_made}, .precise_timer{granted}})
            == evidence::precise_timer_request_granted);

    check("A failed request for a precise timer is chosen over a granted request for the finest timer resolution",
          choose_sleep_timing_evidence({.finest_timer_resolution{granted}, .precise_timer{failed}})
            == evidence::precise_timer_request_failed);

    check("A failed request for the finest timer resolution is chosen over a granted request for a precise timer",
          choose_sleep_timing_evidence({.finest_timer_resolution{failed}, .precise_timer{granted}})
            == evidence::finest_timer_resolution_request_failed);

    check("A failed request for the finest timer resolution is chosen over a failed request for a precise timer",
          choose_sleep_timing_evidence({.finest_timer_resolution{failed}, .precise_timer{failed}})
            == evidence::finest_timer_resolution_request_failed);

    check("A granted request for the finest timer resolution is chosen over a granted request for a precise timer",
          choose_sleep_timing_evidence({.finest_timer_resolution{granted}, .precise_timer{granted}})
            == evidence::finest_timer_resolution_request_granted);
  }

  void coarse_sleeps_free_test::test_coarse_sleep_messages()
  {
    const double_precision_milliseconds target{5.0};
    const double_precision_milliseconds slept{15.2};

    auto checkMessage{
      [this, target, slept](const reporter& description, sleep_timing_evidence evidence, std::string_view fileName) {
        write_to_file(working_materials() /= fileName,
                      coarse_sleep_message(slept, target, evidence),
                      std::ios_base::out);
        check(equivalence, description, working_materials() /= fileName, predictive_materials() /= fileName);
      }
    };

    checkMessage("The request for the finest timer resolution failed",
                 sleep_timing_evidence::finest_timer_resolution_request_failed,
                 "FinestTimerResolutionRequestFailed.txt");

    checkMessage("A request for a precise timer failed",
                 sleep_timing_evidence::precise_timer_request_failed,
                 "PreciseTimerRequestFailed.txt");

    checkMessage("The request for the finest timer resolution was granted",
                 sleep_timing_evidence::finest_timer_resolution_request_granted,
                 "FinestTimerResolutionRequestGranted.txt");

    checkMessage("The requests for a precise timer were granted",
                 sleep_timing_evidence::precise_timer_request_granted,
                 "PreciseTimerRequestGranted.txt");

    checkMessage("No request is made",
                 sleep_timing_evidence::no_timer_request_made,
                 "NoTimerRequestMade.txt");
  }
}
