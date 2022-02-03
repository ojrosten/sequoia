////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2022.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ProbabilityTest.hpp"

namespace sequoia::testing
{
    [[nodiscard]]
    std::string_view probability_test::source_file() const noexcept
    {
        return __FILE__;
    }

    void probability_test::run_tests()
    {
        // For example:

        // maths::probability x{args}, y{different args};
        // check(equivalence, LINE("Useful Description"), x, something equivalent);
        // check(equivalence,LINE("Useful Description"), y, something equivalent);
        // For orderable type, with x < y:
        // check_semantics(LINE("Useful Description"), x, y, std::weak_ordering::less);
        // For equality comparable but not orderable:
        // check_semantics(LINE("Useful Description"), x, y);
    }
}
