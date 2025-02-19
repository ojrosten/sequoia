////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "QuantityTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{ 
  using namespace physics;

  namespace
  {
    template<class T, class U>
    constexpr bool can_multiply{
      requires(const T& t, const U& u) { t * u; }
    };

    template<class T, class U>
    constexpr bool can_divide{
      requires(const T& t, const U& u) { t / u; }
    };

    template<class T, class U>
    constexpr bool can_add{
      requires(const T& t, const U& u) { t + u; }
    };

    template<class T, class U>
    constexpr bool can_subtract{
      requires(const T& t, const U& u) { t - u; }
    };
  }

  [[nodiscard]]
  std::filesystem::path quantity_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void quantity_test::run_tests()
  {
    test_absolute_quantity<si::mass<float>>();
    test_absolute_quantity<si::mass<double>>();
    test_absolute_quantity<si::length<float>>();
    test_absolute_quantity<si::length<double>>();
    test_absolute_quantity<si::temperature<float>>();
    test_absolute_quantity<si::temperature<double>>();

    test_affine_quantity<si::time<float>>();
    test_affine_quantity<si::time<double>>();

    test_vector_quantity<si::electrical_charge<float>>();
    test_vector_quantity<si::electrical_charge<double>>();

    test_convex_quantity<quantity<temperature_space<float>, units::celsius_t>>();
    test_convex_quantity<quantity<temperature_space<double>, units::celsius_t>>();

    test_mixed();
  }

  template<class Quantity>
  void quantity_test::test_absolute_quantity()
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

  template<class Quantity>
  void quantity_test::test_affine_quantity()
  {
    using quantity_t = Quantity;
    using delta_q_t  = quantity_t::displacement_quantity_type;
    using space_type = quantity_t::quantity_space_type;
    using units_type = quantity_t::units_type;

    STATIC_CHECK(affine_space<space_type>);
    STATIC_CHECK(vector_space<typename space_type::vector_space_type>);
    STATIC_CHECK(!can_multiply<quantity_t, float>);
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

  template<class Quantity>
  void quantity_test::test_vector_quantity()
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

    check(equivalence, "", quantity_t{-2.0, units_type{}} / delta_q_t{1.0, units_type{}}, value_type(-2.0));
    check(equality, "", quantity_t{-2.0, units_type{}} / quantity_t{1.0, units_type{}}, quantity<euclidean_vector_space<1, value_type>, no_unit_t<std::identity>>{value_type(-2.0), no_unit<std::identity>});
  }

  template<class Quantity>
  void quantity_test::test_convex_quantity()
  {
    {
      using quantity_t = quantity<temperature_space<float>, units::celsius_t>;;
      using delta_q_t = quantity_t::displacement_quantity_type;
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

  void quantity_test::test_mixed()
  {
    using mass_t            = si::mass<float>;
    using length_t          = si::length<float>;
    using charge_t          = si::electrical_charge<float>;
    using temperature_t     = si::temperature<float>;
    using unsafe_mass_t     = quantity<mass_space<float>, units::kilogram_t, std::identity>;
    using unsafe_inv_mass_t = quantity<dual<mass_space<float>>, dual<units::kilogram_t>, std::identity>;
    using inv_charge_t      = quantity<dual<electrical_charge_space<float>>, dual<units::coulomb_t>, std::identity>;
    
    auto ml = mass_t{1.0, units::kilogram} * length_t{2.0, units::metre},
         lm = length_t{2.0, units::metre} * mass_t{1.0, units::kilogram};
    check(equivalence, "", ml, 2.0f);
    check(equivalence, "", lm, 2.0f);
    check(equality, "", lm / mass_t{2.0, units::kilogram}, length_t{1.0, units::metre});
    check(equality, "", lm / length_t{0.5, units::metre}, mass_t{4.0, units::kilogram});
    check(equality, "", (mass_t{3.0, units::kilogram} / mass_t{1.0, units::kilogram})*mass_t{3.0, units::kilogram}, mass_t{9.0, units::kilogram});
    check(equality, "", (charge_t{-3.0, units::coulomb} / charge_t{1.0, units::coulomb})*mass_t{3.0, units::kilogram}, unsafe_mass_t{-9.0, units::kilogram});
      
    auto mlc = mass_t{1.0, units::kilogram} * length_t{2.0, units::metre} * charge_t{-1.0, units::coulomb},
         clm = charge_t{-1.0, units::coulomb} * length_t{2.0, units::metre} *  mass_t{1.0, units::kilogram};

    check(equivalence, "", mlc, -2.0f);
    check(equivalence, "", clm, -2.0f);
    check(equality, "", mlc / mass_t{1.0, units::kilogram}, length_t{2.0, units::metre} * charge_t{-1.0, units::coulomb});
    check(equality, "", mlc / length_t{1.0, units::metre}, mass_t{2.0, units::kilogram} * charge_t{-1.0, units::coulomb});
    check(equality, "", mlc / charge_t{-1.0, units::coulomb}, unsafe_mass_t{1.0, units::kilogram} * length_t{2.0, units::metre});
      
    auto mlct = mass_t{1.0, units::kilogram} * length_t{2.0, units::metre} * charge_t{-1.0, units::coulomb} * temperature_t{5.0, units::kelvin},
         cltm = charge_t{-1.0, units::coulomb} * length_t{2.0, units::metre} * temperature_t{5.0, units::kelvin} *  mass_t{1.0, units::kilogram};

    check(equivalence, "", mlct, -10.0f);
    check(equivalence, "", cltm, -10.0f);
    check(equality, "", mlct / mass_t{1.0, units::kilogram}, length_t{2.0, units::metre} * charge_t{-1.0, units::coulomb} * temperature_t{5.0, units::kelvin});

    check(equivalence, "", 4.0f / length_t{2.0, units::metre}, 2.0f);
    check(equality, "", 4.0f / mass_t{2.0, units::kilogram}, unsafe_inv_mass_t{2.0f, dual<units::kilogram_t>{}});
    check(equality, "", 4.0f / unsafe_inv_mass_t{2.0f, dual<units::kilogram_t>{}}, unsafe_mass_t{2.0, units::kilogram});
    check(equality, "", 4.0f / inv_charge_t{2.0f, dual<units::coulomb_t>{}}, charge_t{2.0, units::coulomb});

    check(equality, "", 1.0f /(1.0f / charge_t{2.0, units::coulomb}), charge_t{2.0, units::coulomb});
    check(equality, "", charge_t{2.0, units::coulomb} /(1.0f / charge_t{2.0, units::coulomb}), charge_t{2.0, units::coulomb} * charge_t{2.0, units::coulomb});
  }
}
