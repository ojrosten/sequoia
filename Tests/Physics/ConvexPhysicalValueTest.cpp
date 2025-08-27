////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ConvexPhysicalValueTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{ 
  using namespace physics;

  [[nodiscard]]
  std::filesystem::path convex_physical_value_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void convex_physical_value_test::run_tests()
  {
    test_convex_quantity<si::temperature_celsius<float>>();
    test_convex_quantity<si::temperature_celsius<double>>();
  }

  template<class Quantity>
  void convex_physical_value_test::test_convex_quantity()
  {
    {
      using quantity_t = Quantity;
      using delta_q_t  = quantity_t::displacement_type;
      using space_type = quantity_t::space_type;
      using value_t    = quantity_t::value_type;

      STATIC_CHECK(convex_space<space_type>);
      STATIC_CHECK(vector_space<free_module_type_of_t<space_type>>);
      STATIC_CHECK(!can_multiply<quantity_t, float>);
      STATIC_CHECK(!can_multiply<quantity_t, quantity_t>);
      STATIC_CHECK(!can_divide<quantity_t, float>);
      STATIC_CHECK(!can_divide<quantity_t, quantity_t>);
      STATIC_CHECK(!can_divide<quantity_t, delta_q_t>);
      STATIC_CHECK(!can_divide<delta_q_t, quantity_t>);
      STATIC_CHECK(can_divide<delta_q_t, delta_q_t>);
      STATIC_CHECK(!can_add<quantity_t, quantity_t>);
      STATIC_CHECK(can_add<quantity_t, delta_q_t>);
      STATIC_CHECK(can_subtract<quantity_t, quantity_t>);
      STATIC_CHECK(can_subtract<quantity_t, delta_q_t>);
      STATIC_CHECK(has_unary_plus<quantity_t>);
      STATIC_CHECK(!has_unary_minus<quantity_t>);

      coordinates_operations<quantity_t>{*this}.execute();

      using units_type = quantity_t::units_type;
      STATIC_CHECK(
        !defines_physical_value_v<
          dual<space_type>,
          dual<units_type>,
          canonical_right_handed_basis<free_module_type_of_t<dual<space_type>>>,
          to_origin_type_t<dual<space_type>,
          dual<units_type>>,
          typename dual<units_type>::validator_type>
      );

      STATIC_CHECK(has_quantity_conversion_v<si::temperature<value_t>, quantity_t>);
      //STATIC_CHECK(has_quantity_conversion_v<quantity_t, si::temperature<value_t>>);

      using absolute_temp_t       = si::temperature<value_t>;
      using delta_absolute_temp_t = absolute_temp_t::displacement_type;
      
      check(
        equality,
        "",
        absolute_temp_t{}.convert_to(si::units::celsius),
        quantity_t{-273.15, si::units::celsius}
      );

      check(
        equality,
        "",
        delta_absolute_temp_t{}.convert_to(si::units::celsius),
        delta_q_t{0, si::units::celsius}
      );
    }
  }
}
