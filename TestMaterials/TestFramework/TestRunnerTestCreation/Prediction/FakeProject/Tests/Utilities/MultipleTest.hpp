////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2022.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "MultipleTestingUtilities.hpp"

namespace sequoia::testing
{
    class multiple_test final : public move_only_test
    {
    public:
        using move_only_test::move_only_test;

    private:
        [[nodiscard]]
        std::string_view source_file() const noexcept final;

        void run_tests() final;
    };
}
