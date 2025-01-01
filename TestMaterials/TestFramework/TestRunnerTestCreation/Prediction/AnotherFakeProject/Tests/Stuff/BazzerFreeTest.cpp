////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2025.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "BazzerFreeTest.hpp"
#include "curlew/Stuff/Baz.h"

namespace curlew::testing
{
    [[nodiscard]]
    std::filesystem::path bazzer_free_test::source_file() const
    {
        return std::source_location::current().file_name();
    }

    void bazzer_free_test::run_tests()
    {
        // e.g. check(equality, "Useful description", some_function(), 42);
    }
}
