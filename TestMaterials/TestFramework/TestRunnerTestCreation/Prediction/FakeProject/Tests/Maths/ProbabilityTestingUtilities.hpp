////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2022.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "fakeProject/Maths/Probability.hpp"

namespace sequoia::testing
{
    template<> struct value_tester<maths::probability>
    {
        using type = maths::probability;

        template<test_mode Mode>
        static void test_equality(test_logger<Mode>& logger, const type& actual, const type& prediction)
        {
            // e.g. check_equality("Description", logger, actual.method(), prediction.method());
        }
        
        template<test_mode Mode>
        static void test_equivalence(test_logger<Mode>& logger, const type& actual, const double prediction)
        {
            // e.g. check_equality("Description", logger, actual.method(), prediction);
        }
    };
}
