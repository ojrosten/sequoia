////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "RatioFreeTest.hpp"
#include "sequoia/Maths/Algebra/Ratio.hpp"

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path ratio_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void ratio_free_test::run_tests()
  {    
    check_static<(!std::same_as<maths::ratio<1, 1>, maths::ratio<1L, 1>>)>();
    
    test_ratio<int>();
    test_ratio<std::size_t>();
    test_ratio<std::intmax_t>();

    test_ratio<float>();
    test_ratio<double>();
    test_ratio<long double>();

    test_ratio<int, float>();
    test_ratio<std::intmax_t, long double>();

    test_ratio_multiply<int>();    
    test_ratio_multiply<std::intmax_t>();
    test_ratio_multiply<unsigned>();
    test_ratio_multiply<std::size_t>();

    test_ratio_multiply<float>();
    test_ratio_multiply<double>();
    test_ratio_multiply<long double>();

    test_ratio_multiply<int, float>();
    test_ratio_multiply<std::intmax_t, long double>();

    test_ratio_divide<int>();
    test_ratio_divide<double>();
    test_ratio_divide<std::intmax_t, long double>();
  }

  template<std::integral T>
  void ratio_free_test::test_ratio()
  {
    check_static<(maths::defines_ratio_v<maths::ratio<T(1), T(2)>>)>();
    check_static<(maths::defines_ratio_v<std::ratio<T(1), T(2)>>)>();

    {
      using r = maths::ratio<T(1), T(2)>;
      check_static<(r::num == T(1))>();
      check_static<(r::den == T(2))>();
    }

    {
      using r = maths::ratio<T(2), T(4)>;
      check_static<(r::num == T(1))>();
      check_static<(r::den == T(2))>();
    }

    {
      using r = maths::ratio<T(4), T(2)>;
      check_static<(r::num == T(2))>();
      check_static<(r::den == T(1))>();
    }
  }

  template<std::integral T, std::floating_point U>
  void ratio_free_test::test_ratio()
  {
    check_static<(maths::defines_ratio_v<maths::ratio<T(2), U(1.1)>>)>();
    check_static<(maths::defines_ratio_v<maths::ratio<U(1.1), T(2)>>)>();

    {
      using r = maths::ratio<T(2), U(1.1)>;
      check_static<(r::num == T(2))>();
      check_static<(r::den == U(1.1))>();
    }

    {
      using r = maths::ratio<U(1.1), T(2)>;
      check_static<(r::num == U(1.1))>();
      check_static<(r::den == T(2))>();
    }

    {
      using r = maths::ratio<U(2), T(2)>;
      check_static<(r::num == U(1))>();
      check_static<(r::den == T(1))>();
    }
  }
  
  template<std::floating_point T>
  void ratio_free_test::test_ratio()
  {
    {
      using r = maths::ratio<T(1.1), T(2.1)>;
      check_static<(r::num == T(1.1))>();
      check_static<(r::den == T(2.1))>();
    }

    {
      using r = maths::ratio<T(1.1), T(1.1)>;
      check_static<(r::num == T(1))>();
      check_static<(r::den == T(1))>();
    }    
  }

  template<std::integral T>
  void ratio_free_test::test_ratio_multiply()
  {
    constexpr auto max{std::numeric_limits<T>::max()};
    check_static<(std::same_as<ratio_multiply<ratio<T(1), T(3)>, ratio<T(2), T(4)>>, ratio<T(1), T(6)>>)>();
    check_static<(std::same_as<ratio_multiply<ratio<max, T(1)>, ratio<T(2), T(4)>>, ratio<max, T(2)>>)>();
    if constexpr(std::same_as<T, int>)
    {
      check_static<(std::same_as<ratio_multiply<ratio<max, T(1)>, ratio<max, T(1)>, allow_ratio_fp_conversion::yes>, ratio<static_cast<std::intmax_t>(max) * max, T(1)>>)>();
    }
    else
    {
      check_static<(std::same_as<ratio_multiply<ratio<max, T(1)>, ratio<max, T(1)>, allow_ratio_fp_conversion::yes>, ratio<static_cast<long double>(max) * max, T(1)>>)>();
    }
  }

  template<std::floating_point T>
  void ratio_free_test::test_ratio_multiply()
  {
    constexpr auto max{std::numeric_limits<T>::max()};
    check_static<(std::same_as<ratio_multiply<ratio<T(1.5), T(0.5)>, ratio<T(1.0), T(2.5)>>, ratio<T(1.5), T(1.25)>>)>();
    check_static<(std::same_as<ratio_multiply<ratio<T(1.5), T(0.5)>, ratio<T(0.5), T(2.5)>>, ratio<T(1.5), T(2.5)>>)>();
    check_static<(std::same_as<ratio_multiply<ratio<max, T(3.0)>, ratio<T(0.5), max>>, ratio<T(0.5), T(3.0)>>)>();
  }

  template<std::integral T, std::floating_point U>
  void ratio_free_test::test_ratio_multiply()
  {
    check_static<(std::same_as<ratio_multiply<ratio<U(1.0), T(3)>, ratio<T(2), T(4)>>, ratio<std::intmax_t{1}, T(6)>>)>();
    check_static<(std::same_as<ratio_multiply<ratio<U(1.0), T(3)>, ratio<U(2.0), T(4)>>, ratio<U(2.0), T(12)>>)>();
    check_static<(std::same_as<ratio_multiply<ratio<U(1.1), T(3)>, ratio<T(4), U(1.1)>>, ratio<T(4), T(3)>>)>();
    check_static<(std::same_as<ratio_multiply<ratio<U(1.1), T(12)>, ratio<T(4), U(1.1)>>, ratio<std::intmax_t{1}, T(3)>>)>();
  }

  template<std::integral T>
  void ratio_free_test::test_ratio_divide()
  {
    constexpr auto max{std::numeric_limits<T>::max()};
    check_static<(std::same_as<ratio_divide<ratio<T(1), T(3)>, ratio<T(4), T(2)>>, ratio<T(1), T(6)>>)>();
    check_static<(std::same_as<ratio_divide<ratio<max, T(1)>, ratio<T(4), T(2)>>, ratio<max, T(2)>>)>();
  }

  template<std::floating_point T>
  void ratio_free_test::test_ratio_divide()
  {
    constexpr auto max{std::numeric_limits<T>::max()};
    check_static<(std::same_as<ratio_divide<ratio<T(1.5), T(0.5)>, ratio<T(2.5), T(1.0)>>, ratio<T(1.5), T(1.25)>>)>();
    check_static<(std::same_as<ratio_divide<ratio<T(1.5), T(0.5)>, ratio<T(2.5), T(0.5)>>, ratio<T(1.5), T(2.5)>>)>();
    check_static<(std::same_as<ratio_divide<ratio<max, T(3.0)>, ratio<max, T(0.5)>>, ratio<T(0.5), T(3.0)>>)>();
  }

  template<std::integral T, std::floating_point U>
  void ratio_free_test::test_ratio_divide()
  {
    check_static<(std::same_as<ratio_divide<ratio<U(1.0), T(3)>, ratio<T(4), T(2)>>, ratio<std::intmax_t{1}, T(6)>>)>();
    check_static<(std::same_as<ratio_divide<ratio<U(1.0), T(3)>, ratio<T(4), U(2.0)>>, ratio<U(2.0), T(12)>>)>();
  }
}
