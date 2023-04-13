////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2023.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "AngleTestingUtilities.hpp"

namespace sequoia::testing
{
    class angle_false_positive_test final : public regular_false_positive_test
    {
    public:
        using regular_false_positive_test::regular_false_positive_test;

    private:
        [[nodiscard]]
        std::filesystem::path source_file() const noexcept final;

        void run_tests() final;
    };
}
