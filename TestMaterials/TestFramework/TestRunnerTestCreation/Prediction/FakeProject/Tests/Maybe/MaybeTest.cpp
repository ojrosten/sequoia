////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2023.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "MaybeTest.hpp"

namespace sequoia::testing
{
    [[nodiscard]]
    std::filesystem::path maybe_test::source_file() const noexcept
    {
        return std::source_location::current().file_name();
    }

    void maybe_test::run_tests()
    {
        // For example:

        // other::functional::maybe<T> x{args}, y{different args};
        // check(equivalence, report_line("Useful Description"), x, something equivalent);
        // check(equivalence,report_line("Useful Description"), y, something equivalent);
        // For orderable type, with x < y:
        // check_semantics(report_line("Useful Description"), x, y, std::weak_ordering::less);
        // For equality comparable but not orderable:
        // check_semantics(report_line("Useful Description"), x, y);
    }
}
