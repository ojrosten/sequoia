////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

#include "UnitTestRunner.hpp"

namespace sequoia::unit_testing
{
  template<>
  struct weak_equivalence_checker<commandline_operation>
  {
    using type = commandline_operation;
    using weak_type = std::vector<std::string>;

    template<class Logger>
    static void check(std::string_view description, Logger& logger, const type& operations, const weak_type& prediction)
    {
      check_equality(description, logger, operations.parameters, prediction);
    }
  };
  
  template<>
  struct weak_equivalence_checker<std::vector<commandline_operation>>
  {
    using type = std::vector<commandline_operation>;
    using weak_type = std::vector<std::vector<std::string>>;

    template<class Logger>
    static void check(std::string_view description, Logger& logger, const type& operations, const weak_type& prediction)
    {
      check_range_weak_equivalence(description, logger, std::begin(operations), std::end(operations), std::begin(prediction), std::end(prediction));
    }
  };
  
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
