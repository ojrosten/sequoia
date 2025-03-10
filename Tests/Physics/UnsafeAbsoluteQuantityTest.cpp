////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "UnsafeAbsoluteQuantityTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{
  using namespace physics;

  [[nodiscard]]
  std::filesystem::path unsafe_absolute_quantity_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void unsafe_absolute_quantity_test::run_tests()
  {
    test_absolute_quantity<si::mass<float>>();
    test_absolute_quantity<si::mass<double>>();
    test_absolute_quantity<si::length<float>>();
    test_absolute_quantity<si::temperature<double>>();
  }

  template<class Quantity>
  void unsafe_absolute_quantity_test::test_absolute_quantity()
  {
    using space_type   = typename Quantity::quantity_space_type;
    using unsafe_qty_t = quantity<space_type, typename Quantity::units_type, std::identity>;
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

    using inv_unit_t = dual<units_type>;
    using unsafe_inv_quantity_t = quantity<dual<space_type>, inv_unit_t, std::identity>;
    coordinates_operations<unsafe_inv_quantity_t>{*this}.execute();

    using unsafe_euc_half_space_qty = quantity<euclidean_half_space<1, value_type>, no_unit_t, std::identity>;
    using euc_vec_space_qty = quantity<euclidean_vector_space<1, value_type>, no_unit_t, std::identity>;
    check(equality, "", unsafe_qty_t{2.0, units_type{}}  / unsafe_qty_t {-1.0, units_type{}}, unsafe_euc_half_space_qty{-2.0f, no_unit});
    check(equality, "", unsafe_qty_t{-2.0, units_type{}} / delta_q_t{1.0, units_type{}},      euc_vec_space_qty{-2.0, no_unit});
    check(equality, "", delta_q_t{-2.0, units_type{}}    / unsafe_qty_t{1.0, units_type{}},   euc_vec_space_qty{-2.0, no_unit});
     
    check(equality, "", 4.0f / unsafe_inv_quantity_t{2.0f, inv_unit_t{}}, unsafe_qty_t{2.0, units_type{}});
  }
}
