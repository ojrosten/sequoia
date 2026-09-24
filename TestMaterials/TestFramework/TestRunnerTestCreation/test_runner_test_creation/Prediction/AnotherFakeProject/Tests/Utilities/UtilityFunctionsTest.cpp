////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2026.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UtilityFunctionsTest.hpp"
#include "curlew/Utilities/Utilities.h"

namespace curlew::testing
{
    [[nodiscard]]
    std::filesystem::path utility_functions_test::source_file()
    {
        return std::source_location::current().file_name();
    }

    void utility_functions_test::run_tests()
    {
        // e.g. check(equality, "Useful description", some_function(), 42);
    }
}
