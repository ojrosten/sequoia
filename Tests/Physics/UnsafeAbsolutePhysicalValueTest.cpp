////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "UnsafeAbsolutePhysicalValueTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{
  using namespace physics;

  [[nodiscard]]
  std::filesystem::path unsafe_absolute_physical_value_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void unsafe_absolute_physical_value_test::run_tests()
  {
    test_absolute_quantity<si::mass<float>>();
    test_absolute_quantity<si::mass<double>>();
    test_absolute_quantity<si::length<float>>();
    test_absolute_quantity<si::temperature<double>>();
  }

  template<class Quantity>
  void unsafe_absolute_physical_value_test::test_absolute_quantity()
  {
    using value_type   = Quantity::value_type;
    using unsafe_qty_t = quantity<typename Quantity::units_type, value_type, canonical_representation<value_type, no_bounds<value_type>>, identity_validator>;
    using delta_q_t    = unsafe_qty_t::displacement_type;

    STATIC_CHECK(can_multiply<unsafe_qty_t, value_type>);
    STATIC_CHECK(can_divide<unsafe_qty_t, value_type>);
    STATIC_CHECK(can_add<unsafe_qty_t, unsafe_qty_t>);
    STATIC_CHECK(can_add<unsafe_qty_t, delta_q_t>);
    STATIC_CHECK(can_subtract<unsafe_qty_t, unsafe_qty_t>);
    STATIC_CHECK(can_subtract<unsafe_qty_t, delta_q_t>);
    STATIC_CHECK(has_unary_plus<unsafe_qty_t>);
    STATIC_CHECK(!has_unary_minus<unsafe_qty_t>);

    coordinates_operations<unsafe_qty_t>{*this}.execute();
  }
}
