////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class test_runner_reported_durations_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    /** Serial, since other tests running concurrently inflate the durations
        which its checks bound
     */
    using parallelizable_type = std::false_type;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:
    void test_runner_overhead_reported_apart();

    void test_execution_duration_of_busiest_thread();

    void check_output(std::string_view description, std::string_view dirName, const std::stringstream& output);
  };
}
