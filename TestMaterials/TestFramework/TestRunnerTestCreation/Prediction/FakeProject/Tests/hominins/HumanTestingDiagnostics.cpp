////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2024.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "HumanTestingDiagnostics.hpp"

namespace fakeProject::testing
{
    [[nodiscard]]
    std::filesystem::path human_false_negative_test::source_file() const
    {
        return std::source_location::current().file_name();
    }

    void human_false_negative_test::run_tests()
    {
        // For example:

        // human x{args}, y{different args};
        // check(equivalence, "Useful Description", x, something inequivalent - ordinarily this would fail);
        // check(equality, "Useful Description", x, y);
    }
}
