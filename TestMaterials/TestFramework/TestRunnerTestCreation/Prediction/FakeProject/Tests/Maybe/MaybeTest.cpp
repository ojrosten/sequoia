////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2025.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "MaybeTest.hpp"

namespace fakeProject::testing
{
    [[nodiscard]]
    std::filesystem::path maybe_test::source_file() const
    {
        return std::source_location::current().file_name();
    }

    void maybe_test::run_tests()
    {
        // For example:

        // other::functional::maybe<T> x{args}, y{different args};
        // check(equivalence, "Useful Description", x, something equivalent);
        // check(equivalence,"Useful Description", y, something equivalent);
        // For orderable type, with x < y:
        // check_semantics("Useful Description", x, y, std::weak_ordering::less);
        // For equality comparable but not orderable:
        // check_semantics("Useful Description", x, y);
    }
}
