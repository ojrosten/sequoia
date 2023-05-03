////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2023.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "ContainerTestingUtilities.hpp"

namespace sequoia::testing
{
    class container_test final : public regular_test
    {
    public:
        using regular_test::regular_test;

        [[nodiscard]]
        std::filesystem::path source_file() const noexcept;

        void run_tests();
    };
}
