////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "SaturatingArithmeticFreeTest.hpp"
#include "sequoia/Maths/Arithmetic/SaturatingArithmetic.hpp"

namespace sequoia::testing
{
  using namespace maths;
  
  [[nodiscard]]
  std::filesystem::path saturating_mul_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

    [[nodiscard]]
  std::filesystem::path saturating_add_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  template<arithmetic T, arithmetic U>
  void saturating_mul_test_base::execute_tests()
  {
    using value_t = std::common_type_t<T, U>;
    constexpr value_t
      gub{greatest_upper_bound<value_t>},
      llb{least_lower_bound<value_t>},
      max{std::numeric_limits<value_t>::max()},
      low{std::numeric_limits<value_t>::lowest()};
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

    STATIC_CHECK(saturating_mul(gubT,  U{}) == value_t{});
    STATIC_CHECK(saturating_mul(T{},  gubU) == value_t{});
    STATIC_CHECK(saturating_mul(gubT, gubU) ==  gub);
    STATIC_CHECK(saturating_mul(gubT, llbU) == (llbU == 0 ? value_t{} : llb));
    STATIC_CHECK(saturating_mul(llbT, gubU) == (llbT == 0 ? value_t{} : llb));
    STATIC_CHECK(saturating_mul(llbT, llbU) == (((llbT == 0) || (llbU == 0))? value_t{} : gub));

    // Suppress unhelpful MSVC warning coming from integer overflow
    // on branchesin ternarys that can be sinply proven not to occur, statically 
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4307)
#endif

    check(equality, "", saturating_mul(gubT,  U{}), value_t{});
    check(equality, "", saturating_mul(T{},  gubU), value_t{});
    check(equality, "", saturating_mul(gubT, U{2}), gubT < gub ? gubT * U{2} : gub);
    check(equality, "", saturating_mul(T{2}, gubU), gubU < gub ? T{2} * gubU : gub);
    check(equality, "", saturating_mul(gubT, maxU), gub);
    check(equality, "", saturating_mul(maxT, gubU), gub);
    check(equality, "", saturating_mul(T{2}, maxU), maxU < max ? T{2} * maxU : gub);
    check(equality, "", saturating_mul(maxT, U{2}), maxT < max ? maxT * U{2} : gub);
    check(equality, "", saturating_mul(maxT, maxU), gub);
    check(equality, "", saturating_mul(gubT, gubU), gub);
    check(equality, "", saturating_mul(gubT, llbU), llbU == 0 ? value_t{} : llb);
    check(equality, "", saturating_mul(llbT, gubU), llbT == 0 ? value_t{} : llb);

    check(equality, "", saturating_mul(llbT,  U{}), value_t{});
    check(equality, "", saturating_mul(T{},  llbU), value_t{});
    check(equality, "", saturating_mul(llbT, maxU), llbT == 0 ? value_t{} : llb);
    check(equality, "", saturating_mul(lowT, maxU), lowT == 0 ? value_t{} : llb);
    check(equality, "", saturating_mul(T{2}, lowU), lowU > low ? T{2} * lowU : llb);
    check(equality, "", saturating_mul(lowT, U{2}), lowT > low ? lowT * U{2} : llb);
    check(equality, "", saturating_mul(T{2}, llbU), llbU > llb ? T{2} * llbU : llb);
    check(equality, "", saturating_mul(llbT, U{2}), llbT > llb ? llbT * U{2} : llb);
    check(equality, "", saturating_mul(llbT, llbU), ((llbT == 0) || (llbU == 0))? value_t{} : gub);

    if constexpr(std::is_signed_v<value_t>)
    {
      if constexpr(std::is_signed_v<U>)
        check(equality, "", saturating_mul(gubT, U{-2}), gubT < gub ? gubT * U{-2} :llb);

      if constexpr(std::is_signed_v<T>)
        check(equality, "", saturating_mul(T{-2}, gubU), llbU > llb ? T{-2} * gubU : llb);

#ifdef _MSC_VER
#pragma warning(pop)
#endif

      check(equality, "", saturating_mul(llbT, llbU), (llbT == 0) || (llbU == 0) ? value_t{} : gub);     
    }
    else
    {
      check(equality, "", saturating_mul(llbT,  llbU), value_t{});
    }

