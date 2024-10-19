////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2024.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "FooTest.hpp"

namespace curlew::testing
{
    [[nodiscard]]
    std::filesystem::path foo_test::source_file() const
    {
        return std::source_location::current().file_name();
    }

    void foo_test::run_tests()
    {
        // For example:

        // auto x = []() { return bar::baz::foo<T>{args}; };
        // auto y = []() { return bar::baz::foo<T>{different args}; };
        // check(equivalence, "Useful Description", x(), something equivalent);
        // check(equivalence, "Useful Description", y(), something equivalent);
        // For orderable type, with x < y:
        // check_semantics("Useful Description", x, y, std::weak_ordering::less);
        // For equality comparable but not orderable:
        // check_semantics("Useful Description", x, y);
    }
}
