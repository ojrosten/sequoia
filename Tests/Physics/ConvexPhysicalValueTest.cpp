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
    test_convex_quantity<quantity<temperature_space<float,  implicit_common_arena>, si::units::celsius_t>>();
    test_convex_quantity<quantity<temperature_space<double, implicit_common_arena>, si::units::celsius_t>>();
  }

  template<class Quantity>
  void convex_physical_value_test::test_convex_quantity()
  {
    {
      using quantity_t = Quantity;
      using delta_q_t  = quantity_t::displacement_type;
      using space_type = quantity_t::space_type;

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
    }
  }
}
