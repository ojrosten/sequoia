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
    test_masses();
    test_lengths();
    test_times();
    test_temperatures();
    test_charges();
    test_mixed();
  }

  void quantity_test::test_masses()
  {
    {
      using mass_t    = si::mass<float>;
      using delta_m_t = mass_t::displacement_quantity_type;
      STATIC_CHECK(can_multiply<mass_t, float>);
      STATIC_CHECK(can_divide<mass_t, float>);
      STATIC_CHECK(can_divide<mass_t, mass_t>);
      STATIC_CHECK(can_divide<mass_t, delta_m_t>);
      STATIC_CHECK(can_divide<delta_m_t, mass_t>);
      STATIC_CHECK(can_divide<delta_m_t, delta_m_t>);
      STATIC_CHECK(can_add<mass_t, mass_t>);
      STATIC_CHECK(can_add<mass_t, delta_m_t>);
      STATIC_CHECK(can_subtract<mass_t, mass_t>);
      STATIC_CHECK(can_subtract<mass_t, delta_m_t>);
            
      check_exception_thrown<std::domain_error>("Negative mass", [](){ return mass_t{-1.0, units::kilogram}; });

      coordinates_operations<mass_t>{*this}.execute();

      using inv_mass_t = quantity<dual<mass_space<float>>, dual<units::kilogram_t>>;
      coordinates_operations<inv_mass_t>{*this}.execute();

      // TO DO: this isn't quite right; the space should be the 1D Euclidean half space
      check(equality, "", mass_t{2.0, units::kilogram} / mass_t{1.0, units::kilogram}, quantity<euclidean_vector_space<1, float>, no_unit_t<half_space_validator>>{2.0f, no_unit<half_space_validator>});
      check(equivalence, "", mass_t{2.0, units::kilogram} / delta_m_t{1.0, units::kilogram}, 2.0f);
      check(equivalence, "", delta_m_t{2.0, units::kilogram} / mass_t{1.0, units::kilogram}, 2.0f);
    }

    {
      using unsafe_mass_t = quantity<mass_space<float>, units::kilogram_t, std::identity>;
      using delta_m_t = unsafe_mass_t::displacement_quantity_type;

      STATIC_CHECK(can_multiply<unsafe_mass_t, float>);
      STATIC_CHECK(can_divide<unsafe_mass_t, float>);
      STATIC_CHECK(can_divide<unsafe_mass_t, unsafe_mass_t>);
      STATIC_CHECK(can_divide<unsafe_mass_t, delta_m_t>);
      STATIC_CHECK(can_divide<delta_m_t, unsafe_mass_t>);
      STATIC_CHECK(can_divide<delta_m_t, delta_m_t>);
      STATIC_CHECK(can_add<unsafe_mass_t, unsafe_mass_t>);
      STATIC_CHECK(can_add<unsafe_mass_t, delta_m_t>);
      STATIC_CHECK(can_subtract<unsafe_mass_t, unsafe_mass_t>);
      STATIC_CHECK(can_subtract<unsafe_mass_t, delta_m_t>);

      coordinates_operations<unsafe_mass_t>{*this}.execute();

      check(equivalence, "", unsafe_mass_t{-2.0, units::kilogram} / delta_m_t{1.0, units::kilogram}, -2.0f);
    }
  }

  void quantity_test::test_lengths()
  {
    using length_t  = si::length<float>;
    using delta_len_t = length_t::displacement_quantity_type;
    STATIC_CHECK(can_multiply<length_t, float>);
    STATIC_CHECK(can_divide<length_t, float>);
    STATIC_CHECK(can_divide<length_t, length_t>);
    STATIC_CHECK(can_divide<length_t, delta_len_t>);
    STATIC_CHECK(can_divide<delta_len_t, length_t>);
    STATIC_CHECK(can_divide<delta_len_t, delta_len_t>);
    STATIC_CHECK(can_add<length_t, length_t>);
    STATIC_CHECK(can_add<length_t, delta_len_t>);
    STATIC_CHECK(can_subtract<length_t, length_t>);
    STATIC_CHECK(can_subtract<length_t, delta_len_t>);
            
    check_exception_thrown<std::domain_error>("Negative length", [](){ return length_t{-1.0, units::metre}; });

    coordinates_operations<length_t>{*this}.execute();
  }

  void quantity_test::test_times()
  {
    using time_t = si::time<float>;
    using delta_time_t = time_t::displacement_quantity_type;

    STATIC_CHECK(!can_multiply<time_t, float>);
    STATIC_CHECK(!can_divide<time_t, float>);
    STATIC_CHECK(!can_divide<time_t, time_t>);
    STATIC_CHECK(!can_divide<time_t, delta_time_t>);
    STATIC_CHECK(!can_divide<delta_time_t, time_t>);
    STATIC_CHECK(can_divide<delta_time_t, delta_time_t>);
    STATIC_CHECK(!can_add<time_t, time_t>);
    STATIC_CHECK(can_add<time_t, delta_time_t>);
    STATIC_CHECK(can_subtract<time_t, time_t>);
    STATIC_CHECK(can_subtract<time_t, delta_time_t>);

    coordinates_operations<time_t>{*this}.execute();
  }

  void quantity_test::test_temperatures()
  {
    {
      using temperature_t = si::temperature<float>;
      using delta_temp_t = temperature_t::displacement_quantity_type;
      STATIC_CHECK(can_multiply<temperature_t, float>);
      STATIC_CHECK(can_multiply<temperature_t, temperature_t>);
      STATIC_CHECK(can_divide<temperature_t, float>);
      STATIC_CHECK(can_divide<temperature_t, temperature_t>);
      STATIC_CHECK(can_divide<temperature_t, delta_temp_t>);
      STATIC_CHECK(can_divide<delta_temp_t, temperature_t>);
      STATIC_CHECK(can_divide<delta_temp_t, delta_temp_t>);
      STATIC_CHECK(can_add<temperature_t, temperature_t>);
      STATIC_CHECK(can_add<temperature_t, delta_temp_t>);
      STATIC_CHECK(can_subtract<temperature_t, temperature_t>);
      STATIC_CHECK(can_subtract<temperature_t, delta_temp_t>);

      coordinates_operations<temperature_t>{*this}.execute();
    }

    {
      using temperature_t = quantity<temperature_space<float>, units::celsius_t>;;
      using delta_temp_t = temperature_t::displacement_quantity_type;
      STATIC_CHECK(!can_multiply<temperature_t, float>);
      STATIC_CHECK(!can_multiply<temperature_t, temperature_t>);
      STATIC_CHECK(!can_divide<temperature_t, float>);
      STATIC_CHECK(!can_divide<temperature_t, temperature_t>);
      STATIC_CHECK(!can_divide<temperature_t, delta_temp_t>);
      STATIC_CHECK(!can_divide<delta_temp_t, temperature_t>);
      STATIC_CHECK(can_divide<delta_temp_t, delta_temp_t>);
      STATIC_CHECK(!can_add<temperature_t, temperature_t>);
      STATIC_CHECK(can_add<temperature_t, delta_temp_t>);
      STATIC_CHECK(can_subtract<temperature_t, temperature_t>);
      STATIC_CHECK(can_subtract<temperature_t, delta_temp_t>);

      coordinates_operations<temperature_t>{*this}.execute();
    }
  }

  void quantity_test::test_charges()
  {
    using charge_t = si::electrical_charge<float>;
    using delta_charge_t = charge_t::displacement_quantity_type;

    STATIC_CHECK(can_multiply<charge_t, float>);
    STATIC_CHECK(can_divide<charge_t, float>);
    STATIC_CHECK(can_divide<charge_t, charge_t>);
    STATIC_CHECK(can_divide<charge_t, delta_charge_t>);
    STATIC_CHECK(can_divide<delta_charge_t, charge_t>);
    STATIC_CHECK(can_divide<delta_charge_t, delta_charge_t>);
    STATIC_CHECK(can_add<charge_t, charge_t>);
    STATIC_CHECK(can_add<charge_t, delta_charge_t>);
    STATIC_CHECK(can_subtract<charge_t, charge_t>);
    STATIC_CHECK(can_subtract<charge_t, delta_charge_t>);

    coordinates_operations<charge_t>{*this}.execute();

    check(equivalence, "", charge_t{-2.0, units::coulomb} / delta_charge_t{1.0, units::coulomb}, -2.0f);
    check(equality, "", charge_t{-2.0, units::coulomb} / charge_t{1.0, units::coulomb}, quantity<euclidean_vector_space<1, float>, no_unit_t<std::identity>>{-2.0f, no_unit<std::identity>});
  }

  void quantity_test::test_mixed()
  {
    using mass_t        = si::mass<float>;
    using length_t      = si::length<float>;
    using charge_t      = si::electrical_charge<float>;
    using temperature_t = si::temperature<float>;
    using unsafe_mass_t = quantity<mass_space<float>, units::kilogram_t, std::identity>;
    using unsafe_inv_mass_t = quantity<dual<mass_space<float>>, dual<units::kilogram_t>, std::identity>;
    using inv_charge_t = quantity<dual<electrical_charge_space<float>>, dual<units::coulomb_t>, std::identity>;
    
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
