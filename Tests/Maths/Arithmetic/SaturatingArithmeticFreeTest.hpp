////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  template<std::derived_from<free_test> Test>
  class saturating_arithmetic_free_test : public Test
  {
  public:
    using Test::Test;

    void run_tests()
    {
      using long_t = std::conditional_t<(sizeof(long) > sizeof(int)), long, long long>;

      Test::template execute_tests<double, double>();
      Test::template execute_tests<float, double>();
      Test::template execute_tests<double, float>();
      
      Test::template execute_tests<int, int>();
      Test::template execute_tests<unsigned, unsigned>();
      Test::template execute_tests<unsigned, long_t>();
      Test::template execute_tests<long_t, unsigned>();
      
      Test::template execute_tests<double, int>();
      Test::template execute_tests<int, double>();
      Test::template execute_tests<double, unsigned>();
      Test::template execute_tests<unsigned, double>();
      // TO DO
      /*Test::template execute_tests<float, int>();
      Test::template execute_tests<int, float>();
      Test::template execute_tests<float, unsigned>();
      Test::template execute_tests<unsigned, float>();
      */
    }
  protected:
    saturating_arithmetic_free_test(saturating_arithmetic_free_test&&) noexcept = default;

    saturating_arithmetic_free_test& operator=(saturating_arithmetic_free_test&&) noexcept = default;

    ~saturating_arithmetic_free_test() = default;    
  };

  class saturating_mul_test_base : public free_test
  {
  public:
    using free_test::free_test;

  protected:
    template<arithmetic T, arithmetic U>
    void execute_tests();
  };

  class saturating_add_test_base : public free_test
  {
  public:
    using free_test::free_test;

  protected:
    template<arithmetic T, arithmetic U>
    void execute_tests();
  };

  class saturating_mul_free_test : public saturating_arithmetic_free_test<saturating_mul_test_base>
  {
  public:
    using saturating_arithmetic_free_test<saturating_mul_test_base>::saturating_arithmetic_free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;
  };

  class saturating_add_free_test : public saturating_arithmetic_free_test<saturating_add_test_base>
  {
  public:
    using saturating_arithmetic_free_test<saturating_add_test_base>::saturating_arithmetic_free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;
  };
}
