////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2024.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "FooTestingDiagnostics.hpp"

namespace fakeProject::testing
{
    [[nodiscard]]
    std::filesystem::path foo_false_negative_test::source_file() const
    {
        return std::source_location::current().file_name();
    }

    void foo_false_negative_test::run_tests()
    {
        // For example:

        // auto x = []() { return bar::baz::foo<T>{args}; };
        // auto y = []() { return bar::baz::foo<T>{different args}; };
        // check(equivalence, "Useful Description", x(), something inequivalent - ordinarily this would fail);
        // check(equality, "Useful Description", x(), y());
    }
}
