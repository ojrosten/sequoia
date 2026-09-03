////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "VectorPhysicalValueCompositionsTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{
  using namespace physics;

  [[nodiscard]]
  std::filesystem::path vector_physical_value_compositions_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void vector_physical_value_compositions_test::run_tests()
  {
    test_compositions<si::electrical_current<float>>();
    test_compositions<si::electrical_current<double>>();
    test_compositions<si::angle<float>>();
    test_compositions<euclidean_1d_vector_quantity<float>>();
    test_compositions<dimensionless_quantity<euclidean_vector_space<2>, no_unit_t, float>>();
  }

  template<class Quantity>
  void vector_physical_value_compositions_test::test_compositions()
  {
    using quantity_t = Quantity;
    using delta_q_t  = quantity_t::displacement_type;
    using value_type = quantity_t::value_type;
    using units_type = quantity_t::units_type;

    if constexpr(has_default_space_v<dual_of_t<units_type>, value_type>)
    {
      check(
        equality,
        "",
        physical_value{value_type{2.0}, units_type{} * units_type{}},
        quantity_t{value_type{2.0}, units_type{}} * quantity_t{value_type{1.0}, units_type{}}
      );

      check(
        equality,
        "",
        physical_value{value_type{2.0}, units_type{} * units_type{}},
        quantity_t{value_type{1.0}, units_type{}} * quantity_t{value_type{2.0}, units_type{}}
      );

      using inv_quantity_t = quantity<dual<units_type>, value_type>;
      coordinates_operations<inv_quantity_t>{*this}.execute();      

      using euc_vec_space_qty  = euclidean_1d_vector_quantity<value_type>;
      check(equality, "", quantity_t{-2.0, units_type{}} / quantity_t{1.0, units_type{}}, euc_vec_space_qty{value_type(-2.0), no_unit}); 
      check(equality, "", quantity_t{-2.0, units_type{}} / delta_q_t{1.0, units_type{}},  euc_vec_space_qty{value_type(-2.0), no_unit});
      check(equality, "", delta_q_t{2.0, units_type{}}  / quantity_t{-1.0, units_type{}}, euc_vec_space_qty{value_type(-2.0)});

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
}
