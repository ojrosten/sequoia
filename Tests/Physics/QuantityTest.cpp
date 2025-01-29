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

  namespace{
    template<class T>
    inline constexpr bool has_unary_minus{
      requires(T t){ { -t } -> std::same_as<T>; }
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
    static_assert(convex_space<mass_space<float>>);
    static_assert(!has_unary_minus<mass_space<float>>);

    {
      using mass_t    = si::mass<float>;
      using delta_m_t = mass_t::displacement_quantity_type;
      static_assert(vector_space<delta_m_t>);
      static_assert(mass_t::is_intrinsically_absolute);
            
      check_exception_thrown<std::domain_error>("Negative mass", [](){ return mass_t{-1.0, units::kilogram}; });

      coordinates_operations<mass_t>{*this}.execute();

      check(equality, "", mass_t{2.0, units::kilogram} / delta_m_t{1.0, units::kilogram}, 2.0f);
      check(equality, "", delta_m_t{2.0, units::kilogram} / mass_t{1.0, units::kilogram}, 2.0f);
    }

    {
      using unsafe_mass_t = quantity<mass_space<float>, units::kilogram_t, std::identity>;

      coordinates_operations<unsafe_mass_t>{*this}.execute();
    }

    {
      using mass_t   = si::mass<float>;
      using length_t = si::length<float>;

      using mass_space_t   = mass_space<float>;
      using length_space_t = length_space<float>;
      //using temp_space_t   = temperature_space<float>;

      using delta_mass_space_t = displacement_space<classical_quantity_sets::masses, float>;
      using delta_len_space_t  = displacement_space<classical_quantity_sets::lengths, float>;
      using delta_temp_space_t = displacement_space<classical_quantity_sets::temperatures, float>;
   
      static_assert(vector_space<delta_mass_space_t>);
      static_assert(vector_space<delta_len_space_t>);
      static_assert(vector_space<direct_product<delta_mass_space_t, delta_len_space_t>>);
      static_assert(vector_space<reduction_t<direct_product<delta_mass_space_t, delta_len_space_t>>>);
      static_assert(std::is_same_v<reduction_t<direct_product<delta_mass_space_t, delta_len_space_t>>,
                                   reduction<direct_product<std::tuple<delta_len_space_t, delta_mass_space_t>>>>);
      static_assert(std::is_same_v<reduction_t<direct_product<delta_temp_space_t, reduction_t<direct_product<delta_mass_space_t, delta_len_space_t>>>>,
                                   reduction<direct_product<std::tuple<delta_len_space_t, delta_mass_space_t, delta_temp_space_t>>>>);
      
      static_assert(convex_space<length_space_t>);
      static_assert(convex_space<mass_space_t>);
      static_assert(convex_space<direct_product<mass_space_t, length_space_t>>);
      static_assert(convex_space<reduction_t<direct_product<mass_space_t, length_space_t>>>);
      static_assert(std::is_same_v<reduction_t<direct_product<mass_space_t, length_space_t>>,
                                   reduction<direct_product<std::tuple<length_space_t, mass_space_t>>>>);
      static_assert(std::is_same_v<reduction_t<direct_product<mass_space_t, length_space_t>>,
                                   reduction_t<direct_product<length_space_t, mass_space_t>>>);

      //static_assert(std::is_same_v<reduction_t<direct_product<reduction_t<direct_product<mass_space_t, length_space_t>>, temp_space_t>>,
      //            reduction<direct_product<std::tuple<length_space_t, mass_space_t, temp_space_t>>>>);


      
      static_assert(std::is_same_v<reduction_t<std::tuple<units::metre_t, units::kilogram_t>>,
                                   reduction_t<std::tuple<units::kilogram_t, units::metre_t>>>);

      static_assert(std::is_same_v<reduction_t<std::tuple<units::kelvin_t, reduction_t<std::tuple<units::metre_t, units::kilogram_t>>>>,
                                   reduction_t<std::tuple<reduction_t<std::tuple<units::kilogram_t, units::metre_t>>, units::kelvin_t>>>);

      static_assert(std::is_same_v<reduction_t<std::tuple<units::coulomb_t, units::kelvin_t>>, composite_unit<std::tuple<units::coulomb_t, units::kelvin_t>>>);
      static_assert(std::is_same_v<reduction_t<std::tuple<units::kilogram_t, units::metre_t>>, composite_unit<std::tuple<units::kilogram_t, units::metre_t>>>);
      static_assert(std::is_same_v<reduction_t<std::tuple<composite_unit<std::tuple<units::coulomb_t, units::kelvin_t>>, composite_unit<std::tuple<units::kilogram_t, units::metre_t>>>>,
                    composite_unit<std::tuple<units::coulomb_t, units::kelvin_t, units::kilogram_t, units::metre_t>>
                    >);
      
      static_assert(std::is_same_v<reduction_t<std::tuple<reduction_t<std::tuple<units::coulomb_t, units::kelvin_t>>, reduction_t<std::tuple<units::kilogram_t, units::metre_t>>>>,
                                  reduction_t<std::tuple<reduction_t<std::tuple<units::kelvin_t, units::kilogram_t>>, reduction_t<std::tuple<units::coulomb_t, units::metre_t>>>>>);
      
      auto ml = mass_t{1.0, units::kilogram} * length_t{2.0, units::metre},
           lm = length_t{2.0, units::metre} * mass_t{1.0, units::kilogram};
      check(equivalence, "", ml, 2.0f);
      check(equivalence, "", lm, 2.0f);

      /* using charge_t = si::electrical_charge<float>;
      using temperature_t = si::time<float>;
      
      auto mlct = mass_t{1.0, units::kilogram} * length_t{2.0, units::metre} * charge_t{-1.0, units::coulomb} * temperature_t{5.0, units::kelvin},
           cltm = charge_t{-1.0, units::coulomb} * length_t{2.0, units::metre} * temperature_t{5.0, units::kelvin} *  mass_t{1.0, units::kilogram};

      check(equivalence, "", mlct, -10.0f);
      check(equivalence, "", cltm, -10.0f);*/
    }
  }
}
