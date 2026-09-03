////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "AbsolutePhysicalValueTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{
  using namespace physics;

  [[nodiscard]]
  std::filesystem::path absolute_physical_value_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void absolute_physical_value_test::run_tests()
  {
    test_absolute_quantity<si::mass<float>>();
    test_absolute_quantity<si::length<double>>();
    test_absolute_quantity<si::time_interval<float>>();
    test_absolute_quantity<si::temperature<double>>();
    test_absolute_quantity<euclidean_half_line_quantity<float>>();
  }

  template<class Quantity>
  void absolute_physical_value_test::test_absolute_quantity()
  {
    using quantity_t = Quantity;
    using delta_q_t  = quantity_t::displacement_type;
    using space_t    = quantity_t::space_type;
    using value_t    = quantity_t::value_type;
    using units_t    = quantity_t::units_type;
    using value_t    = quantity_t::value_type;

    STATIC_CHECK(convex_space<space_t>);
    STATIC_CHECK(vector_space<free_module_type_of_t<space_t>>);
    STATIC_CHECK(can_multiply<quantity_t, value_t>);
    STATIC_CHECK(can_divide<quantity_t, value_t>);
    STATIC_CHECK(can_divide<quantity_t, quantity_t>);
    STATIC_CHECK(can_divide<quantity_t, delta_q_t>);
    STATIC_CHECK(can_divide<delta_q_t, quantity_t>);
    STATIC_CHECK(can_divide<delta_q_t, delta_q_t>);
    STATIC_CHECK(can_add<quantity_t, quantity_t>);
    STATIC_CHECK(can_add<quantity_t, delta_q_t>);
    STATIC_CHECK(can_subtract<quantity_t, quantity_t>);
    STATIC_CHECK(can_subtract<quantity_t, delta_q_t>);
    STATIC_CHECK(has_unary_plus<quantity_t>);
    STATIC_CHECK(!has_unary_minus<quantity_t>);
    STATIC_CHECK(std::same_as<units_t, no_unit_t> ? !can_multiply<value_t, units_t> : can_multiply<value_t, units_t>);
    STATIC_CHECK(std::same_as<units_t, no_unit_t> ? !  can_divide<value_t, units_t> :   can_divide<value_t, units_t>);

    coordinates_operations<quantity_t>{*this}.execute();

    if constexpr(has_default_space_v<dual_of_t<units_t>, value_t>)
    {
      using inv_quantity_t = quantity<dual_of_t<units_t>, value_t>;
      coordinates_operations<inv_quantity_t>{*this}.execute();
    }
  }
}
