////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2022.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "BazagainFreeTest.hpp"
#include "fakeProject/Stuff/Baz.h"

namespace sequoia::testing
{
    [[nodiscard]]
    std::string_view bazagain_free_test::source_file() const noexcept
    {
        return __FILE__;
    }

    void bazagain_free_test::run_tests()
    {
        // e.g. check(equality, LINE("Useful description"), some_function(), 42);
    }
}