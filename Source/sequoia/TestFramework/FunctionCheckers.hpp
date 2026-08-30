////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Specializations of the class template value_tester for `std::function`.

    See ConcreteTypeCheckers.hpp for how nested checks dispatch, and for a single
    header pulling in every specialization at once.
 */

#include "sequoia/TestFramework/FreeCheckers.hpp"

#include <functional>

namespace sequoia::testing
{
  /** \brief Provides a `weak_equivalence` test for `std::function`

      Two instances of `std::function<R (Args...)>` are taken to be weakly equivalent
      if they are either both null or both not null.
   */
  template<class R, class... Args>
  struct value_tester<std::function<R (Args...)>>
  {
    using type = std::function<R (Args...)>;

    template<test_mode Mode>
    static void test(weak_equivalence_check_t, test_logger<Mode>& logger, const type& obtained, const type& prediction)
    {
      const bool obtainedIsNull{obtained}, predictionIsNull{prediction};

      check(nullable_type_message(obtainedIsNull, predictionIsNull),
            logger,
            (obtainedIsNull && predictionIsNull) || (!obtainedIsNull && !predictionIsNull));
    }
  };

}
