////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Core/Functional/ErasedFunction.hpp"

#include <optional>

namespace sequoia::testing
{
  /** \brief An `erased_function` made regular by its observable behaviour: two are equal when both are
      empty, or both are engaged and invoking them gives equal results.
   */
  struct observed_function
  {
    erased_function<int() const> function;

    [[nodiscard]]
    friend bool operator==(const observed_function& lhs, const observed_function& rhs)
    {
      return (static_cast<bool>(lhs.function) == static_cast<bool>(rhs.function))
          && (!lhs.function || (lhs.function() == rhs.function()));
    }
  };

  template<> struct value_tester<observed_function>
  {
    using type = observed_function;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      check(equality, "Engaged", logger, static_cast<bool>(actual.function), static_cast<bool>(prediction.function));
      if(actual.function && prediction.function)
        check(equality, "Result", logger, actual.function(), prediction.function());
    }

    template<test_mode Mode>
    static void test(equivalence_check_t,
                     test_logger<Mode>& logger,
                     const type& actual,
                     const std::optional<int>& prediction)
    {
      check(equality, "Engaged", logger, static_cast<bool>(actual.function), prediction.has_value());
      if(actual.function && prediction)
        check(equality, "Result", logger, actual.function(), *prediction);
    }
  };
}
