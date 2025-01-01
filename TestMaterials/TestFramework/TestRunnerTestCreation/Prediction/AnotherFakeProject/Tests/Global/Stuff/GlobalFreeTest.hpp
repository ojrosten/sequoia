////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2025.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace curlew::testing
{
    using namespace sequoia::testing;

    class global_free_test final : public free_test
    {
    public:
        using free_test::free_test;

        [[nodiscard]]
        std::filesystem::path source_file() const;

        void run_tests();
    };
}
