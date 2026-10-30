////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module sequoia.test_framework;

import std;

import sequoia.parsing;

namespace sequoia::testing
{
  namespace
  {
    [[nodiscard]]
    std::string finest_timer_resolution_message(timer_request_outcome outcome)
    {
      switch(outcome)
      {
      case timer_request_outcome::failed:
        return "The likely cause is that the request for the finest timer resolution failed";
      case timer_request_outcome::granted:
        return "Windows granted the request for the finest timer resolution";
      case timer_request_outcome::not_observed:
        return "The outcome of the request for the finest timer resolution was not observed";
      case timer_request_outcome::not_made:
        return "No request for the finest timer resolution was made";
      }

      throw std::logic_error{"Unhandled timer_request_outcome"};
    }

    [[nodiscard]]
    std::string precise_timer_message(timer_request_outcome outcome)
    {
      switch(outcome)
      {
      case timer_request_outcome::failed:
        return "The likely cause is that a request for a precise timer failed, "
               "so a sleep fell back to a plain sleep";
      case timer_request_outcome::granted:
        return "The kernel accepted every request for a precise timer";
      case timer_request_outcome::not_observed:
        return "The outcome of a request for a precise timer was not observed";
      case timer_request_outcome::not_made:
        return "No request for a precise timer was made";
      }

      throw std::logic_error{"Unhandled timer_request_outcome"};
    }

    [[nodiscard]]
    std::string outcome_message(timer_request request, timer_request_outcome outcome)
    {
      switch(request)
      {
      case timer_request::none:
        if(outcome != timer_request_outcome::not_made)
          throw std::logic_error{"An outcome other than not_made, for a platform which makes no timer request"};

        return "This platform makes no request for more precise sleeps";
      case timer_request::finest_timer_resolution:
        return finest_timer_resolution_message(outcome);
      case timer_request::precise_timer:
        return precise_timer_message(outcome);
      }

      throw std::logic_error{"Unhandled timer_request"};
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
  std::string coarse_sleep_message(double_precision_milliseconds slept,
                                   double_precision_milliseconds target,
                                   timer_request request,
                                   timer_request_outcome outcome)
  {
    using parsing::commandline::warning;
    return warning({std::format("Sleeps of {:.1f} ms repeatedly lasted {:.1f} ms or more, "
                                "so timings built on sleeps are unreliable",
                                target.count(),
                                slept.count()),
                    outcome_message(request, outcome)});
  }
}
