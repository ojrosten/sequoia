////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2025.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/MoveOnlyTestCore.hpp"
#include "curlew/Stuff/Foo.h"

namespace sequoia::testing
{
    template<maths::floating_point T>
    struct value_tester<bar::baz::foo<T>>
    {
        using type = bar::baz::foo<T>;

        template<test_mode Mode>
        static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
        {
            // e.g. check(equality, "Description", logger, actual.some_method(), prediction.some_method());
        }
        
        template<test_mode Mode>
        static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const T& prediction)
        {
            // e.g. check(equality, "Description", logger, actual.some_method(), prediction);
        }
    };
}
