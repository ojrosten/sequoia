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
    test_mul<double, double>();
    test_mul<float, double>();
    test_mul<int, int>();
    test_mul<unsigned int, unsigned int>();
    //test_mul<unsigned int, int>();

    test_div<double>();
    test_add<double>();
    test_sub<double>();
  }

  template<arithmetic T, arithmetic U>
  void saturating_arithmetic_free_test::test_mul()
  {
    using value_t = std::common_type_t<T, U>;
    constexpr value_t
      gub{greatest_upper_bound<value_t>},
      llb{least_lower_bound<value_t>};
    constexpr T
      gubT{greatest_upper_bound<T>},
      llbT{least_lower_bound<T>},
      maxT{std::numeric_limits<T>::max()},
      lowT{std::numeric_limits<T>::lowest()};
    constexpr U
      gubU{greatest_upper_bound<U>},
      llbU{least_lower_bound<U>},
      maxU{std::numeric_limits<U>::max()},
      lowU{std::numeric_limits<U>::lowest()};

    check(equality, "", saturating_mul(gubT,  U{}), value_t{});
    check(equality, "", saturating_mul(T{},  gubU), value_t{});
    check(equality, "", saturating_mul(gubT, U{2}), gub);
    check(equality, "", saturating_mul(T{2}, gubU), gub);    
    check(equality, "", saturating_mul(maxT, maxU), gub);
    check(equality, "", saturating_mul(T{2}, maxU), gub);
    check(equality, "", saturating_mul(gubT, gubU), gub);
    check(equality, "", saturating_mul(gubT, llbU), llb);
    check(equality, "", saturating_mul(llbT, gubU), llb);

    check(equality, "", saturating_mul(llbT,  U{}), value_t{});
    check(equality, "", saturating_mul(T{},  llbU), value_t{});
    check(equality, "", saturating_mul(llbT, maxU), llb);
    check(equality, "", saturating_mul(lowT, maxU), llb);
    check(equality, "", saturating_mul(T{2}, lowU), llb);
    check(equality, "", saturating_mul(T{2}, llbU), llb);

    if constexpr(std::is_signed_v<T>)
    {
      check(equality, "", saturating_mul(gubT, U(-2)), llb);
      check(equality, "", saturating_mul(T{-2}, gubU), llb);
      check(equality, "", saturating_mul(llbT,  llbU), gub);     
    }
    else
    {
      check(equality, "", saturating_mul(llbT,  llbU), value_t{});
    }

    if constexpr(std::floating_point<T>)
    {
      constexpr T nanT{std::numeric_limits<T>::quiet_NaN()};
      constexpr U nanU{std::numeric_limits<U>::quiet_NaN()};
      check("", std::isnan(saturating_mul( nanT, U(-1))));
      check("", std::isnan(saturating_mul( nanT,   U{})));
      check("", std::isnan(saturating_mul( nanT,  U{1})));
      check("", std::isnan(saturating_mul(T{-1},  nanU)));
      check("", std::isnan(saturating_mul(  T{},  nanU)));
      check("", std::isnan(saturating_mul( T{1},  nanU)));
      check("", std::isnan(saturating_mul( nanT,  nanU)));
      check("", std::isnan(saturating_mul( gubT,  nanU)));
      check("", std::isnan(saturating_mul( nanT,  gubU)));
      check("", std::isnan(saturating_mul( llbT,  nanU)));
      check("", std::isnan(saturating_mul( nanT,  llbU)));
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
