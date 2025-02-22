////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AbsoluteQuantityTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{ 
  using namespace physics;

  [[nodiscard]]
  std::filesystem::path absolute_quantity_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void absolute_quantity_test::run_tests()
  {
    test_absolute_quantity<si::mass<float>>();
    test_absolute_quantity<si::mass<double>>();
    test_absolute_quantity<si::length<float>>();
    test_absolute_quantity<si::temperature<double>>();
  }

  template<class Quantity>
  void absolute_quantity_test::test_absolute_quantity()
  {
    {
      using quantity_t = Quantity;
      using delta_q_t  = quantity_t::displacement_quantity_type;
      using space_type = quantity_t::quantity_space_type;
      using value_type = quantity_t::value_type;
      using units_type = quantity_t::units_type;

      STATIC_CHECK(convex_space<space_type>);
      STATIC_CHECK(vector_space<typename space_type::vector_space_type>);
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

      check_exception_thrown<std::domain_error>("Negative quantity", [](){ return quantity_t{-1.0, units_type{}}; });

      coordinates_operations<quantity_t>{*this}.execute();

      using inv_quantity_t = quantity<dual<space_type>, dual<units_type>>;
      coordinates_operations<inv_quantity_t>{*this}.execute();

      check(equality, "", quantity_t{2.0, units_type{}} / quantity_t{1.0, units_type{}}, quantity<euclidean_half_space<1, value_type>, no_unit_t<half_space_validator>>{2.0f, no_unit<half_space_validator>});
      check(equivalence, "", quantity_t{2.0, units_type{}} / delta_q_t{1.0, units_type{}}, value_type(2.0));
      check(equivalence, "", delta_q_t{2.0, units_type{}} / quantity_t{1.0, units_type{}}, value_type(2.0));
    }

    {
      using unsafe_qty_t = quantity<typename Quantity::quantity_space_type, typename Quantity::units_type, std::identity>;
      using delta_q_t    = unsafe_qty_t::displacement_quantity_type;
      using units_type   = unsafe_qty_t::units_type;
      using value_type   = unsafe_qty_t::value_type;

      STATIC_CHECK(can_multiply<unsafe_qty_t, value_type>);
      STATIC_CHECK(can_divide<unsafe_qty_t, value_type>);
      STATIC_CHECK(can_divide<unsafe_qty_t, unsafe_qty_t>);
      STATIC_CHECK(can_divide<unsafe_qty_t, delta_q_t>);
      STATIC_CHECK(can_divide<delta_q_t, unsafe_qty_t>);
      STATIC_CHECK(can_divide<delta_q_t, delta_q_t>);
      STATIC_CHECK(can_add<unsafe_qty_t, unsafe_qty_t>);
      STATIC_CHECK(can_add<unsafe_qty_t, delta_q_t>);
      STATIC_CHECK(can_subtract<unsafe_qty_t, unsafe_qty_t>);
      STATIC_CHECK(can_subtract<unsafe_qty_t, delta_q_t>);

      coordinates_operations<unsafe_qty_t>{*this}.execute();

      check(equivalence, "", unsafe_qty_t{-2.0, units_type{}} / delta_q_t{1.0, units_type{}}, value_type(-2.0));
    }
  }
}
