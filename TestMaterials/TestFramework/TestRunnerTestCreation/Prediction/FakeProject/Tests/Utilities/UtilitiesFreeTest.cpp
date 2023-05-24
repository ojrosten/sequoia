////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2023.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "UtilitiesFreeTest.hpp"
#include "fakeProject/Utilities/Utilities.h"

namespace sequoia::testing
{
    [[nodiscard]]
    std::filesystem::path utilities_free_test::source_file() const
    {
        return std::source_location::current().file_name();
    }

    void utilities_free_test::run_tests()
    {
        // e.g. check(equality, report_line("Useful description"), some_function(), 42);
    }
}
