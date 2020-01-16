////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

namespace sequoia::unit_testing
{
  template<std::size_t... Ns>
  class commandline_arguments
  {
  public:
    commandline_arguments(char const(&...args)[Ns])
      : m_Args{(char*)args...}
    {}

    [[nodiscard]]
    char** get() noexcept { return &m_Args[0]; }
  private:
    std::array<char*, sizeof...(Ns)> m_Args;
  };
  
  class unit_test_runner_test : public unit_test
  {
  public:
    using unit_test::unit_test;
  private:
    void run_tests() override;

    void test_parser();
  };

  class unit_test_runner_false_positive_test : public false_positive_test
  {
  public:
    using false_positive_test::false_positive_test;
  private:
    void run_tests() override;

    void test_parser();
  };
}
