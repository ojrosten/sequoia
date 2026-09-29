////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2026.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UtilitiesFreeTestExtras.hpp"
#include "fakeProject/Utilities/Utilities.h"

namespace fakeProject::testing
{
    [[nodiscard]]
    std::filesystem::path utilities_free_test_extras::source_file()
    {
        return std::source_location::current().file_name();
    }

    void utilities_free_test_extras::run_tests()
    {
        // e.g. check(equality, "Useful description", some_function(), 42);
    }
}
