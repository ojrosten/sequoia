////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "IteratorTest.hpp"

namespace sequoia::testing
{
    [[nodiscard]]
    std::string_view iterator_test::source_file() const noexcept
    {
        return __FILE__;
    }

    void iterator_test::run_tests()
    {
        // e.g iterator x{args}, y{different args};
        // check_equivalence(LINE("Useful Description"), x, something equivalent);
        // check_equivalence(LINE("Useful Description"), y, something equivalent);
        // check_semantics(LINE("Useful Description"), x, y);
    }
}
