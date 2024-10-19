////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2024.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DoohickyFreeTest.hpp"
#include "fakeProject/Stuff/Doohicky.hpp"

namespace fakeProject::testing
{
    [[nodiscard]]
    std::filesystem::path doohicky_free_test::source_file() const
    {
        return std::source_location::current().file_name();
    }

    void doohicky_free_test::run_tests()
    {
        // e.g. check(equality, "Useful description", some_function(), 42);
    }
}
