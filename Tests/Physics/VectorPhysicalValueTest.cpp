////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "VectorPhysicalValueTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

#include <numbers>

namespace sequoia::testing
{
  using namespace maths;
  using namespace physics;

  namespace alternative
  {
    struct gradian_t : coordinate_transform<non_si::units::degree_t, dilatation<std::ratio<10, 9>>, translation<0>>
    {
      using validator_type = std::identity;
      constexpr static std::string_view symbol{"gon"};
    };

    constexpr gradian_t gradian{};
  }
}

namespace sequoia::testing
{
  using namespace physics;

  [[nodiscard]]
  std::filesystem::path vector_physical_value_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void vector_physical_value_test::run_tests()
  {
    test_vector_quantity<si::electrical_current<float>>();
    test_vector_quantity<si::electrical_current<double>>();
    test_vector_quantity<si::angle<float>>();

    test_trig<float>();
    test_trig<double>();

    test_conversions<float>();
    test_conversions<double>();
    test_conversions<long double>();
  }

  template<class Quantity>
  void vector_physical_value_test::test_vector_quantity()
  {
    using quantity_t = Quantity;
    using delta_q_t  = quantity_t::displacement_type;
    using space_type = quantity_t::space_type;
    using value_type = quantity_t::value_type;
    using units_type = quantity_t::units_type;

    STATIC_CHECK(vector_space<space_type>);
    STATIC_CHECK(can_multiply<quantity_t, value_type>);
    STATIC_CHECK(can_divide<quantity_t, value_type>);
    STATIC_CHECK(can_divide<quantity_t, quantity_t>);
    STATIC_CHECK(can_divide<quantity_t, delta_q_t>);
    STATIC_CHECK(can_divide<delta_q_t, quantity_t>);
    STATIC_CHECK(can_divide<delta_q_t, delta_q_t>);
    STATIC_CHECK(can_add<quantity_t, quantity_t>);
    STATIC_CHECK(can_add<quantity_t, delta_q_t>);
    STATIC_CHECK(can_subtract<quantity_t, quantity_t>);
    STATIC_CHECK(can_subtract<quantity_t, delta_q_t>);
    STATIC_CHECK(has_unary_plus<quantity_t>);
    STATIC_CHECK(has_unary_minus<quantity_t>);

    coordinates_operations<quantity_t>{*this}.execute();

    check(equality,
          "",
          physical_value{value_type{2.0}, units_type{} * units_type{}},
          quantity_t{value_type{2.0}, units_type{}} * quantity_t{value_type{1.0}, units_type{}});

    check(equality,
          "",
          physical_value{value_type{2.0}, units_type{} * units_type{}},
          quantity_t{value_type{1.0}, units_type{}} * quantity_t{value_type{2.0}, units_type{}});

    using inv_quantity_t = quantity<dual<units_type>, value_type>;
    coordinates_operations<inv_quantity_t>{*this}.execute();

    using euc_vec_space_qty  = euclidean_1d_vector_quantity<value_type>;
    check(equality, "", quantity_t{-2.0, units_type{}} / quantity_t{1.0, units_type{}}, euc_vec_space_qty{value_type(-2.0), no_unit}); 
    check(equality, "", quantity_t{-2.0, units_type{}} / delta_q_t{1.0, units_type{}},  euc_vec_space_qty{value_type(-2.0), no_unit});
    check(equality, "", delta_q_t{2.0, units_type{}}  / quantity_t{-1.0, units_type{}}, euc_vec_space_qty{value_type(-2.0), no_unit});

    check(equality, "", (quantity_t{4.0, units_type{}} *  quantity_t{3.0, units_type{}}  /  quantity_t{2.0, units_type{}}) / quantity_t{2.0, units_type{}},   euc_vec_space_qty{3.0, no_unit});
    check(equality, "", (quantity_t{4.0, units_type{}} *  quantity_t{3.0, units_type{}}) / (quantity_t{2.0, units_type{}}  * quantity_t{2.0, units_type{}}),  euc_vec_space_qty{3.0, no_unit});
    check(equality, "",  quantity_t{4.0, units_type{}} * (quantity_t{3.0, units_type{}}  / (quantity_t{2.0, units_type{}}  * quantity_t{2.0, units_type{}})), euc_vec_space_qty{3.0, no_unit});

    check(equality, "", (quantity_t{4.0, units_type{}} *  quantity_t{3.0, units_type{}}  /  delta_q_t{2.0, units_type{}})  / delta_q_t{-2.0, units_type{}},   euc_vec_space_qty{-3.0, no_unit});
    check(equality, "", (quantity_t{4.0, units_type{}} *  quantity_t{3.0, units_type{}}) / (delta_q_t{2.0, units_type{}}   * delta_q_t{-2.0, units_type{}}),  euc_vec_space_qty{-3.0, no_unit});
    check(equality, "",  quantity_t{4.0, units_type{}} * (quantity_t{3.0, units_type{}}  / (delta_q_t{2.0, units_type{}}   * delta_q_t{-2.0, units_type{}})), euc_vec_space_qty{-3.0, no_unit});

    check(equality, "", (delta_q_t{4.0, units_type{}} *  delta_q_t{-3.0, units_type{}}  /  quantity_t{2.0, units_type{}})  / quantity_t{2.0, units_type{}},   euc_vec_space_qty{-3.0, no_unit});
    check(equality, "", (delta_q_t{4.0, units_type{}} *  delta_q_t{-3.0, units_type{}}) / (quantity_t{2.0, units_type{}}   * quantity_t{2.0, units_type{}}),  euc_vec_space_qty{-3.0, no_unit});
    check(equality, "",  delta_q_t{4.0, units_type{}} * (delta_q_t{-3.0, units_type{}}  / (quantity_t{2.0, units_type{}}   * quantity_t{2.0, units_type{}})), euc_vec_space_qty{-3.0, no_unit});

    check(equality, "", 1.0f / (1.0f / quantity_t{2.0, units_type{}}), quantity_t{2.0, units_type{}});
    check(equality, "", quantity_t{2.0, units_type{}} /(1.0f / quantity_t{2.0, units_type{}}), quantity_t{2.0, units_type{}} * quantity_t{2.0, units_type{}});
    check(equality, "", 4.0f / inv_quantity_t{2.0f, dual<units_type>{}}, quantity_t{2.0, units_type{}});
  }

