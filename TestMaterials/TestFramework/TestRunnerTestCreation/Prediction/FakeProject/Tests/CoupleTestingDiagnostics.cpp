////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2021.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CoupleTestingDiagnostics.hpp"

namespace sequoia::testing
{
    [[nodiscard]]
    std::string_view couple_false_positive_test::source_file() const noexcept
    {
        return __FILE__;
    }

    void couple_false_positive_test::run_tests()
    {
        // For example:

        // other::couple<S, T> x{args}, y{different args};
        // check_equivalence(LINE("Useful Description"), x, something inequivalent - ordinarily this would fail);
        // check_equality(LINE("Useful Description"), x, y);
    }
}
