////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "BoundsFreeTest.hpp"
#include "BoundsTestingUtilities.hpp"

namespace sequoia::testing
{  
  using namespace maths;

  template<std::floating_point T>
  struct annulus_bounds
  {
    using value_type = T;
    
    T lower{}, upper{};

    [[nodiscard]]
    constexpr bool operator()(const std::array<T, 2>& vals) const noexcept
    {
      const T r{vals[0] * vals[0] + vals[1] * vals[1]};
      return (r >= lower*lower) && (r <= upper*upper);
    };

    [[nodiscard]]
    std::string format_input(std::span<const T, 2> vals) const
    {
      return std::format("{0} --> sqrt({1}*{1} + {2}*{2}) = {3}", vals, vals[0], vals[1], std::sqrt(vals[0] * vals[0] + vals[1] * vals[1]));
    }
  };
  
  [[nodiscard]]
  std::filesystem::path bounds_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void bounds_free_test::run_tests()
  {
    test_meta();
    test_exceptions();
    test_invert<float>();
    test_invert<double>();
    test_multiply_fp<float>();
    test_multiply_fp<double>();
  }

  void bounds_free_test::test_meta()
  {
    STATIC_CHECK( bounds<coordinate_bounds<float>>);
    STATIC_CHECK( bounds<coordinate_bounds<double>>);
    STATIC_CHECK( bounds<annulus_bounds<float>>);
    STATIC_CHECK( bounds<annulus_bounds<double>>);
    STATIC_CHECK( bounds_for<coordinate_bounds<double>, euclidean_vector_space<double, 1>>);
    STATIC_CHECK( bounds_for<coordinate_bounds<double>, euclidean_vector_space<double, 2>>);
    STATIC_CHECK( bounds_for<annulus_bounds<double>, euclidean_vector_space<double, 2>>);
    STATIC_CHECK(!bounds_for<annulus_bounds<double>, euclidean_vector_space<double, 1>>);
    STATIC_CHECK(!bounds_for<annulus_bounds<double>, euclidean_vector_space<double, 3>>);
  }

  void bounds_free_test::test_exceptions()
  {
    check_exception_thrown<std::domain_error>(
      "",
      [](){
        throwing_validator{}(coordinate_bounds<double>{0.0, 1.0}, 2.0);
      }
    );

    check_exception_thrown<std::domain_error>(
      "",
      [](){
        throwing_validator{}(coordinate_bounds<double>{0.0, 1.0}, std::array{2.0, 1.0});
      }
    );

    check_exception_thrown<std::domain_error>(
      "",
      [](){
        throwing_validator{}(annulus_bounds{1.0, 2.0}, std::array{3.0, 4.0});
      }
    );

    check_exception_thrown<std::domain_error>(
      "",
      [](){
        throwing_validator{}(std::array{coordinate_bounds{0.0, 1.0}, coordinate_bounds{-1.0, 0.0}}, std::array{-1.0, 0.0});
      }
    );
  }

  template<std::floating_point T>
  void bounds_free_test::test_invert()
  {
    constexpr static auto inf{std::numeric_limits<double>::infinity()};
    check(equality, "[a >  0, b <  infty]", reciprocal(coordinate_bounds{T(1.0), T(2.0)}), coordinate_bounds{T(0.5), T(1.0)});
    check(equality, "[a >  0, b == infty]", reciprocal(coordinate_bounds{T(2.0), T(inf)}), coordinate_bounds{T(0.0), T(0.5)});
    check(equality, "[a == 0, b <  infty]", reciprocal(coordinate_bounds{T(0.0), T(2.0)}), coordinate_bounds{T(0.5), T(inf)});
    check(equality, "[a == 0, b == infty]", reciprocal(coordinate_bounds{T(0.0), T(inf)}), coordinate_bounds{T(0.0), T(inf)});

    check(equality, "[a >  -inf, b <  0]", reciprocal(coordinate_bounds{T(-2.0), T(-1.0)}), coordinate_bounds{T(-1.0), T(-0.5)});
    check(equality, "[a >  -inf, b == 0]", reciprocal(coordinate_bounds{T(-2.0), T( 0.0)}), coordinate_bounds{T(-inf), T(-0.5)});
    check(equality, "[a == -inf, b <  0]", reciprocal(coordinate_bounds{T(-inf), T(-1.0)}), coordinate_bounds{T(-1.0), T( 0.0)});
    check(equality, "[a == -inf, b == 0]", reciprocal(coordinate_bounds{T(-inf), T( 0.0)}), coordinate_bounds{T(-inf), T(0.0)});

    check(equality, "[a == -inf, b == inf]", reciprocal(coordinate_bounds{T(-inf), T(inf)}), coordinate_bounds{T(-inf), T(inf)});
    check(equality, "[a == -2,   b == inf]", reciprocal(coordinate_bounds{T(-2.0), T(inf)}), coordinate_bounds{T(-inf), T(inf)});
    check(equality, "[a == -inf, b == 3]",   reciprocal(coordinate_bounds{T(-inf), T(3.0)}), coordinate_bounds{T(-inf), T(inf)});
    check(equality, "[a == -1,   b == 1]",   reciprocal(coordinate_bounds{T(-1.0), T(1.0)}), coordinate_bounds{T(-inf), T(inf)});
  }

  template<std::floating_point T>
  void bounds_free_test::test_multiply_fp()
  {
    constexpr static auto inf{std::numeric_limits<double>::infinity()};
    using cb = coordinate_bounds<T>;

    check(equality, "+ve, finite non-overlapping",  cb(1.0, 2.0) * cb{3.0, 4.0}, cb(3.0, 8.0));
    check(equality, "+ve, finite overlapping",      cb(1.0, 4.0) * cb{2.0, 3.0}, cb(2.0, 12.0));
    check(equality, "+ve, finite / infinite",       cb(1.0, 4.0) * cb{0.5, inf}, cb(0.5, inf));
    check(equality, "semi+ve, finite / infinite",   cb(0.0, 4.0) * cb{0.5, inf}, cb(0.0, inf));
    check(equality, "semi+ve, finite / infinite",   cb(1.0, 4.0) * cb{0.0, inf}, cb(0.0, inf));
    check(equality, "semi+ve, infinite / infinite", cb(1.0, inf) * cb{0.0, inf}, cb(0.0, inf));
    check(equality, "semi+ve, infinite / infinite", cb(0.0, inf) * cb{0.0, inf}, cb(0.0, inf));

    check(equality, "-ve, finite non-overlapping",  cb(-2.0, -1.0) * cb{-4.0, -3.0}, cb(3.0, 8.0));
    check(equality, "-ve, finite overlapping",      cb(-4.0, -1.0) * cb{-3.0, -2.0}, cb(2.0, 12.0));
    /*check(equality, "-ve, finite / infinite",       cb(-4.0, -1.0) * cb{-inf, -0.5}, cb(0.5, inf));
    check(equality, "semi-ve, finite / infinite",   cb(-4.0,  0.0) * cb{-inf, -0.5}, cb(0.0, inf));
    check(equality, "semi-ve, finite / infinite",   cb(-4.0, -1.0) * cb{-inf,  0.0}, cb(0.0, inf));
    check(equality, "semi-ve, infinite / infinite", cb(-inf, -1.0) * cb{-inf,  0.0}, cb(0.0, inf));
    check(equality, "semi-ve, infinite / infinite", cb(-inf,  0.0) * cb{-inf,  0.0}, cb(0.0, inf));*/
  }
}