  template<std::floating_point T>
  void vector_physical_value_test::test_trig()
  {
    using angle_t = si::angle<T>;
    using namespace si::units;
    constexpr auto pi{std::numbers::pi_v<T>};

    check(equality, "", sin(angle_t{-pi / 2, radian}), -T(1));
    check(equality, "", sin(angle_t{}), T{});
    check(equality, "", sin(angle_t{pi/2, radian}), T(1));

    check(equality, "", cos(angle_t{-pi / 2, radian}), std::cos(-pi/2));
    check(equality, "", cos(angle_t{}), T{1});
    check(equality, "", cos(angle_t{pi / 2, radian}), std::cos(pi / 2));

    check(equality, "", tan(angle_t{-pi / 2, radian}), std::tan(-pi / 2));
    check(equality, "", tan(angle_t{}), T{});
    check(equality, "", tan(angle_t{pi / 2, radian}), std::tan(pi / 2));

    check(equality, "", asin(T{}), angle_t{});
    if constexpr(std::is_same_v<T, float>)
    {
      // libc++ computes a value which differs from pi/2 by 1ulp
      check(within_tolerance{angle_t{T(1e-6), radian}}, "", asin(T{1}), angle_t{pi/2, radian});
    }
    else
    {
      check(equality, "", asin(T{1}), angle_t{pi/2, radian});
    }

    check(equality, "", acos(T{}), angle_t{pi / 2, radian});
    check(equality, "", acos(T{1}), angle_t{});

    check(equality, "", atan(T{}), angle_t{});
    check(equality, "", atan(T{1}), angle_t{pi / 4, radian});
  }

