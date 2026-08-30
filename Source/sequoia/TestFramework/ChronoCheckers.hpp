////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Specializations of the class template value_tester for `std::chrono::time_point`.

    See ConcreteTypeCheckers.hpp for how nested checks dispatch, and for a single
    header pulling in every specialization at once.
 */

#include "sequoia/TestFramework/FreeCheckers.hpp"

#include <chrono>

namespace sequoia::testing
{
  /** \brief Compares instance of `std::chrono::time_point`

      For the advice to be invoked, `tutor` must be constructed by a function object
      with an overload `operator()(std::chrono::nanoseconds, std::chrono::nanoseconds)`.
   */

  template<class Clock, class Duration>
  struct value_tester<std::chrono::time_point<Clock, Duration>>
  {
    using type = std::chrono::time_point<Clock, Duration>;

    template<test_mode Mode, class Advisor>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& obtained, const type& prediction, const tutor<Advisor>& advisor)
    {
      using ns = std::chrono::nanoseconds;
      check(equality,
            "Time since epoch",
            logger,
            std::chrono::duration_cast<ns>(obtained.time_since_epoch()),
            std::chrono::duration_cast<ns>(prediction.time_since_epoch()), advisor);
    }
  };
}
