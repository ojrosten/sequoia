////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2024.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "VariadicTestingDiagnostics.hpp"

namespace sequoia::testing
{
    [[nodiscard]]
    std::filesystem::path variadic_false_positive_test::source_file() const
    {
        return std::source_location::current().file_name();
    }

    void variadic_false_positive_test::run_tests()
    {
        // For example:

        // auto x = []() { return variadic<T...>{args}; };
        // auto y = []() { return variadic<T...>{different args}; };
        // check(equivalence, report_line("Useful Description"), x(), something inequivalent - ordinarily this would fail);
        // check(equality, report_line("Useful Description"), x(), y());
    }
}
