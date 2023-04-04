////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/PerformanceTestCore.hpp"

namespace sequoia::testing
{
  class test_runner_performance_test final : public performance_test
  {
  public:
    using performance_test::performance_test;

  private:
    [[nodiscard]]
    std::string_view source_file() const noexcept final;

    void run_tests() final;

    void test_parallel_acceleration();

    void test_thread_pool_acceleration();

    void test_serial_execution();

    [[nodiscard]]
    std::filesystem::path fake_project() const;

    std::filesystem::path write(std::string_view dirName, std::stringstream& output) const;

    std::filesystem::path check_output(std::string_view description, std::string_view dirName, std::stringstream& output);
  };
}
