////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

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
    test_absolute_quantity<si::mass<double>>();
    test_absolute_quantity<si::length<float>>();
    test_absolute_quantity<si::time_interval<float>>();
    test_absolute_quantity<si::temperature<double>>();

    test_mass_conversions();
    test_length_conversions();
    test_area_conversions();
  }

  template<class Quantity>
  void absolute_physical_value_test::test_absolute_quantity()
  {
    using quantity_t = Quantity;
    using delta_q_t  = quantity_t::displacement_type;
    using space_type = quantity_t::space_type;
    using value_type = quantity_t::value_type;
    using units_type = quantity_t::units_type;

    STATIC_CHECK(convex_space<space_type>);
    STATIC_CHECK(vector_space<free_module_type_of_t<space_type>>);
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
    STATIC_CHECK(has_unary_plus<quantity_t>);
    STATIC_CHECK(!has_unary_minus<quantity_t>);

    check_exception_thrown<std::domain_error>("Negative quantity", [](){ return quantity_t{-1.0, units_type{}}; });

    coordinates_operations<quantity_t>{*this}.execute();
    
    check(equality,
          "",
          physical_value{value_type{2.0}, units_type{} * units_type{}},
          quantity_t{value_type{2.0}, units_type{}} * quantity_t{value_type{1.0}, units_type{}}
    );

    check(equality,
          "",
          physical_value{value_type{2.0}, units_type{} * units_type{}},
          quantity_t{value_type{1.0}, units_type{}} * quantity_t{value_type{2.0}, units_type{}}
    );

    check(equality,
          "",
          physical_value{value_type{2.0}, units_type{} * units_type{} * units_type{}},
          quantity_t{value_type{2.0}, units_type{}} * physical_value{value_type{1.0}, units_type{} * units_type{}}
    );

    check(equality,
          "",
          quantity_t{value_type{2.0}, units_type{}},
          quantity_t{value_type{2.0}, units_type{} * units_type{} / units_type{}}
    );

    check(equality,
          "",
          quantity_t{value_type{2.0}, units_type{}},
          quantity_t{value_type{2.0}, units_type{} / (units_type{} / units_type{})}
    );

    using inv_unit_t = dual<units_type>;

    check(equality,
          "",
          quantity_t{value_type{2.0}, units_type{}},
          quantity_t{value_type{2.0}, no_unit / inv_unit_t{}}
    );

    using inv_quantity_t = quantity<dual<space_type>, inv_unit_t>;
    coordinates_operations<inv_quantity_t>{*this}.execute();

    check(equality, "", (inv_quantity_t{2.0, inv_unit_t{}} * inv_quantity_t{3.0, inv_unit_t{}}) * quantity_t{2.0, units_type{}}, inv_quantity_t{12.0, inv_unit_t{}});

    check(equivalence, "", 4.0f / quantity_t{2.0, units_type{}}, 2.0f);
    check(equality, "", 4.0f / quantity_t{2.0, units_type{}}, inv_quantity_t{2.0f, inv_unit_t{}});

    using euc_half_line_qty = euclidean_half_line_quantity<value_type>;
    using euc_vec_space_qty = euclidean_1d_vector_quantity<value_type>;
    check(equality, "", quantity_t{2.0, units_type{}} / quantity_t{1.0, units_type{}}, euc_half_line_qty{2.0, no_unit});
    check(equality, "", quantity_t{2.0, units_type{}} / delta_q_t{1.0, units_type{}},  euc_vec_space_qty{2.0, no_unit});
    check(equality, "", delta_q_t{2.0, units_type{}}  / quantity_t{1.0, units_type{}}, euc_vec_space_qty{2.0, no_unit});

    check(equality, "", (quantity_t{4.0, units_type{}} *  quantity_t{3.0, units_type{}}  /  quantity_t{2.0, units_type{}}) / quantity_t{2.0, units_type{}},   euc_half_line_qty{3.0, no_unit});
    check(equality, "", (quantity_t{4.0, units_type{}} *  quantity_t{3.0, units_type{}}) / (quantity_t{2.0, units_type{}}  * quantity_t{2.0, units_type{}}),  euc_half_line_qty{3.0, no_unit});
    check(equality, "",  quantity_t{4.0, units_type{}} * (quantity_t{3.0, units_type{}}  / (quantity_t{2.0, units_type{}}  * quantity_t{2.0, units_type{}})), euc_half_line_qty{3.0, no_unit});
    check(equality, "",  quantity_t{4.0, units_type{}} * quantity_t{3.0, units_type{}}  * ((1.0 / quantity_t{2.0, units_type{}}) * (1.0 / quantity_t{2.0, units_type{}})), euc_half_line_qty{3.0, no_unit});

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

  void absolute_physical_value_test::test_mass_conversions()
  {
    STATIC_CHECK(!noexcept(si::mass<float>{}.convert_to(si::units::tonne)));
    
    check(
      equality,
      "",
      si::mass<float>{1000.0, si::units::kilogram}.convert_to(si::units::tonne),
      physical_value{1.0f, si::units::tonne}
    );

    check(
      equality,
      "",
      si::mass<float>{1.0, si::units::kilogram}.convert_to(si::units::gram),
      physical_value{1000.0f, si::units::gram}
    );

    check(
      equality,
      "",
      physical_value{1.0f, si::units::tonne}.convert_to(si::units::kilogram),
      si::mass<float>{1000, si::units::kilogram}
    );

    check(
      equality,
      "",
      physical_value{1.0f, si::units::tonne}.convert_to(si::units::gram),
      physical_value{1'000'000.0f, si::units::gram}
    );

    check(
      equality,
      "",
      physical_value{1'000'000.0f, si::units::gram}.convert_to(si::units::tonne),
      physical_value{1.0f, si::units::tonne}
    );

    check(
      equality,
      "",
      si::mass<float>{1.0, si::units::kilogram}.convert_to(si::units::kilogram),
      si::mass<float>{1.0, si::units::kilogram}
    );

    check(
      equality,
      "",
      physical_value{1000.0f, si::units::tonne}.convert_to(si::units::kilotonne),
      physical_value{1.0f, si::units::kilotonne}
    );
  }

  void absolute_physical_value_test::test_length_conversions()
  {
    check(
      equality,
      "",
      si::length<float>{1.0, si::units::metre}.convert_to(non_si::units::foot),
      physical_value{3.2808399f, non_si::units::foot}
    );

    check(
      equality,
      "",
      physical_value{1.0f, non_si::units::foot}.convert_to(si::units::metre),
      si::length<float>{0.3048f, si::units::metre}
    );
  }

  void absolute_physical_value_test::test_area_conversions()
  {
    check(
      equality,
      "",
      physical_value{1.0f, si::units::metre * si::units::metre}.convert_to(non_si::units::foot * non_si::units::foot),
      physical_value{3.2808399f * 3.2808399f, non_si::units::foot * non_si::units::foot}
    );
  }
}
