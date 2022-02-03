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
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    void test_exceptions();

    void test_critical_errors();

    void test_basic_output();

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
    std::filesystem::path aux_project() const;
  };
}
