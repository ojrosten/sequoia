////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

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
    STATIC_CHECK(!std::same_as<maths::ratio<1, 1>, maths::ratio<1L, 1>>);
    
    test_ratio<int>();
    test_ratio<std::size_t>();
    test_ratio<std::intmax_t>();

    test_ratio<float>();
    test_ratio<double>();
    test_ratio<long double>();

    test_ratio<int, float>();
    test_ratio<std::intmax_t, long double>();

    test_ratio_multiply();
  }

  template<std::integral T>
  void ratio_free_test::test_ratio()
  {
    STATIC_CHECK(maths::defines_ratio_v<maths::ratio<T(1), T(2)>>);
    STATIC_CHECK(maths::defines_ratio_v<std::ratio<T(1), T(2)>>);

    {
      using r = maths::ratio<T(1), T(2)>;
      STATIC_CHECK(r::num == T(1));
      STATIC_CHECK(r::den == T(2));
    }

    {
      using r = maths::ratio<T(2), T(4)>;
      STATIC_CHECK(r::num == T(1));
      STATIC_CHECK(r::den == T(2));
    }

    {
      using r = maths::ratio<T(4), T(2)>;
      STATIC_CHECK(r::num == T(2));
      STATIC_CHECK(r::den == T(1));
    }
  }

  template<std::integral T, std::floating_point U>
  void ratio_free_test::test_ratio()
  {
    STATIC_CHECK(maths::defines_ratio_v<maths::ratio<T(2), U(1.1)>>);
    STATIC_CHECK(maths::defines_ratio_v<maths::ratio<U(1.1), T(2)>>);

    {
      using r = maths::ratio<T(2), U(1.1)>;
      STATIC_CHECK(r::num == T(2));
      STATIC_CHECK(r::den == U(1.1));
    }

    {
      using r = maths::ratio<U(1.1), T(2)>;
      STATIC_CHECK(r::num == U(1.1));
      STATIC_CHECK(r::den == T(2));
    }

    {
      using r = maths::ratio<U(2), T(2)>;
      STATIC_CHECK(r::num == U(1));
      STATIC_CHECK(r::den == T(1));
    }
  }
  
  template<std::floating_point T>
  void ratio_free_test::test_ratio()
  {
    {
      using r = maths::ratio<T(1.1), T(2.1)>;
      STATIC_CHECK(r::num == T(1.1));
      STATIC_CHECK(r::den == T(2.1));
    }

    {
      using r = maths::ratio<T(1.1), T(1.1)>;
      STATIC_CHECK(r::num == T(1));
      STATIC_CHECK(r::den == T(1));
    }    
  }

  void ratio_free_test::test_ratio_multiply()
  {
    constexpr auto int_max{std::numeric_limits<std::intmax_t>::max()};
    STATIC_CHECK(std::same_as<ratio_multiply<ratio<1, 3>, ratio<2, 4>>, ratio<1L, 6L>>);
    STATIC_CHECK(std::same_as<ratio_multiply<ratio<int_max, 1>, ratio<2, 4>>, ratio<int_max, 2L>>);

    STATIC_CHECK(std::same_as<ratio_multiply<ratio<1.0L, 3>, ratio<2, 4>>, ratio<2.0L, 12>>);
  }
}
