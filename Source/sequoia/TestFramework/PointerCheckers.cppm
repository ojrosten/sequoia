////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

export module sequoia.test_framework:PointerCheckers;

import std;

import :Advice;
import :BinaryRelationships;
import :CoreInfrastructure;
import :FailureInfo;
import :FreeCheckers;
import :Output;
import :ProjectPaths;
import :TestLogger;
import :TestMode;
export import sequoia.core.meta;
export import sequoia.platform_specific;
export import sequoia.text_processing;

/** \file
    \brief A specialization of the class template value_tester for raw pointers.

    Raw pointers are elementary enough that every test can be expected to compare them, so this
    is one of the few tester headers pulled in by FreeTestCore.hpp. The smart pointers, which
    are not, live in SmartPointerCheckers.hpp.
 */

export namespace sequoia::testing
{
  /** \brief Compares instance of pointers

     Testing equality is performed via `binary_comparison`, and so does not require this
     specialization of `value_tester`.

     The `test(equivalence_check_t,...)` function checks whether the pointers either both point to
     something or both point to nullptr, reporting a failure if this is not the case. If both
     pointers are not null, a check is dispatched to test the bound type. This is done using
     the strongest available check.
  */

  template<class T>
  struct value_tester<T*>
  {
    using type = T*;

    template<test_mode Mode, class Advisor>
    static void test(equivalence_check_t, test_logger<Mode>& logger, type obtained, type prediction, const tutor<Advisor>& advisor)
    {
      if(obtained && prediction)
      {
        check(with_best_available, "Pointees differ", logger, *obtained, *prediction, advisor);
      }
      else
      {
        const auto obtainedIsNull{static_cast<bool>(obtained)}, predictionIsNull{static_cast<bool>(prediction)};

        check(equality,
              nullable_type_message(obtainedIsNull, predictionIsNull),
              logger,
              obtainedIsNull,
              predictionIsNull);
      }
    }
  };
}
