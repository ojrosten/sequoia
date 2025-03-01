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
    using electrical_charge_space_t = electrical_charge_space<float>;

    using delta_mass_space_t = displacement_space<mass_space_t>;
    using delta_len_space_t  = displacement_space<length_space_t>;
    using delta_temp_space_t = displacement_space<temp_space_t>;
    using delta_time_space_t = displacement_space<time_space_t>;

    STATIC_CHECK(si::mass<float>::is_intrinsically_absolute);
 
    STATIC_CHECK(convex_space<mass_space<float>>);
    STATIC_CHECK(!has_unary_minus<mass_space<float>>);

    STATIC_CHECK(vector_space<electrical_charge_space_t>);
    STATIC_CHECK(vector_space<delta_mass_space_t>);
    STATIC_CHECK(vector_space<delta_len_space_t>);
    STATIC_CHECK(vector_space<direct_product<delta_mass_space_t, delta_len_space_t>>);
    STATIC_CHECK(vector_space<reduction_t<direct_product<delta_mass_space_t, delta_len_space_t>>>);
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<delta_mass_space_t, delta_len_space_t>>,
                   reduction<direct_product<std::tuple<delta_len_space_t, delta_mass_space_t>>>>));
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<delta_temp_space_t, reduction_t<direct_product<delta_mass_space_t, delta_len_space_t>>>>,
                                 reduction<direct_product<std::tuple<delta_len_space_t, delta_mass_space_t, delta_temp_space_t>>>>));
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<reduction_t<direct_product<delta_mass_space_t, delta_len_space_t>>, delta_temp_space_t>>,
                                 reduction<direct_product<std::tuple<delta_len_space_t, delta_mass_space_t, delta_temp_space_t>>>>));
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<reduction_t<direct_product<delta_mass_space_t, delta_len_space_t>>,
                                                            reduction_t<direct_product<delta_time_space_t, delta_temp_space_t>>>>,
                                 reduction<direct_product<std::tuple<delta_len_space_t, delta_mass_space_t, delta_temp_space_t, delta_time_space_t>>>>));
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<delta_len_space_t,
                                                            reduction_t<direct_product<delta_mass_space_t, reduction_t<direct_product<delta_time_space_t, delta_temp_space_t>>>>>>,
                                 reduction<direct_product<std::tuple<delta_len_space_t, delta_mass_space_t, delta_temp_space_t, delta_time_space_t>>>>));

    STATIC_CHECK(vector_space<dual<delta_mass_space_t>>);
    STATIC_CHECK(vector_space<direct_product<dual<delta_mass_space_t>, delta_mass_space_t>>);
    STATIC_CHECK(vector_space<reduction_t<direct_product<dual<delta_mass_space_t>, delta_mass_space_t>>>);
      
    STATIC_CHECK(convex_space<length_space_t>);
    STATIC_CHECK(convex_space<mass_space_t>);
    STATIC_CHECK(convex_space<direct_product<mass_space_t, length_space_t>>);
    STATIC_CHECK(convex_space<reduction_t<direct_product<mass_space_t, length_space_t>>>);
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<mass_space_t, length_space_t>>,
                                 reduction<direct_product<std::tuple<length_space_t, mass_space_t>>>>));
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<mass_space_t, length_space_t>>,
                                 reduction_t<direct_product<length_space_t, mass_space_t>>>));
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<temp_space_t, reduction_t<direct_product<mass_space_t, length_space_t>>>>,
                                 reduction<direct_product<std::tuple<length_space_t, mass_space_t, temp_space_t>>>>));
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<reduction_t<direct_product<mass_space_t, length_space_t>>, temp_space_t>>,
                                 reduction<direct_product<std::tuple<length_space_t, mass_space_t, temp_space_t>>>>));
    STATIC_CHECK((std::is_same_v<reduction_t<direct_product<reduction_t<direct_product<mass_space_t, length_space_t>>,
                                                            reduction_t<direct_product<time_space_t, temp_space_t>>>>,
                                 reduction<direct_product<std::tuple<length_space_t, mass_space_t, temp_space_t, time_space_t>>>>));
      
    STATIC_CHECK((std::is_same_v<reduction_t<std::tuple<units::metre_t, units::kilogram_t>>,
                                 reduction_t<std::tuple<units::kilogram_t, units::metre_t>>>));

    STATIC_CHECK((std::is_same_v<reduction_t<std::tuple<units::kelvin_t, reduction_t<std::tuple<units::metre_t, units::kilogram_t>>>>,
                                 reduction_t<std::tuple<reduction_t<std::tuple<units::kilogram_t, units::metre_t>>, units::kelvin_t>>>));

    STATIC_CHECK(std::is_same_v<reduction_t<std::tuple<units::coulomb_t, units::kelvin_t>>, composite_unit<std::tuple<units::coulomb_t, units::kelvin_t>>>);
    STATIC_CHECK(std::is_same_v<reduction_t<std::tuple<units::kilogram_t, units::metre_t>>, composite_unit<std::tuple<units::kilogram_t, units::metre_t>>>);
    STATIC_CHECK((std::is_same_v<reduction_t<std::tuple<composite_unit<std::tuple<units::coulomb_t, units::kelvin_t>>, composite_unit<std::tuple<units::kilogram_t, units::metre_t>>>>,
                                 composite_unit<std::tuple<units::coulomb_t, units::kelvin_t, units::kilogram_t, units::metre_t>>>));

    STATIC_CHECK(std::is_same_v<reduction_t<std::tuple<units::coulomb_t, dual<units::kelvin_t>>>, composite_unit<std::tuple<units::coulomb_t, dual<units::kelvin_t>>>>);
    STATIC_CHECK(std::is_same_v<reduction_t<std::tuple<dual<units::kelvin_t>, units::coulomb_t>>, composite_unit<std::tuple<units::coulomb_t, dual<units::kelvin_t>>>>);
    STATIC_CHECK(std::is_same_v<reduction_t<std::tuple<units::kelvin_t, dual<units::kelvin_t>>>, no_unit_t>);
    
    
    STATIC_CHECK((std::is_same_v<reduction_t<std::tuple<reduction_t<std::tuple<units::coulomb_t, units::kelvin_t>>, reduction_t<std::tuple<units::kilogram_t, units::metre_t>>>>,
                                 reduction_t<std::tuple<reduction_t<std::tuple<units::kelvin_t, units::kilogram_t>>, reduction_t<std::tuple<units::coulomb_t, units::metre_t>>>>>));

    STATIC_CHECK(convex_space<direct_product<mass_space_t, dual<mass_space_t>>>);

    using namespace physics::impl;

    STATIC_CHECK(std::is_same_v<counter_t<std::tuple<>>, std::tuple<>>);
    STATIC_CHECK(std::is_same_v<counter_t<mass_space_t>,       std::tuple<type_counter<mass_space_t, 1>>>);
    STATIC_CHECK(std::is_same_v<counter_t<dual<mass_space_t>>, std::tuple<type_counter<dual<mass_space_t>, 1>>>);
    STATIC_CHECK(std::is_same_v<counter_t<std::tuple<mass_space_t>>,       std::tuple<type_counter<mass_space_t, 1>>>);
    STATIC_CHECK(std::is_same_v<counter_t<std::tuple<dual<mass_space_t>>>, std::tuple<type_counter<dual<mass_space_t>, 1>>>);
    STATIC_CHECK(std::is_same_v<counter_t<std::tuple<mass_space_t, mass_space_t>>,             std::tuple<type_counter<mass_space_t, 2>>>);
    STATIC_CHECK(std::is_same_v<counter_t<std::tuple<mass_space_t, dual<mass_space_t>>>,       std::tuple<type_counter<mass_space_t, 0>>>);
    STATIC_CHECK(std::is_same_v<counter_t<std::tuple<dual<mass_space_t>, dual<mass_space_t>>>, std::tuple<type_counter<dual<mass_space_t>,2 >>>);

    STATIC_CHECK((std::is_same_v<counter_t<std::tuple<length_space_t, mass_space_t, dual<mass_space_t>>>,
                                 std::tuple<type_counter<mass_space_t, 0>, type_counter<length_space_t, 1>>>));
    STATIC_CHECK((std::is_same_v<counter_t<std::tuple<dual<length_space_t>, mass_space_t, dual<mass_space_t>>>,
                                 std::tuple<type_counter<mass_space_t, 0>, type_counter<dual<length_space_t>, 1>>>));

    STATIC_CHECK((std::is_same_v<counter_t<std::tuple<length_space_t, mass_space_t, dual<mass_space_t>, temp_space_t>>,
                                 std::tuple<type_counter<temp_space_t, 1>, type_counter<mass_space_t, 0>, type_counter<length_space_t, 1>>>));

    STATIC_CHECK(std::is_same_v<reduce_t<counter_t<mass_space_t>>,       std::tuple<mass_space_t>>);
    STATIC_CHECK(std::is_same_v<reduce_t<counter_t<dual<mass_space_t>>>, std::tuple<dual<mass_space_t>>>);
    STATIC_CHECK(std::is_same_v<reduce_t<counter_t<std::tuple<mass_space_t, mass_space_t>>>,  std::tuple<mass_space_t, mass_space_t>>);
    STATIC_CHECK(std::is_same_v<reduce_t<counter_t<std::tuple<mass_space_t, dual<mass_space_t>>>>,  std::tuple<euclidean_half_space<1, float>>>);

    STATIC_CHECK((std::is_same_v<simplify_t<std::tuple<length_space_t, mass_space_t, dual<mass_space_t>, temp_space_t>>,
                                 std::tuple<length_space_t, temp_space_t>>));

    STATIC_CHECK(std::is_same_v<counter_t<units::kilogram_t>,       std::tuple<type_counter<units::kilogram_t, 1>>>);
    STATIC_CHECK(std::is_same_v<counter_t<dual<units::kilogram_t>>,  std::tuple<type_counter<dual<units::kilogram_t>, 1>>>);
    STATIC_CHECK(std::is_same_v<counter_t<std::tuple<units::kilogram_t>>,       std::tuple<type_counter<units::kilogram_t, 1>>>);
    STATIC_CHECK(std::is_same_v<counter_t<std::tuple<dual<units::kilogram_t>>>, std::tuple<type_counter<dual<units::kilogram_t>, 1>>>);
    STATIC_CHECK(std::is_same_v<counter_t<std::tuple<units::kilogram_t, units::kilogram_t>>,             std::tuple<type_counter<units::kilogram_t, 2>>>);
    STATIC_CHECK(std::is_same_v<counter_t<std::tuple<units::kilogram_t, dual<units::kilogram_t>>>,       std::tuple<type_counter<units::kilogram_t, 0>>>);
    STATIC_CHECK(std::is_same_v<counter_t<std::tuple<dual<units::kilogram_t>, dual<units::kilogram_t>>>, std::tuple<type_counter<dual<units::kilogram_t>,2 >>>);
    STATIC_CHECK(std::is_same_v<reduce_t<counter_t<std::tuple<units::kilogram_t, dual<units::kilogram_t>>>>,  std::tuple<no_unit_t>>);
    
    STATIC_CHECK(std::is_same_v<reduce_t<std::tuple<type_counter<mass_space_t, 1>, type_counter<dual<delta_mass_space_t>, 1>>>, std::tuple<euclidean_vector_space<1, float>>>);
    STATIC_CHECK(std::is_same_v<counter_t<std::tuple<mass_space_t, dual<delta_mass_space_t>>>, std::tuple<type_counter<dual<delta_mass_space_t>, 1>, type_counter<mass_space_t, 1>>>);

    STATIC_CHECK(std::is_same_v<reduce_t<std::tuple<type_counter<dual<delta_mass_space_t>, 1>, type_counter<mass_space_t, 1>>>, std::tuple<euclidean_vector_space<1, float>>>);
    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<mass_space_t, dual<delta_mass_space_t>>>, euclidean_vector_space<1, float>>);
  }
}
