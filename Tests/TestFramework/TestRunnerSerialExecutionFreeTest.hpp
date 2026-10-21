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
  class test_runner_serial_execution_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:
    void test_slow_test_runner_constraints();

    void test_reversed_and_empty_sleep_intervals();

    void test_index_beyond_registered_tests();

    void test_separation_of_slow_test_families();

    void test_serial_execution();

    void check_output(std::string_view description, std::string_view dirName, const std::stringstream& output);
  };
}
