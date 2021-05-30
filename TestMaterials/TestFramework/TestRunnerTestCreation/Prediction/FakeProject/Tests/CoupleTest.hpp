////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "CoupleTestingUtilities.hpp"

namespace sequoia::testing
{
    class couple_test final : public regular_test
    {
    public:
        using regular_test::regular_test;

    private:
        [[nodiscard]]
        std::string_view source_file() const noexcept final;

        void run_tests() final;
    };
}
