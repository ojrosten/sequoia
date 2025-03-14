////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "VectorQuantityTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{ 
  using namespace physics;

  [[nodiscard]]
  std::filesystem::path vector_quantity_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void vector_quantity_test::run_tests()
  {
    test_vector_quantity<si::electrical_charge<float>>();
    test_vector_quantity<si::electrical_charge<double>>();
    test_vector_quantity<si::angle<float>>();
  }

  template<class Quantity>
  void vector_quantity_test::test_vector_quantity()
  {
    using quantity_t = Quantity;
    using delta_q_t  = quantity_t::displacement_quantity_type;
    using space_type = quantity_t::quantity_space_type;
    using value_type = quantity_t::value_type;
    using units_type = quantity_t::units_type;

    STATIC_CHECK(vector_space<space_type>);
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

    coordinates_operations<quantity_t>{*this}.execute();

    using inv_quantity_t = quantity<dual<space_type>, dual<units_type>>;
    coordinates_operations<inv_quantity_t>{*this}.execute();

    using euc_vec_space_qty  = quantity<euclidean_vector_space<1, value_type>, no_unit_t, std::identity>;
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
}
