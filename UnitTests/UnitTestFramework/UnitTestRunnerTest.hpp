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
    template<std::size_t... M>
    commandline_arguments(char const(&...args)[Ns])
      : m_Args{(char*)args...}
    {}

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
}