    if constexpr(std::numeric_limits<T>::has_quiet_NaN)
    {
      constexpr T nanT{std::numeric_limits<T>::quiet_NaN()};

      check("", std::isnan(saturating_mul( nanT, U(-1))));
      check("", std::isnan(saturating_mul( nanT,   U{})));
      check("", std::isnan(saturating_mul( nanT,  U{1})));
      check("", std::isnan(saturating_mul( nanT,  gubU)));
      check("", std::isnan(saturating_mul( nanT,  llbU)));
    }
    if constexpr(std::numeric_limits<U>::has_quiet_NaN)
    {     
      constexpr U nanU{std::numeric_limits<U>::quiet_NaN()};

      check("", std::isnan(saturating_mul(T(-1),  nanU)));
      check("", std::isnan(saturating_mul(  T{},  nanU)));
      check("", std::isnan(saturating_mul( T{1},  nanU)));
      check("", std::isnan(saturating_mul( gubT,  nanU)));
      check("", std::isnan(saturating_mul( llbT,  nanU)));
    }
  }

  template<arithmetic T, arithmetic U>
  void saturating_add_test_base::execute_tests()
  {
    using value_t = std::common_type_t<T, U>;
    constexpr value_t
      gub{greatest_upper_bound<value_t>},
      llb{least_lower_bound<value_t>};
    constexpr T
      gubT{greatest_upper_bound<T>},
      llbT{least_lower_bound<T>};
    constexpr U
      gubU{greatest_upper_bound<U>},
      llbU{least_lower_bound<U>};

    STATIC_CHECK(saturating_add(gubT,  gubU) == gub);
    STATIC_CHECK(saturating_add(llbT,  llbU) == llb);
    STATIC_CHECK(saturating_add(T{1},  U{2}) == value_t{3});

    check(equality, "", saturating_add(gubT,  gubU), gub);
    check(equality, "", saturating_add(llbT,  llbU), llb);
    check(equality, "", saturating_add(T{1},  U{2}), value_t{3});

    if constexpr(std::is_signed_v<T>)
    {
      STATIC_CHECK(saturating_add(T{-3}, U{2}) == value_t{-1});
      check(equality, "", saturating_add(T{-3}, U{2}), value_t{-1});
    }

    if constexpr(std::is_signed_v<U>)
    {
      STATIC_CHECK(saturating_add(T{2}, U{-3}) == value_t{-1});
      check(equality, "", saturating_add(T{2}, U{-3}), value_t{-1});
    }

    if constexpr(std::is_signed_v<T> && std::is_signed_v<U>)
    {
      STATIC_CHECK(saturating_add(T{-1}, U{-2}) == value_t{-3});
      check(equality, "", saturating_add(T{-1}, U{-2}), value_t{-3});
    }

    if constexpr(std::is_integral_v<value_t>)
    {
      if constexpr(gubT == gub)
      {
        STATIC_CHECK(saturating_add(T{gubT - 2}, U{1}) == value_t{gub - 1});
        STATIC_CHECK(saturating_add(T{gubT - 1}, U{2}) == gub);

        check(equality, "", saturating_add(T{gubT - 2}, U{1}), value_t{gub - 1});
        check(equality, "", saturating_add(T{gubT - 1}, U{2}), gub);
      }

      if constexpr(gubU == gub)
      {
        STATIC_CHECK(saturating_add(T{1}, U{gubU - 2}) == value_t{gub - 1});
        STATIC_CHECK(saturating_add(T{2}, U{gubU - 1}) == gub);

        check(equality, "", saturating_add(T{1}, U{gubU - 2}), value_t{gub - 1});
        check(equality, "", saturating_add(T{2}, U{gubU - 1}), gub);
      }

      if constexpr((llbT == llb) && std::is_signed_v<U>)
      {
        STATIC_CHECK(saturating_add(T{llbT + 2}, U{-1}) == value_t{llb + 1});
        STATIC_CHECK(saturating_add(T{llbT + 1}, U{-2}) == llb);

        check(equality, "", saturating_add(T{llbT + 2}, U{-1}), value_t{llb + 1});
        check(equality, "", saturating_add(T{llbT + 1}, U{-2}), llb);
      }

      if constexpr((llbU == llb) && std::is_signed_v<T>)
      {
        STATIC_CHECK(saturating_add(T{-1}, U{llbU + 2}) == value_t{llb + 1});
        STATIC_CHECK(saturating_add(T{-2}, U{llbU + 1}) == llb);

        check(equality, "", saturating_add(T{-1}, U{llbU + 2}), value_t{llb + 1});
        check(equality, "", saturating_add(T{-2}, U{llbU + 1}), llb);
      }
    }
    else if constexpr(std::is_same_v<T, U>)
    {
      constexpr value_t
        max{std::numeric_limits<value_t>::max()},
        low{std::numeric_limits<value_t>::lowest()};

      // No STATIC_CHECK: gcc does not accept a floating-point overflow in a constant expression.
      check(equality, "", saturating_add(max, max), gub);
      check(equality, "", saturating_add(low, low), llb);
    }

    if constexpr(std::numeric_limits<T>::has_quiet_NaN)
    {
      constexpr T nanT{std::numeric_limits<T>::quiet_NaN()};

      STATIC_CHECK(maths::isnan(saturating_add(nanT, U(-1))));
      STATIC_CHECK(maths::isnan(saturating_add(nanT,   U{})));
      STATIC_CHECK(maths::isnan(saturating_add(nanT,  U{1})));
      STATIC_CHECK(maths::isnan(saturating_add(nanT,  gubU)));
      STATIC_CHECK(maths::isnan(saturating_add(nanT,  llbU)));

      check("", std::isnan(saturating_add(nanT, U(-1))));
      check("", std::isnan(saturating_add(nanT,   U{})));
      check("", std::isnan(saturating_add(nanT,  U{1})));
      check("", std::isnan(saturating_add(nanT,  gubU)));
      check("", std::isnan(saturating_add(nanT,  llbU)));
    }

    if constexpr(std::numeric_limits<U>::has_quiet_NaN)
    {
      constexpr U nanU{std::numeric_limits<U>::quiet_NaN()};

      STATIC_CHECK(maths::isnan(saturating_add(T(-1), nanU)));
      STATIC_CHECK(maths::isnan(saturating_add(  T{}, nanU)));
      STATIC_CHECK(maths::isnan(saturating_add( T{1}, nanU)));
      STATIC_CHECK(maths::isnan(saturating_add( gubT, nanU)));
      STATIC_CHECK(maths::isnan(saturating_add( llbT, nanU)));

      check("", std::isnan(saturating_add(T(-1), nanU)));
      check("", std::isnan(saturating_add(  T{}, nanU)));
      check("", std::isnan(saturating_add( T{1}, nanU)));
      check("", std::isnan(saturating_add( gubT, nanU)));
      check("", std::isnan(saturating_add( llbT, nanU)));
    }
  }

  template class saturating_arithmetic_free_test<saturating_mul_test_base>;
  template class saturating_arithmetic_free_test<saturating_add_test_base>;
}
