////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2022.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "WidgetTestingDiagnostics.hpp"

namespace sequoia::testing
{
    [[nodiscard]]
    std::string_view widget_false_positive_test::source_file() const noexcept
    {
        return __FILE__;
    }

    void widget_false_positive_test::run_tests()
    {
        // For example:

        // stuff::widget x{args}, y{different args};
        // check_equivalence(LINE("Useful Description"), x, something inequivalent - ordinarily this would fail);
        // check_equality(LINE("Useful Description"), x, y);
    }
}
