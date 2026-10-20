////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/CoarseSleeps.hpp"
#include "sequoia/Parsing/CommandLineMessages.hpp"

#include <format>
#include <stdexcept>

namespace sequoia::testing
{
  namespace
  {
    [[nodiscard]]
    std::string evidence_message(sleep_timing_evidence evidence)
    {
      switch(evidence)
      {
      case sleep_timing_evidence::finest_timer_resolution_request_failed:
        return "The likely cause is that the request for the finest timer resolution failed";
      case sleep_timing_evidence::precise_timer_request_failed:
        return "The likely cause is that a request for a precise timer failed, "
               "so a sleep fell back to a plain sleep";
      case sleep_timing_evidence::finest_timer_resolution_request_granted:
        return "Windows granted the request for the finest timer resolution";
      case sleep_timing_evidence::precise_timer_request_granted:
        return "The kernel accepted every request for a precise timer";
      case sleep_timing_evidence::finest_timer_resolution_request_not_observed:
        return "The outcome of the request for the finest timer resolution was not observed";
      case sleep_timing_evidence::precise_timer_request_not_observed:
        return "The outcome of a request for a precise timer was not observed";
      case sleep_timing_evidence::no_timer_request_made:
        return "No request for a timer finer than the default was made";
      }

      throw std::logic_error{"Unhandled sleep_timing_evidence"};
    }
  }

  [[nodiscard]]
  bool is_coarse_sleep(double_precision_milliseconds slept, double_precision_milliseconds target)
  {
    return slept >= 2 * target;
  }

  [[nodiscard]]
  timer_request_outcome combine_timer_request_outcomes(timer_request_outcome lhs, timer_request_outcome rhs)
  {
    if((lhs == timer_request_outcome::failed) || (rhs == timer_request_outcome::failed))
      return timer_request_outcome::failed;

    if((lhs == timer_request_outcome::granted) || (rhs == timer_request_outcome::granted))
      return timer_request_outcome::granted;

    if((lhs == timer_request_outcome::not_observed) || (rhs == timer_request_outcome::not_observed))
      return timer_request_outcome::not_observed;

    return timer_request_outcome::not_made;
  }

  [[nodiscard]]
  sleep_timing_evidence choose_sleep_timing_evidence(outcomes_of_timer_requests outcomes)
  {
    if(outcomes.finest_timer_resolution == timer_request_outcome::failed)
      return sleep_timing_evidence::finest_timer_resolution_request_failed;

    if(outcomes.precise_timer == timer_request_outcome::failed)
      return sleep_timing_evidence::precise_timer_request_failed;

    if(outcomes.finest_timer_resolution == timer_request_outcome::granted)
      return sleep_timing_evidence::finest_timer_resolution_request_granted;

    if(outcomes.precise_timer == timer_request_outcome::granted)
      return sleep_timing_evidence::precise_timer_request_granted;

    if(outcomes.finest_timer_resolution == timer_request_outcome::not_observed)
      return sleep_timing_evidence::finest_timer_resolution_request_not_observed;

    if(outcomes.precise_timer == timer_request_outcome::not_observed)
      return sleep_timing_evidence::precise_timer_request_not_observed;

    return sleep_timing_evidence::no_timer_request_made;
  }

  [[nodiscard]]
  std::string coarse_sleep_message(double_precision_milliseconds slept,
                                   double_precision_milliseconds target,
                                   sleep_timing_evidence evidence)
  {
    using parsing::commandline::warning;
    return warning({std::format("Sleeps of {:.1f} ms repeatedly lasted {:.1f} ms or more, "
                                "so timings built on sleeps are unreliable",
                                target.count(),
                                slept.count()),
                    evidence_message(evidence)});
  }
}
