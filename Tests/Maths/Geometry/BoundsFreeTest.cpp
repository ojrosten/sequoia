////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

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
    test_multiply_mixed_fp<float, double>();
  }

  void bounds_free_test::test_meta()
  {
    check_static<(bounds<coordinate_bounds<float>>)>();
    check_static<(bounds<coordinate_bounds<double>>)>();
    check_static<(bounds<annulus_bounds<float>>)>();
    check_static<(bounds<annulus_bounds<double>>)>();
    // Being a set of bounds is a property of the type; being usable at a given
    // dimension is a property of what it can be called with. Which of the two
    // interfaces bounds_for consults is settled by the space's dimension, and only a
    // type supplying just one of them shows that both branches are live: coordinate
    // bounds apply componentwise at any width, whereas an annulus is a condition on
    // two coordinates jointly and so has no single-value form at all.
    check_static<(checks_single_val_against_bounds_v<coordinate_bounds<double>>)>();
    check_static<(checks_array_against_bounds_v<coordinate_bounds<double>, 2>)>();
    check_static<(checks_array_against_bounds_v<coordinate_bounds<double>, 3>)>();
    check_static<(!checks_single_val_against_bounds_v<annulus_bounds<double>>)>();
    check_static<(checks_array_against_bounds_v<annulus_bounds<double>, 2>)>();
    check_static<(!checks_array_against_bounds_v<annulus_bounds<double>, 3>)>();

    check_static<(bounds_for<coordinate_bounds<double>, euclidean_vector_space<1>>)>();
    check_static<(bounds_for<coordinate_bounds<double>, euclidean_vector_space<2>>)>();
    check_static<(bounds_for<annulus_bounds<double>, euclidean_vector_space<2>>)>();
    check_static<(!bounds_for<annulus_bounds<double>, euclidean_vector_space<1>>)>();
    check_static<(!bounds_for<annulus_bounds<double>, euclidean_vector_space<3>>)>();

    // bounds_value is a constraint on a *value*, not a type: the bounds must be a
    // genuine interval, so a degenerate one is refused however well-formed the type.
    check_static<(bounds_value<no_bounds<double>>)>();
    check_static<(bounds_value<half_line_bounds<double>>)>();
    check_static<(bounds_value<negative_half_line_bounds<double>>)>();
    check_static<(!bounds_value<coordinate_bounds<double>{1.0, 1.0}>)>();
    check_static<(!bounds_value<coordinate_bounds<double>{2.0, 1.0}>)>();
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

    constexpr auto nan{std::numeric_limits<double>::quiet_NaN()};

    check_exception_thrown<std::domain_error>(
      "",
      [](){
        throwing_validator{}(coordinate_bounds<double>{0.0, 1.0}, nan);
      }
    );

    check_exception_thrown<std::domain_error>(
      "",
      [](){
        throwing_validator{}(coordinate_bounds<double>{0.0, 1.0}, std::array{2.0, nan});
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
    constexpr static auto inf{std::numeric_limits<T>::infinity()};
    using cb = coordinate_bounds<T>;

    check(equality, "+ve, finite non-overlapping",  cb{1.0, 2.0} * cb{3.0, 4.0}, cb{3.0, 8.0});
    check(equality, "+ve, finite overlapping",      cb{1.0, 4.0} * cb{2.0, 3.0}, cb{2.0, 12.0});
    check(equality, "+ve, finite / infinite",       cb{1.0, 4.0} * cb{0.5, inf}, cb{0.5, inf});
    check(equality, "semi+ve, finite / infinite",   cb{0.0, 4.0} * cb{0.5, inf}, cb{0.0, inf});
    check(equality, "semi+ve, finite / infinite",   cb{1.0, 4.0} * cb{0.0, inf}, cb{0.0, inf});
    check(equality, "semi+ve, infinite / infinite", cb{1.0, inf} * cb{0.0, inf}, cb{0.0, inf});
    check(equality, "semi+ve, infinite / infinite", cb{0.0, inf} * cb{0.0, inf}, cb{0.0, inf});

    check(equality, "-ve, finite non-overlapping",  cb{-2.0, -1.0} * cb{-4.0, -3.0}, cb{3.0, 8.0});
    check(equality, "-ve, finite overlapping",      cb{-4.0, -1.0} * cb{-3.0, -2.0}, cb{2.0, 12.0});
    check(equality, "-ve, finite / infinite",       cb{-4.0, -1.0} * cb{-inf, -0.5}, cb{0.5, inf});
    check(equality, "semi-ve, finite / infinite",   cb{-4.0,  0.0} * cb{-inf, -0.5}, cb{0.0, inf});
    check(equality, "semi-ve, finite / infinite",   cb{-4.0, -1.0} * cb{-inf,  0.0}, cb{0.0, inf});
    check(equality, "semi-ve, infinite / infinite", cb{-inf, -1.0} * cb{-inf,  0.0}, cb{0.0, inf});
    check(equality, "semi-ve, infinite / infinite", cb{-inf,  0.0} * cb{-inf,  0.0}, cb{0.0, inf});

    check(equality, "+ve/-ve, finite",                      cb{1.0, 2.0} * cb{-3.0, -0.5}, cb{-6.0, -0.5});
    check(equality, "+ve finite / -ve, infinite",           cb{1.0, 2.0} * cb{-inf, -0.5}, cb{-inf, -0.5});
    check(equality, "+ve finite / semi-ve, infinite",       cb{1.0, 2.0} * cb{-inf,  0.0}, cb{-inf,  0.0});
    check(equality, "+ve infinite / -ve, finite",           cb{1.0, inf} * cb{-3.0, -1.0}, cb{-inf, -1.0});
    check(equality, "semi+ve infinite / -ve, finite",       cb{0.0, inf} * cb{-3.0, -1.0}, cb{-inf,  0.0});
    check(equality, "+ve infinite / -ve, infinite",         cb{1.0, inf} * cb{-inf, -1.0}, cb{-inf, -1.0});
    check(equality, "semi+ve infinite / -ve, infinite",     cb{0.0, inf} * cb{-inf, -1.0}, cb{-inf,  0.0});
    check(equality, "+ve infinite / semi-ve, infinite",     cb{1.0, inf} * cb{-inf,  0.0}, cb{-inf,  0.0});
    check(equality, "semi+ve infinite / semi-ve, infinite", cb{0.0, inf} * cb{-inf,  0.0}, cb{-inf,  0.0});

    check(equality, "-ve/+ve, finite",                      cb{-3.0, -0.5} * cb{1.0, 2.0}, cb{-6.0, -0.5});
    check(equality, "-ve / +ve finite, infinite",           cb{-inf, -0.5} * cb{1.0, 2.0}, cb{-inf, -0.5});
    check(equality, "semi-ve / +ve finite, infinite",       cb{-inf,  0.0} * cb{1.0, 2.0}, cb{-inf,  0.0});
    check(equality, "-ve / +ve infinite, finite",           cb{-3.0, -1.0} * cb{1.0, inf}, cb{-inf, -1.0});
    check(equality, "-ve / semi+ve infinite, finite",       cb{-3.0, -1.0} * cb{0.0, inf}, cb{-inf,  0.0});
    check(equality, "-ve / +ve infinite, infinite",         cb{-inf, -1.0} * cb{1.0, inf}, cb{-inf, -1.0});
    check(equality, "-ve / semi+ve infinite, infinite",     cb{-inf, -1.0} * cb{0.0, inf}, cb{-inf,  0.0});
    check(equality, "semi-ve / +ve infinite, infinite",     cb{-inf,  0.0} * cb{1.0, inf}, cb{-inf,  0.0});
    check(equality, "semi-ve / semi+ve infinite, infinite", cb{-inf,  0.0} * cb{0.0, inf}, cb{-inf,  0.0});

    check(equality, "Finite, +ve",              cb{-3.0, 0.5} * cb{ 1.0,  2.0}, cb{-6.0, 1.0});
    check(equality, "Finite, -ve",              cb{-3.0, 0.5} * cb{-2.0, -1.0}, cb{-1.0, 6.0});
    check(equality, "Finite, finte",            cb{-3.0, 0.5} * cb{-1.0,  2.0}, cb{-6.0, 3.0});
    check(equality, "-ve infnite, +ve",         cb{-inf, 0.5} * cb{ 1.0,  2.0}, cb{-inf, 1.0});
    check(equality, "-ve infnite, +ve infinte", cb{-inf, 0.5} * cb{ 1.0,  inf}, cb{-inf, inf});
    check(equality, "Infnite, +infinte",        cb{-inf, inf} * cb{-inf,  inf}, cb{-inf, inf});
  }

  template<std::floating_point T, std::floating_point U>
  void bounds_free_test::test_multiply_mixed_fp()
  {
    using common_t = std::common_type_t<T, U>;
    using cb_T     = coordinate_bounds<T>;
    using cb_U     = coordinate_bounds<U>;
    using cb_TU    = coordinate_bounds<common_t>;
    constexpr static auto infT{std::numeric_limits<T>::infinity()};
    constexpr static auto infU{std::numeric_limits<U>::infinity()};
    constexpr static auto infTU{std::numeric_limits<common_t>::infinity()};

    check(equality, "Semi+ve, semi+ve", cb_T{ 0.0, infT} * cb_U{0.0, infU}, cb_TU{ 0.0, infTU});
    check(equality, "Semi+ve, semi+ve", cb_U{ 0.0, infU} * cb_T{0.0, infT}, cb_TU{ 0.0, infTU});
    check(equality, "Finite, +ve",      cb_T{-3.0,  0.5} * cb_U{1.0,  2.0}, cb_TU{-6.0,   1.0});
    check(equality, "Finite, +ve",      cb_U{-3.0,  0.5} * cb_T{1.0,  2.0}, cb_TU{-6.0,   1.0});
  }
}
