////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2022.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "fakeProject/Stuff/Container.hpp"

namespace sequoia::testing
{
    template<class T>
    struct value_tester<container<T>>
    {
        using type = container<T>;

        template<test_mode Mode>
        static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
        {
            // e.g. check_equality("Description", logger, actual.method(), prediction.method());
        }
        
        template<test_mode Mode>
        static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const std::vector<T>& prediction)
        {
            // e.g. check_equality("Description", logger, actual.method(), prediction);
        }
    };
}
