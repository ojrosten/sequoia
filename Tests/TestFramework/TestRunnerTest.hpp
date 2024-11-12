////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class test_runner;

  class test_runner_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:

    void test_exceptions();

    void test_critical_errors();

    void test_filtered_suites();

    void test_basic_output();

    void test_verbose_output();

    void test_serial_verbose_output();

    void test_throwing_tests();

    void test_prune_basic_output();

    void test_nested_suite();

    void test_nested_suite_verbose();

    void test_instability_analysis();

    template<std::invocable<test_runner&> Manipulator, concrete_test... Ts>
    void test_instability_analysis(std::string_view message,
                                   std::string_view outputDirName,
                                   std::string_view numRuns,
                                   std::initializer_list<std::string_view> extraArgs,
                                   Manipulator manipulator,
                                   Ts&&... ts);

    template<concrete_test... Ts>
    void test_instability_analysis(std::string_view message,
                                   std::string_view outputDirName,
                                   std::string_view numRuns,
                                   std::initializer_list<std::string_view> extraArgs,
                                   Ts&&... ts);

    template<concrete_test... Ts>
    void test_instability_analysis(std::string_view message,
                                   std::string_view outputDirName,
                                   std::string_view numRuns,
                                   Ts&&... ts);

    [[nodiscard]]
    std::filesystem::path fake_project() const;

    [[nodiscard]]
    std::filesystem::path minimal_fake_path() const;

    [[nodiscard]]
    std::string zeroth_arg() const;

    std::filesystem::path write(std::string_view dirName, std::stringstream& output) const;

    std::filesystem::path check_output(reporter description, std::string_view dirName, std::stringstream& output);
  };
}