  template<std::floating_point T>
  void vector_physical_value_test::test_conversions()
  {
    STATIC_CHECK(std::same_as<root_transform_unit_t<alternative::gradian_t>, si::units::radian_t>);
    STATIC_CHECK(std::same_as<root_transform_unit_t<non_si::units::gradian_t>, si::units::radian_t>);
    STATIC_CHECK(std::same_as<root_transform_t<gradian_t>, coordinate_transform<si::units::radian_t, dilatation<ratio<std::intmax_t{200}, std::numbers::pi_v<long double>>>, translation<0>>>);
    STATIC_CHECK(std::same_as<root_transform_t<degree_t>, coordinate_transform<si::units::radian_t, dilatation<ratio<std::intmax_t{180}, std::numbers::pi_v<long double>>>, translation<0>>>);
    //STATIC_CHECK(std::same_as<inverse_t<root_transform_t<degree_t>>, coordinate_transform<si::units::radian_t, dilatation<ratio<std::numbers::pi_v<long double>, std::intmax_t{180}>>, translation<0>>>);
    //STATIC_CHECK(std::same_as<product_t<root_transform_t<gradian_t>, inverse_t<root_transform_t<degree_t>>>, coordinate_transform<si::units::radian_t, dilatation<ratio<std::intmax_t{10}, std::intmax_t{9}>>, translation<0>>>);
    
    
    STATIC_CHECK(std::same_as<root_transform_unit_t<milli<si::units::radian_t>>, si::units::radian_t>);
    STATIC_CHECK(std::same_as<root_transform_unit_t<milli<milli<si::units::radian_t>>>, si::units::radian_t>);

    using angle_t = si::angle<T>;
    using namespace si::units;
    constexpr auto pi{std::numbers::pi_v<long double>};

    check(
      equality,
      "Radians to Degrees",
      angle_t{1, si::units::radian}.convert_to(non_si::units::degree),
      physical_value{static_cast<T>(1.0 / (pi / 180)), non_si::units::degree}
    );

    check(
      equality,
      "Degrees to Radians",
      physical_value{static_cast<T>(1.0 / (pi / 180)), non_si::units::degree}.convert_to(si::units::radian),
      angle_t{1, si::units::radian}
    );

    check(
      equality,
      "Radians to Gradians",
      angle_t{1, si::units::radian}.convert_to(non_si::units::gradian),
      physical_value{static_cast<T>(1.0 / (pi / 200)), non_si::units::gradian}
    );

    check(
      equality,
      "Gradians to Radians",
      physical_value{static_cast<T>(1.0 / (pi / 200)), non_si::units::gradian}.convert_to(si::units::radian),
      physical_value{T(1), si::units::radian}
    );

    check(
      equality,
      "Degrees to Gradians",
      physical_value{T(360), non_si::units::degree}.convert_to(non_si::units::gradian),
      physical_value{T(400), non_si::units::gradian}  
    );

    check(
      equality,
      "Gradians to Degrees",
      physical_value{T(400), non_si::units::gradian}.convert_to(non_si::units::degree),
      physical_value{T(360), non_si::units::degree}  
    );

    check(
      equality,
      "Degrees to Gradians (not exactly representable as floating-point)",
      physical_value{T(1.1), non_si::units::degree}.convert_to(non_si::units::gradian),
      physical_value{T(1.1) * 10 / 9, non_si::units::gradian}  
    );

    check(
      equality,
      "Gradians to Degrees (not exactly representable as floating-point)",
      physical_value{T(1.1), non_si::units::gradian}.convert_to(non_si::units::degree),
      physical_value{T(1.1) * 9 / 10, non_si::units::degree}
    );

    check(
      equality,
      "Degrees to alternative::gradian",
      physical_value{T(360), non_si::units::degree}.convert_to(alternative::gradian),
      physical_value{T(400), alternative::gradian}  
    );

    check(
      equality,
      "alternative::Gradian to Degrees",
      physical_value{T(400), alternative::gradian}.convert_to(non_si::units::degree),
      physical_value{T(360), non_si::units::degree}  
    );
  }
}
