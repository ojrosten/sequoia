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

    using inv_unit_t = dual<units_type>;
    using inv_quantity_t = quantity<dual<space_type>, inv_unit_t>;
    coordinates_operations<inv_quantity_t>{*this}.execute();

    check(equality, "", (inv_quantity_t{2.0, inv_unit_t{}} * inv_quantity_t{3.0, inv_unit_t{}}) * quantity_t{2.0, units_type{}}, inv_quantity_t{12.0, inv_unit_t{}});

    check(equivalence, "", 4.0f / quantity_t{2.0, units_type{}}, 2.0f);
    check(equality, "", 4.0f / quantity_t{2.0, units_type{}}, inv_quantity_t{2.0f, inv_unit_t{}});

    using euc_half_space_qty = quantity<euclidean_half_space<1, value_type>, no_unit_t, intrinsic_origin, half_space_validator>;
    using euc_vec_space_qty  = quantity<euclidean_vector_space<1, value_type>, no_unit_t, intrinsic_origin, std::identity>;
    check(equality, "", quantity_t{2.0, units_type{}} / quantity_t{1.0, units_type{}}, euc_half_space_qty{2.0, no_unit});
    check(equality, "", quantity_t{2.0, units_type{}} / delta_q_t{1.0, units_type{}},  euc_vec_space_qty{2.0, no_unit});
    check(equality, "", delta_q_t{2.0, units_type{}}  / quantity_t{1.0, units_type{}}, euc_vec_space_qty{2.0, no_unit});

    check(equality, "", (quantity_t{4.0, units_type{}} *  quantity_t{3.0, units_type{}}  /  quantity_t{2.0, units_type{}}) / quantity_t{2.0, units_type{}},   euc_half_space_qty{3.0, no_unit});
    check(equality, "", (quantity_t{4.0, units_type{}} *  quantity_t{3.0, units_type{}}) / (quantity_t{2.0, units_type{}}  * quantity_t{2.0, units_type{}}),  euc_half_space_qty{3.0, no_unit});
    check(equality, "",  quantity_t{4.0, units_type{}} * (quantity_t{3.0, units_type{}}  / (quantity_t{2.0, units_type{}}  * quantity_t{2.0, units_type{}})), euc_half_space_qty{3.0, no_unit});
    check(equality, "",  quantity_t{4.0, units_type{}} * quantity_t{3.0, units_type{}}  * ((1.0 / quantity_t{2.0, units_type{}}) * (1.0 / quantity_t{2.0, units_type{}})), euc_half_space_qty{3.0, no_unit});

    check(equality, "", (quantity_t{4.0, units_type{}} *  quantity_t{3.0, units_type{}}  /  delta_q_t{2.0, units_type{}})  / delta_q_t{-2.0, units_type{}},   euc_vec_space_qty{-3.0, no_unit});
    check(equality, "", (quantity_t{4.0, units_type{}} *  quantity_t{3.0, units_type{}}) / (delta_q_t{2.0, units_type{}}   * delta_q_t{-2.0, units_type{}}),  euc_vec_space_qty{-3.0, no_unit});
    check(equality, "",  quantity_t{4.0, units_type{}} * (quantity_t{3.0, units_type{}}  / (delta_q_t{2.0, units_type{}}   * delta_q_t{-2.0, units_type{}})), euc_vec_space_qty{-3.0, no_unit});

    check(equality, "", (quantity_t{4.0, units_type{}} *  quantity_t{3.0, units_type{}}  / delta_q_t{-2.0, units_type{}})  / quantity_t{2.0, units_type{}},  euc_vec_space_qty{-3.0, no_unit});
    check(equality, "", (quantity_t{4.0, units_type{}} *  quantity_t{3.0, units_type{}}) / (delta_q_t{-2.0, units_type{}}  * quantity_t{2.0, units_type{}}),  euc_vec_space_qty{-3.0, no_unit});
    check(equality, "", (quantity_t{4.0, units_type{}} *  quantity_t{3.0, units_type{}}) * ((1.0 / delta_q_t{-2.0, units_type{}})  * (1.0 / quantity_t{2.0, units_type{}})),  euc_vec_space_qty{-3.0, no_unit});      

    check(equality, "", (delta_q_t{4.0, units_type{}} *  delta_q_t{-3.0, units_type{}}  /  quantity_t{2.0, units_type{}})  / quantity_t{2.0, units_type{}},   euc_vec_space_qty{-3.0, no_unit});
    check(equality, "", (delta_q_t{4.0, units_type{}} *  delta_q_t{-3.0, units_type{}}) / (quantity_t{2.0, units_type{}}   * quantity_t{2.0, units_type{}}),  euc_vec_space_qty{-3.0, no_unit});
    check(equality, "",  delta_q_t{4.0, units_type{}} * (delta_q_t{-3.0, units_type{}}  / (quantity_t{2.0, units_type{}}   * quantity_t{2.0, units_type{}})), euc_vec_space_qty{-3.0, no_unit});
  }
}
