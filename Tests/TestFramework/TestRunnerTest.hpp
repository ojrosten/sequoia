////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class test_runner_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    void test_name_and_source_duplication();

    void test_critical_errors();

    void test_instability_analysis();

    [[nodiscard]]
    std::filesystem::path aux_project() const;
  };
}
