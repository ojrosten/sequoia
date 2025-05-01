////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class test_runner_false_negative_test final : public free_false_negative_test
  {
  public:
    using free_false_negative_test::free_false_negative_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:

    void test_template_data_generation();
  };
}
