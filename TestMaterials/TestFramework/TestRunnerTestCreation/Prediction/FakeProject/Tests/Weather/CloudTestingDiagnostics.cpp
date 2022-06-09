////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2022.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "CloudTestingDiagnostics.hpp"

namespace sequoia::testing
{
    [[nodiscard]]
    std::string_view cloud_false_positive_test::source_file() const noexcept
    {
        return __FILE__;
    }

    void cloud_false_positive_test::run_tests()
    {
        // For example:

        // auto x = []() { return cloud{args}; };
        // auto y = []() { return cloud{different args}; };
        // check(equivalence, LINE("Useful Description"), x(), something inequivalent - ordinarily this would fail);
        // check(equality, LINE("Useful Description"), x(), y());
    }
}