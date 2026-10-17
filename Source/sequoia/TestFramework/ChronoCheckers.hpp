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
  /** \brief Compares instances of `std::chrono::time_point` by their times
             since their clock's epoch, in the time point's own duration.

      An advisor receives the obtained and predicted times since the clock's
      epoch. An advisor whose only call operator takes
      `std::chrono::nanoseconds` is also used when
      `std::common_type_t<std::chrono::nanoseconds, Duration>` is
      `std::chrono::nanoseconds`. Converting a time more than
      about 292 years from the epoch to nanoseconds overflows. Under MSVC,
      `std::chrono::file_clock`'s epoch is 1601, so converting any
      present-day instance of a `std::filesystem::file_time_type` overflows.
   */

  template<class Clock, class Duration>
  struct value_tester<std::chrono::time_point<Clock, Duration>>
  {
    using type = std::chrono::time_point<Clock, Duration>;

    template<test_mode Mode, class Advisor>
    static void test(equality_check_t,
                     test_logger<Mode>& logger,
                     const type& obtained,
                     const type& prediction,
                     const tutor<Advisor>& advisor)
    {
      check(equality, "Time since epoch", logger, obtained.time_since_epoch(), prediction.time_since_epoch(), advisor);
    }
  };
}
