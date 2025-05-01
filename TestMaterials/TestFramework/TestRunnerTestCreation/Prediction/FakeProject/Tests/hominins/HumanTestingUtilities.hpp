////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2025.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "fakeProject/hominins/Human.hpp"

namespace sequoia::testing
{
    template<> struct value_tester<human>
    {
        using type = human;

        template<test_mode Mode>
        static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
        {
            // e.g. check(equality, "Description", logger, actual.some_method(), prediction.some_method());
        }
        
        template<test_mode Mode>
        static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const std::string& prediction)
        {
            // e.g. check(equality, "Description", logger, actual.some_method(), prediction);
        }
    };
}
