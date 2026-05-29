////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "SaturatingArithmeticFreeTest.hpp"
#include "sequoia/Maths/Arithmetic/SaturatingArithmetic.hpp"

namespace sequoia::testing
{
  using namespace maths;
  
  [[nodiscard]]
  std::filesystem::path saturating_arithmetic_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void saturating_arithmetic_free_test::run_tests()
  {
    test_mul<double>();
    test_div<double>();
    test_add<double>();
    test_sub<double>();
  }

  template<class T>
  void saturating_arithmetic_free_test::test_mul()
  {
    constexpr auto gub{greatest_upper_bound<T>};
    check(equality, "", saturating_mul(gub, 2), gub);
  }

  template<class T>
  void saturating_arithmetic_free_test::test_div()
  {
  }

  template<class T>
  void saturating_arithmetic_free_test::test_add()
  {
  }

  template<class T>
  void saturating_arithmetic_free_test::test_sub()
  {
  }
}
