////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2026.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

import std;
import sequoia.test_framework;

namespace fakeProject::testing
{
    using namespace sequoia::testing;

    class container_performance_test final : public performance_test
    {
    public:
        using performance_test::performance_test;

        [[nodiscard]]
        std::filesystem::path source_file() const;

        void run_tests();
    };
}
