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
  class test_runner_test_creation final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const noexcept final;
  private:
    void run_tests() final;

    void test_type_handling();

    void test_template_data_generation();

    void test_creation();

    void test_creation_failure();

    [[nodiscard]]
    std::filesystem::path fake_project() const;

    [[nodiscard]]
    std::string zeroth_arg() const;
  };
}
