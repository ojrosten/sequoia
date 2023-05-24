////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2023.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ThingummyTestingDiagnostics.hpp"

namespace sequoia::testing
{
    [[nodiscard]]
    std::filesystem::path thingummy_false_positive_test::source_file() const
    {
        return std::source_location::current().file_name();
    }

    void thingummy_false_positive_test::run_tests()
    {
        // For example:

        // stuff::thingummy<T> x{args}, y{different args};
        // check(equivalence, report_line("Useful Description"), x, something inequivalent - ordinarily this would fail);
        // check(equality, report_line("Useful Description"), x, y);
    }
}
