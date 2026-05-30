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
    test_mul<int>();
    test_mul<unsigned int>();

    test_div<double>();
    test_add<double>();
    test_sub<double>();
  }

  template<arithmetic T>
  void saturating_arithmetic_free_test::test_mul()
  {
    constexpr auto gub{greatest_upper_bound<T>},
      llb{least_lower_bound<T>},
      max{std::numeric_limits<T>::max()},
      low{std::numeric_limits<T>::lowest()};

    check(equality, "", saturating_mul(gub,  T{}), T{});
    check(equality, "", saturating_mul(T{},  gub), T{});
    check(equality, "", saturating_mul(gub, T{2}), gub);
    check(equality, "", saturating_mul(max, T{2}), gub);
    check(equality, "", saturating_mul(T{2}, gub), gub);
    check(equality, "", saturating_mul(gub,  gub), gub);
    check(equality, "", saturating_mul(gub,  llb), llb);
    check(equality, "", saturating_mul(llb,  gub), llb);

    check(equality, "", saturating_mul(llb,  T{}), T{});
    check(equality, "", saturating_mul(T{},  llb), T{});
    check(equality, "", saturating_mul(llb, T{2}), llb);
    check(equality, "", saturating_mul(low, T{2}), llb);
    check(equality, "", saturating_mul(T{2}, llb), llb);

    if constexpr(std::is_signed_v<T>)
    {
      check(equality, "", saturating_mul(gub, T(-2)), llb);
      check(equality, "", saturating_mul(llb,  llb), gub);     
    }
    else
    {
      check(equality, "", saturating_mul(llb,  llb), T{});
    }

    if constexpr(std::floating_point<T>)
    {
      constexpr T nan{std::numeric_limits<T>::quiet_NaN()};
      check("", std::isnan(saturating_mul(  nan, T(-1))));
      check("", std::isnan(saturating_mul(  nan,   T{})));
      check("", std::isnan(saturating_mul(  nan,  T{1})));
      check("", std::isnan(saturating_mul(T{-1},   nan)));
      check("", std::isnan(saturating_mul(  T{},   nan)));
      check("", std::isnan(saturating_mul( T{1},   nan)));
      check("", std::isnan(saturating_mul(  nan,   nan)));
      check("", std::isnan(saturating_mul(  gub,   nan)));
      check("", std::isnan(saturating_mul(  nan,   gub)));
      check("", std::isnan(saturating_mul(  llb,   nan)));
      check("", std::isnan(saturating_mul(  nan,   llb)));
    }
  }

  template<arithmetic T>
  void saturating_arithmetic_free_test::test_div()
  {
  }

  template<arithmetic T>
  void saturating_arithmetic_free_test::test_add()
  {
  }

  template<arithmetic T>
  void saturating_arithmetic_free_test::test_sub()
  {
  }
}
