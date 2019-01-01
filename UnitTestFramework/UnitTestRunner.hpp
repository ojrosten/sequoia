////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file UnitTestRunner.hpp
    \brief Utility for running unit tests from the command line.
*/

#include "UnitTestUtils.hpp"

namespace sequoia::unit_testing
{
  class unit_test_runner
  {
  public:
    unit_test_runner(int argc, char** argv);

    unit_test_runner(const unit_test_runner&) = delete;
    unit_test_runner(unit_test_runner&&)      = default;

    void add_test_family(test_family&& f);

    void execute();
  private:
    std::vector<test_family> m_Families;
    std::set<std::string> m_SpecificCases{};
    bool m_Asynchronous{};
  };
}
