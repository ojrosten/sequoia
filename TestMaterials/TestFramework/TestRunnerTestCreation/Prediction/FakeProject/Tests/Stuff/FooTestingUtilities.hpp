////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2022.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/MoveOnlyTestCore.hpp"
#include "fakeProject/Stuff/Foo.h"

namespace sequoia::testing
{
    template<maths::floating_point T>
    struct value_checker<bar::baz::foo<T>>
    {
        using type = bar::baz::foo<T>;

        template<test_mode Mode>
        static void check(test_logger<Mode>& logger, const type& actual, const type& prediction)
        {
            // e.g. check_equality("Description", logger, actual.method(), prediction.method());
        }
    };

    template<maths::floating_point T>
    struct equivalence_checker<bar::baz::foo<T>>
    {
        using type = bar::baz::foo<T>;

        template<test_mode Mode>
        static void check(test_logger<Mode>& logger, const type& actual, const T& prediction)
        {
            // e.g. check_equality("Description", logger, actual.method(), prediction);
        }
    };
}
