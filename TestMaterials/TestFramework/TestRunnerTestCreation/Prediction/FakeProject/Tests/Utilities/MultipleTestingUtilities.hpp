////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/MoveOnlyTestCore.hpp"
#include "fakeProject/Utilities/Multiple.hpp"

namespace sequoia::testing
{
    template<class... T>
  struct detailed_equality_checker<multiple<T...>>
    {
        using type = multiple<T...>;

        template<test_mode Mode>
        static void check(test_logger<Mode>& logger, const type& actual, const type& prediction)
        {
            // e.g. check_equality("Description", logger, actual.method(), prediction.method());
        }
    };

    template<class... T>
  struct equivalence_checker<multiple<T...>>
    {
        using type = multiple<T...>;

        template<test_mode Mode>
        static void check(test_logger<Mode>& logger, const type& actual, const std::tuple<T...>& prediction)
        {
            // e.g. check_equality("Description", logger, actual.method(), predictions.foo());
        }
    };
}
