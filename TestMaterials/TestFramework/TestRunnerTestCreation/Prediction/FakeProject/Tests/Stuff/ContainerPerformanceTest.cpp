////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2023.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ContainerPerformanceTest.hpp"
#include "fakeProject/Stuff/Container.hpp"

namespace sequoia::testing
{
    [[nodiscard]]
    std::string_view container_performance_test::source_file() const noexcept
    {
        return __FILE__;
    }

    void container_performance_test::run_tests()
    {
        // e.g. check(equality, report_line("Useful description"), some_function(), 42);
    }
}
