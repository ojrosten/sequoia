////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2024.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "MultipleTestingUtilities.hpp"

namespace fakeProject::testing
{
    using namespace sequoia::testing;

    class multiple_test final : public move_only_test
    {
    public:
        using move_only_test::move_only_test;

        [[nodiscard]]
        std::filesystem::path source_file() const;

        void run_tests();
    };
}
