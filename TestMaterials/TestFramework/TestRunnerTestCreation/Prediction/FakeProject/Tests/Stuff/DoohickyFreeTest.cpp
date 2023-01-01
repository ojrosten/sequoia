////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2023.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DoohickyFreeTest.hpp"
#include "fakeProject/Stuff/Doohicky.hpp"

namespace sequoia::testing
{
    [[nodiscard]]
    std::string_view doohicky_free_test::source_file() const noexcept
    {
        return __FILE__;
    }

    void doohicky_free_test::run_tests()
    {
        // e.g. check(equality, LINE("Useful description"), some_function(), 42);
    }
}
