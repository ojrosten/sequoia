////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ConvexQuantityTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{ 
  using namespace physics;

  [[nodiscard]]
  std::filesystem::path convex_quantity_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void convex_quantity_test::run_tests()
  {
    test_convex_quantity<quantity<temperature_space<float>, units::celsius_t>>();
    test_convex_quantity<quantity<temperature_space<double>, units::celsius_t>>();
  }

  template<class Quantity>
  void convex_quantity_test::test_convex_quantity()
  {
    {
      using quantity_t = Quantity;
      using delta_q_t  = quantity_t::displacement_quantity_type;
      using space_type = quantity_t::quantity_space_type;
      using units_type = quantity_t::units_type;

      STATIC_CHECK(convex_space<temperature_space<float>>);
      STATIC_CHECK(vector_space<temperature_space<float>::vector_space_type>);
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

      using inv_quantity_t = quantity<dual<space_type>, dual<units_type>>;
      coordinates_operations<inv_quantity_t>{*this}.execute();
    }
  }
}
