////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/PerformanceTestCore.hpp"

namespace sequoia::testing
{
  class test_runner_performance_test final : public performance_test
  {
  public:
    using performance_test::performance_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:

    void test_parallel_acceleration();

    void test_thread_pool_acceleration();

    void test_serial_execution();

    void test_recording_of_performance_check_attempts();

    void test_summary_of_performance_checks_within_other_checks();

    void check_speedup_over_serial_execution(std::string_view description,
                                             const std::vector<std::string>& arguments,
                                             relative_performance_interval prediction);

    [[nodiscard]]
    std::filesystem::path fake_project() const;

    [[nodiscard]]
    std::filesystem::path minimal_fake_path() const;

    void write(std::string_view dirName, std::string_view printedOutput) const;

    void check_output(std::string_view description,
                      std::string_view dirName,
                      const std::stringstream& output);
  };
}
