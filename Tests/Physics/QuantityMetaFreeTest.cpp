////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "QuantityMetaFreeTest.hpp"
#include "sequoia/Physics/Quantities.hpp"

namespace sequoia::testing
{
  namespace{
    template<class T>
    inline constexpr bool has_unary_minus{
      requires(T t){ { -t } -> std::same_as<T>; }
    };
  }

  [[nodiscard]]
  std::filesystem::path quantity_meta_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void quantity_meta_free_test::run_tests()
  {
    using namespace physics;

    using mass_space_t   = mass_space<float>;
    using length_space_t = length_space<float>;
    using temp_space_t   = temperature_space<float>;
    using time_space_t   = time_space<float>;

    using delta_mass_space_t = displacement_space<classical_quantity_sets::masses, float>;
    using delta_len_space_t  = displacement_space<classical_quantity_sets::lengths, float>;
    using delta_temp_space_t = displacement_space<classical_quantity_sets::temperatures, float>;
    using delta_time_space_t = displacement_space<classical_quantity_sets::times, float>;

    STATIC_CHECK((si::mass<float>::is_intrinsically_absolute), "");
    
    STATIC_CHECK((convex_space<mass_space<float>>), "");
    STATIC_CHECK((!has_unary_minus<mass_space<float>>), "");
   
    STATIC_CHECK((vector_space<delta_mass_space_t>), "");
    STATIC_CHECK((vector_space<delta_len_space_t>), "");
    STATIC_CHECK((vector_space<direct_product<delta_mass_space_t, delta_len_space_t>>), "");
    STATIC_CHECK((vector_space<reduction_t<direct_product<delta_mass_space_t, delta_len_space_t>>>), "");
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<delta_mass_space_t, delta_len_space_t>>,
                                 reduction<direct_product<std::tuple<delta_len_space_t, delta_mass_space_t>>>>), "");
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<delta_temp_space_t, reduction_t<direct_product<delta_mass_space_t, delta_len_space_t>>>>,
                                 reduction<direct_product<std::tuple<delta_len_space_t, delta_mass_space_t, delta_temp_space_t>>>>), "");
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<reduction_t<direct_product<delta_mass_space_t, delta_len_space_t>>, delta_temp_space_t>>,
                                 reduction<direct_product<std::tuple<delta_len_space_t, delta_mass_space_t, delta_temp_space_t>>>>), "");
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<reduction_t<direct_product<delta_mass_space_t, delta_len_space_t>>,
                                                            reduction_t<direct_product<delta_time_space_t, delta_temp_space_t>>>>,
                                 reduction<direct_product<std::tuple<delta_len_space_t, delta_mass_space_t, delta_temp_space_t, delta_time_space_t>>>>), "");
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<delta_len_space_t,
                                                            reduction_t<direct_product<delta_mass_space_t, reduction_t<direct_product<delta_time_space_t, delta_temp_space_t>>>>>>,
                                 reduction<direct_product<std::tuple<delta_len_space_t, delta_mass_space_t, delta_temp_space_t, delta_time_space_t>>>>), "");

    STATIC_CHECK((vector_space<dual<delta_mass_space_t>>), "");
    STATIC_CHECK((vector_space<direct_product<dual<delta_mass_space_t>, delta_mass_space_t>>), "");
    STATIC_CHECK((vector_space<reduction_t<direct_product<dual<delta_mass_space_t>, delta_mass_space_t>>>), "");
      
    STATIC_CHECK((convex_space<length_space_t>), "");
    STATIC_CHECK((convex_space<mass_space_t>), "");
    STATIC_CHECK((convex_space<direct_product<mass_space_t, length_space_t>>), "");
    STATIC_CHECK((convex_space<reduction_t<direct_product<mass_space_t, length_space_t>>>), "");
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<mass_space_t, length_space_t>>,
                                 reduction<direct_product<std::tuple<length_space_t, mass_space_t>>>>), "");
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<mass_space_t, length_space_t>>,
                                 reduction_t<direct_product<length_space_t, mass_space_t>>>), "");
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<temp_space_t, reduction_t<direct_product<mass_space_t, length_space_t>>>>,
                                 reduction<direct_product<std::tuple<length_space_t, mass_space_t, temp_space_t>>>>), "");
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<reduction_t<direct_product<mass_space_t, length_space_t>>, temp_space_t>>,
                                 reduction<direct_product<std::tuple<length_space_t, mass_space_t, temp_space_t>>>>), "");
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<reduction_t<direct_product<mass_space_t, length_space_t>>,
                                                            reduction_t<direct_product<time_space_t, temp_space_t>>>>,
                                 reduction<direct_product<std::tuple<length_space_t, mass_space_t, temp_space_t, time_space_t>>>>), "");
      
      
    STATIC_CHECK((std::is_same_v<reduction_t<std::tuple<units::metre_t, units::kilogram_t>>,
                                 reduction_t<std::tuple<units::kilogram_t, units::metre_t>>>), "");

    STATIC_CHECK((std::is_same_v<reduction_t<std::tuple<units::kelvin_t, reduction_t<std::tuple<units::metre_t, units::kilogram_t>>>>,
                                 reduction_t<std::tuple<reduction_t<std::tuple<units::kilogram_t, units::metre_t>>, units::kelvin_t>>>), "");

    STATIC_CHECK((std::is_same_v<reduction_t<std::tuple<units::coulomb_t, units::kelvin_t>>, composite_unit<std::tuple<units::coulomb_t, units::kelvin_t>>>), "");
    STATIC_CHECK((std::is_same_v<reduction_t<std::tuple<units::kilogram_t, units::metre_t>>, composite_unit<std::tuple<units::kilogram_t, units::metre_t>>>), "");
    STATIC_CHECK((std::is_same_v<reduction_t<std::tuple<composite_unit<std::tuple<units::coulomb_t, units::kelvin_t>>, composite_unit<std::tuple<units::kilogram_t, units::metre_t>>>>,
                                 composite_unit<std::tuple<units::coulomb_t, units::kelvin_t, units::kilogram_t, units::metre_t>>>), "");
      
    STATIC_CHECK((std::is_same_v<reduction_t<std::tuple<reduction_t<std::tuple<units::coulomb_t, units::kelvin_t>>, reduction_t<std::tuple<units::kilogram_t, units::metre_t>>>>,
                                 reduction_t<std::tuple<reduction_t<std::tuple<units::kelvin_t, units::kilogram_t>>, reduction_t<std::tuple<units::coulomb_t, units::metre_t>>>>>), "");
  }
}
