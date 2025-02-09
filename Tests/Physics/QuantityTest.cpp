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
  }

  void quantity_test::test_masses()
  {
    {
      using mass_t    = si::mass<float>;
      using delta_m_t = mass_t::displacement_quantity_type;
      STATIC_CHECK((can_multiply<mass_t, float>),      "");
      STATIC_CHECK((can_divide<mass_t, float>),        "");
      STATIC_CHECK((can_divide<mass_t, mass_t>),       "");
      STATIC_CHECK((can_divide<mass_t, delta_m_t>),    "");
      STATIC_CHECK((can_divide<delta_m_t, mass_t>),    "");
      STATIC_CHECK((can_divide<delta_m_t, delta_m_t>), "");
      STATIC_CHECK((can_add<mass_t, mass_t>),          "");
      STATIC_CHECK((can_add<mass_t, delta_m_t>),       "");
      STATIC_CHECK((can_subtract<mass_t, mass_t>),     "");
      STATIC_CHECK((can_subtract<mass_t, delta_m_t>),  "");
            
      check_exception_thrown<std::domain_error>("Negative mass", [](){ return mass_t{-1.0, units::kilogram}; });

      coordinates_operations<mass_t>{*this}.execute();

      //check(equality, "", mass_t{2.0, units::kilogram} / delta_m_t{1.0, units::kilogram}, 2.0f);
      //check(equality, "", delta_m_t{2.0, units::kilogram} / mass_t{1.0, units::kilogram}, 2.0f);
    }

    {
      using unsafe_mass_t = quantity<mass_space<float>, units::kilogram_t, std::identity>;
      using delta_m_t = unsafe_mass_t::displacement_quantity_type;

      STATIC_CHECK((can_multiply<unsafe_mass_t, float>),         "");
      STATIC_CHECK((can_divide<unsafe_mass_t, float>),           "");
      STATIC_CHECK((can_divide<unsafe_mass_t, unsafe_mass_t>),   "");
      STATIC_CHECK((can_divide<unsafe_mass_t, delta_m_t>),       "");
      STATIC_CHECK((can_divide<delta_m_t, unsafe_mass_t>),       "");
      STATIC_CHECK((can_divide<delta_m_t, delta_m_t>),           "");
      STATIC_CHECK((can_add<unsafe_mass_t, unsafe_mass_t>),      "");
      STATIC_CHECK((can_add<unsafe_mass_t, delta_m_t>),          "");
      STATIC_CHECK((can_subtract<unsafe_mass_t, unsafe_mass_t>), "");
      STATIC_CHECK((can_subtract<unsafe_mass_t, delta_m_t>),     "");

      coordinates_operations<unsafe_mass_t>{*this}.execute();

      //check(equality, "", unsafe_mass_t{-2.0, units::kilogram} / delta_m_t{1.0, units::kilogram}, -2.0f);
    }

    {
      using temperature_t = si::temperature<float>;
      using delta_temp_t = temperature_t::displacement_quantity_type;
      STATIC_CHECK((can_multiply<temperature_t, float>),         "");
      STATIC_CHECK((can_divide<temperature_t, float>),           "");
      STATIC_CHECK((can_divide<temperature_t, temperature_t>),   "");
      STATIC_CHECK((can_divide<temperature_t, delta_temp_t>),    "");
      STATIC_CHECK((can_divide<delta_temp_t, temperature_t>),    "");
      STATIC_CHECK((can_divide<delta_temp_t, delta_temp_t>),     "");
      STATIC_CHECK((can_add<temperature_t, temperature_t>),      "");
      STATIC_CHECK((can_add<temperature_t, delta_temp_t>),       "");
      STATIC_CHECK((can_subtract<temperature_t, temperature_t>), "");
      STATIC_CHECK((can_subtract<temperature_t, delta_temp_t>),  "");

      coordinates_operations<temperature_t>{*this}.execute();
    }

    {
      using temperature_t = quantity<temperature_space<float>, units::celsius_t>;;
      using delta_temp_t = temperature_t::displacement_quantity_type;
      STATIC_CHECK(!(can_multiply<temperature_t, float>),         "");
      STATIC_CHECK(!(can_divide<temperature_t, float>),           "");
      STATIC_CHECK(!(can_divide<temperature_t, temperature_t>),   "");
      STATIC_CHECK(!(can_divide<temperature_t, delta_temp_t>),    "");
      STATIC_CHECK(!(can_divide<delta_temp_t, temperature_t>),    "");
      STATIC_CHECK((can_divide<delta_temp_t, delta_temp_t>),      "");
      STATIC_CHECK(!(can_add<temperature_t, temperature_t>),      "");
      STATIC_CHECK((can_add<temperature_t, delta_temp_t>),        "");
      STATIC_CHECK((can_subtract<temperature_t, temperature_t>),  "");
      STATIC_CHECK((can_subtract<temperature_t, delta_temp_t>),   "");

      coordinates_operations<temperature_t>{*this}.execute();
    }
    
    {
      using mass_t        = si::mass<float>;
      using length_t      = si::length<float>;
      using charge_t      = si::electrical_charge<float>;
      using temperature_t = si::temperature<float>;

      
      auto ml = mass_t{1.0, units::kilogram} * length_t{2.0, units::metre},
           lm = length_t{2.0, units::metre} * mass_t{1.0, units::kilogram};
      check(equivalence, "", ml, 2.0f);
      check(equivalence, "", lm, 2.0f);

      auto mlc = mass_t{1.0, units::kilogram} * length_t{2.0, units::metre} * charge_t{-1.0, units::coulomb},
           clm = charge_t{-1.0, units::coulomb} * length_t{2.0, units::metre} *  mass_t{1.0, units::kilogram};

      check(equivalence, "", mlc, -2.0f);
      check(equivalence, "", clm, -2.0f);
      
      auto mlct = mass_t{1.0, units::kilogram} * length_t{2.0, units::metre} * charge_t{-1.0, units::coulomb} * temperature_t{5.0, units::kelvin},
           cltm = charge_t{-1.0, units::coulomb} * length_t{2.0, units::metre} * temperature_t{5.0, units::kelvin} *  mass_t{1.0, units::kilogram};

      check(equivalence, "", mlct, -10.0f);
      check(equivalence, "", cltm, -10.0f);
    }
  }
}
