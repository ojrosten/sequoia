////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2023.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "FooTestingDiagnostics.hpp"

namespace sequoia::testing
{
    [[nodiscard]]
    std::string_view foo_false_positive_test::source_file() const noexcept
    {
        return __FILE__;
    }

    void foo_false_positive_test::run_tests()
    {
        // For example:

        // auto x = []() { return bar::baz::foo<T>{args}; };
        // auto y = []() { return bar::baz::foo<T>{different args}; };
        // check(equivalence, report_line("Useful Description"), x(), something inequivalent - ordinarily this would fail);
        // check(equality, report_line("Useful Description"), x(), y());
    }
}
