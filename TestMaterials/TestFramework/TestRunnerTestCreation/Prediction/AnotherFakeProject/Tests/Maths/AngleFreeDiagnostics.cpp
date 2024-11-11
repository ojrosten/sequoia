////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2024.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AngleFreeDiagnostics.hpp"
#include "curlew/Maths/Angle.hpp"

namespace curlew::testing
{
    [[nodiscard]]
    std::filesystem::path angle_false_negative_free_diagnostics::source_file() const
    {
        return std::source_location::current().file_name();
    }

    void angle_false_negative_free_diagnostics::run_tests()
    {
        // e.g. check(equality, "Useful description", some_function(), 42);
    }

    [[nodiscard]]
    std::filesystem::path angle_false_negative_free_diagnostics::source_file() const
    {
        return std::source_location::current().file_name();
    }

    void angle_false_negative_free_diagnostics::run_tests()
    {
        // e.g. check(equality, "Useful description", some_function(), 42);
    }
}
