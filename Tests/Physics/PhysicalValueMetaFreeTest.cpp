////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "PhysicalValueMetaFreeTest.hpp"
#include "sequoia/Physics/PhysicalValues.hpp"

namespace sequoia::testing
{
  using namespace physics;
  using namespace physics::impl;


  namespace
  {
    template<class T>
    inline constexpr bool has_unary_minus{
      requires(T t){ { -t } -> std::same_as<T>; }
    };

    using mass_space_t   =        mass_space<float, implicit_common_arena>;
    using length_space_t =      length_space<float, implicit_common_arena>;
    using temp_space_t   = temperature_space<float, implicit_common_arena>;
    using time_space_t   =        time_space<float, implicit_common_arena>;
    using electrical_current_space_t
                  = electrical_current_space<float, implicit_common_arena>;

    using delta_mass_space_t = associated_displacement_space<mass_space_t>;
    using delta_len_space_t  = associated_displacement_space<length_space_t>;
    using delta_temp_space_t = associated_displacement_space<temp_space_t>;
    using delta_time_space_t = associated_displacement_space<time_space_t>;
  }

  [[nodiscard]]
  std::filesystem::path physical_value_meta_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void physical_value_meta_free_test::run_tests()
  {
    test_type_comparator();
    test_space_properties();
    test_count_and_combine();
    test_reduce();
    test_space_reduction();
    test_units_reduction();
    test_simplify();
  }

  void physical_value_meta_free_test::test_type_comparator()
  {
    STATIC_CHECK(meta::type_comparator_v<mass_space_t, dual<mass_space_t>>);
    STATIC_CHECK(!meta::type_comparator_v<dual<mass_space_t>, mass_space_t>);
    STATIC_CHECK(meta::type_comparator_v<mass_space_t, delta_mass_space_t>);
    STATIC_CHECK(!meta::type_comparator_v<delta_mass_space_t, mass_space_t>);
    STATIC_CHECK(meta::type_comparator_v<mass_space_t, dual<delta_mass_space_t>>);
    STATIC_CHECK(!meta::type_comparator_v<dual<delta_mass_space_t>, mass_space_t>);

    STATIC_CHECK(meta::type_comparator_v<delta_mass_space_t, dual<mass_space_t>>);
    STATIC_CHECK(!meta::type_comparator_v<dual<mass_space_t>, delta_mass_space_t>);    
    STATIC_CHECK(meta::type_comparator_v<delta_mass_space_t, dual<delta_mass_space_t>>);
    STATIC_CHECK(!meta::type_comparator_v<dual<delta_mass_space_t>, delta_mass_space_t>);
    STATIC_CHECK(meta::type_comparator_v<dual<mass_space_t>, dual<delta_mass_space_t>>);
    STATIC_CHECK(!meta::type_comparator_v<dual<delta_mass_space_t>, dual<mass_space_t>>);

    STATIC_CHECK(meta::type_comparator_v<delta_mass_space_t, dual<delta_mass_space_t>>);
    STATIC_CHECK(!meta::type_comparator_v<dual<delta_mass_space_t>, delta_mass_space_t>);
  }

  void physical_value_meta_free_test::test_space_properties()
  {
    STATIC_CHECK(si::mass<float>::is_intrinsically_absolute);
 
    STATIC_CHECK(convex_space<mass_space_t>);
    STATIC_CHECK(!has_unary_minus<mass_space_t>);

    STATIC_CHECK(vector_space<electrical_current_space_t>);
    STATIC_CHECK(vector_space<delta_mass_space_t>);
    STATIC_CHECK(vector_space<direct_product<delta_mass_space_t, delta_len_space_t>>);

    STATIC_CHECK(vector_space<dual<delta_mass_space_t>>);
    STATIC_CHECK(vector_space<direct_product<dual<delta_mass_space_t>, delta_mass_space_t>>);
          
    STATIC_CHECK(convex_space<length_space_t>);
    STATIC_CHECK(convex_space<mass_space_t>);
    STATIC_CHECK(convex_space<direct_product<mass_space_t, length_space_t>>);
    
    STATIC_CHECK(convex_space<direct_product<mass_space_t, dual<mass_space_t>>>);
  }

  void physical_value_meta_free_test::test_count_and_combine()
  {
    STATIC_CHECK(std::is_same_v<count_and_combine_t<direct_product<>>, direct_product<>>);
    STATIC_CHECK(std::is_same_v<count_and_combine_t<mass_space_t>,       direct_product<type_counter<mass_space_t, 1>>>);
    STATIC_CHECK(std::is_same_v<count_and_combine_t<dual<mass_space_t>>, direct_product<type_counter<dual<mass_space_t>, 1>>>);
    STATIC_CHECK(std::is_same_v<count_and_combine_t<direct_product<mass_space_t>>,       direct_product<type_counter<mass_space_t, 1>>>);
    STATIC_CHECK(std::is_same_v<count_and_combine_t<direct_product<dual<mass_space_t>>>, direct_product<type_counter<dual<mass_space_t>, 1>>>);
    STATIC_CHECK(std::is_same_v<count_and_combine_t<direct_product<mass_space_t, mass_space_t>>,             direct_product<type_counter<mass_space_t, 2>>>);
    STATIC_CHECK(std::is_same_v<count_and_combine_t<direct_product<mass_space_t, dual<mass_space_t>>>,       direct_product<type_counter<mass_space_t, 0>>>);
    STATIC_CHECK(std::is_same_v<count_and_combine_t<direct_product<dual<mass_space_t>, dual<mass_space_t>>>, direct_product<type_counter<dual<mass_space_t>,2 >>>);
    
    STATIC_CHECK(std::is_same_v<count_and_combine_t<si::units::kilogram_t>,             direct_product<type_counter<si::units::kilogram_t, 1>>>);
    STATIC_CHECK(std::is_same_v<count_and_combine_t<dual<si::units::kilogram_t>>,       direct_product<type_counter<dual<si::units::kilogram_t>, 1>>>);
    STATIC_CHECK(std::is_same_v<count_and_combine_t<direct_product<si::units::kilogram_t>>, direct_product<type_counter<si::units::kilogram_t, 1>>>);
    STATIC_CHECK(std::is_same_v<count_and_combine_t<direct_product<dual<si::units::kilogram_t>>>, direct_product<type_counter<dual<si::units::kilogram_t>, 1>>>);

    STATIC_CHECK(std::is_same_v<count_and_combine_t<direct_product<si::units::kilogram_t, si::units::kilogram_t>>,
                                direct_product<type_counter<si::units::kilogram_t, 2>>>);

    STATIC_CHECK(std::is_same_v<count_and_combine_t<direct_product<si::units::kilogram_t, dual<si::units::kilogram_t>>>,
                                direct_product<type_counter<si::units::kilogram_t, 0>>>);

    STATIC_CHECK(std::is_same_v<count_and_combine_t<direct_product<dual<si::units::kilogram_t>, dual<si::units::kilogram_t>>>,
                                direct_product<type_counter<dual<si::units::kilogram_t>,2 >>>);    

    STATIC_CHECK(std::is_same_v<count_and_combine_t<direct_product<mass_space_t, dual<delta_mass_space_t>>>,
                                direct_product<type_counter<delta_mass_space_t, 0>>>);

    
    STATIC_CHECK(std::is_same_v<count_and_combine_t<direct_product<length_space_t, mass_space_t, dual<mass_space_t>>>,
                                 direct_product<type_counter<mass_space_t, 0>, type_counter<length_space_t, 1>>>);

    STATIC_CHECK(std::is_same_v<count_and_combine_t<direct_product<dual<length_space_t>, mass_space_t, dual<mass_space_t>>>,
                                 direct_product<type_counter<mass_space_t, 0>, type_counter<dual<length_space_t>, 1>>>);

    STATIC_CHECK(std::is_same_v<count_and_combine_t<direct_product<length_space_t, mass_space_t, dual<mass_space_t>, temp_space_t>>,
                                 direct_product<type_counter<temp_space_t, 1>, type_counter<mass_space_t, 0>, type_counter<length_space_t, 1>>>);
  }

  void physical_value_meta_free_test::test_reduce()
  {
    STATIC_CHECK(std::is_same_v<reduce_t<count_and_combine_t<direct_product<si::units::kilogram_t, dual<si::units::kilogram_t>>>>,
                                direct_product<no_unit_t>>);
    STATIC_CHECK(std::is_same_v<reduce_t<count_and_combine_t<mass_space_t>>,       direct_product<mass_space_t>>);
    STATIC_CHECK(std::is_same_v<reduce_t<count_and_combine_t<dual<mass_space_t>>>, direct_product<dual<mass_space_t>>>);
    STATIC_CHECK(std::is_same_v<reduce_t<count_and_combine_t<direct_product<mass_space_t, mass_space_t>>>,  direct_product<mass_space_t, mass_space_t>>);
    STATIC_CHECK(std::is_same_v<reduce_t<count_and_combine_t<direct_product<mass_space_t, dual<mass_space_t>>>>,  direct_product<euclidean_half_space<1, float>>>);
    STATIC_CHECK(std::is_same_v<reduce_t<count_and_combine_t<direct_product<mass_space_t, dual<delta_mass_space_t>>>>, direct_product<euclidean_vector_space<1, float>>>);
  }

  void physical_value_meta_free_test::test_simplify()
  {
    STATIC_CHECK(std::is_same_v<simplify_t<direct_product<length_space_t, mass_space_t>, direct_product<dual<mass_space_t>, temp_space_t>>,
                                reduction<direct_product<length_space_t, temp_space_t>>>);
  }

  void physical_value_meta_free_test::test_space_reduction()
  {
    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<delta_mass_space_t, delta_len_space_t>>,
                                  reduction<direct_product<delta_len_space_t, delta_mass_space_t>>>);

    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<delta_len_space_t, delta_mass_space_t>>,
                                  reduction<direct_product<delta_len_space_t, delta_mass_space_t>>>);

    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<length_space_t, mass_space_t>>,
                                  reduction<direct_product<length_space_t, mass_space_t>>>);

    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<mass_space_t, length_space_t>>,
                                  reduction<direct_product<length_space_t, mass_space_t>>>);
   
    STATIC_CHECK(std::is_same_v<to_composite_space_t<reduction_t<direct_product<length_space_t, mass_space_t>>>,
                                composite_space<length_space_t, mass_space_t>>);

    STATIC_CHECK(std::is_same_v<to_composite_space_t<reduction_t<direct_product<mass_space_t, length_space_t>>>,
                                composite_space<length_space_t, mass_space_t>>);

    STATIC_CHECK(std::is_same_v<to_composite_space_t<reduction_t<direct_product<mass_space_t, dual<delta_mass_space_t>>>>,
                                euclidean_vector_space<1, float>>);
    
    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<delta_temp_space_t, composite_space<delta_len_space_t, delta_mass_space_t>>>,
                                  reduction<direct_product<delta_len_space_t, delta_mass_space_t, delta_temp_space_t>>>);

    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<composite_space<delta_len_space_t, delta_mass_space_t>, delta_temp_space_t>>,
                                  reduction<direct_product<delta_len_space_t, delta_mass_space_t, delta_temp_space_t>>>);
    
    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<temp_space_t, composite_space<length_space_t, mass_space_t>>>,
                                  reduction<direct_product<length_space_t, mass_space_t, temp_space_t>>>);

    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<length_space_t, composite_space<mass_space_t, temp_space_t>>>,
                                  reduction<direct_product<length_space_t, mass_space_t, temp_space_t>>>);
    
    STATIC_CHECK(std::is_same_v<to_composite_space_t<reduction_t<direct_product<composite_space<length_space_t, mass_space_t>, mass_space_t>>>,
                                composite_space<length_space_t, mass_space_t, mass_space_t>>);

    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<composite_space<delta_len_space_t, delta_mass_space_t>,
                                                           composite_space<delta_temp_space_t, delta_time_space_t>>>,
                                  reduction<direct_product<delta_len_space_t, delta_mass_space_t, delta_temp_space_t, delta_time_space_t>>>);

    
    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<composite_space<length_space_t, mass_space_t>,
                                                           composite_space<electrical_current_space_t, temp_space_t>>>,
                                  reduction<direct_product<electrical_current_space_t, length_space_t, mass_space_t, temp_space_t>>>);

    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<delta_len_space_t,
                                                           to_composite_space_t<reduction_t<direct_product<delta_mass_space_t, composite_space<delta_temp_space_t, delta_time_space_t>>>>>>,
                                  reduction<direct_product<delta_len_space_t, delta_mass_space_t, delta_temp_space_t, delta_time_space_t>>>);

    STATIC_CHECK(std::is_same_v<to_composite_space_t<reduction_t<direct_product<composite_space<mass_space_t, mass_space_t>, composite_space<dual<mass_space_t>, dual<mass_space_t>>>>>,
                                euclidean_half_space<1, float>>);

    STATIC_CHECK(std::is_same_v<to_composite_space_t<reduction_t<direct_product<composite_space<mass_space_t, mass_space_t>, dual_of_t<composite_space<mass_space_t, mass_space_t>>>>>,
                                euclidean_half_space<1, float>>);
    
    STATIC_CHECK(convex_space<to_composite_space_t<reduction_t<direct_product<mass_space_t, length_space_t>>>>);
    STATIC_CHECK(vector_space<to_composite_space_t<reduction_t<direct_product<delta_mass_space_t, delta_len_space_t>>>>);
    STATIC_CHECK(vector_space<to_composite_space_t<reduction_t<direct_product<dual<delta_mass_space_t>, delta_mass_space_t>>>>);
  }

  void physical_value_meta_free_test::test_units_reduction()
  {
    using namespace si::units;

    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<metre_t, kilogram_t>>,
                                  reduction<direct_product<kilogram_t, metre_t>>>);

    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<kilogram_t, metre_t>>,
                                  reduction<direct_product<kilogram_t, metre_t>>>);

    STATIC_CHECK(std::is_same_v<to_composite_space_t<reduction_t<direct_product<metre_t, kilogram_t>>>,
                                composite_unit<kilogram_t, metre_t>>);

    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<kelvin_t, composite_unit<kilogram_t, metre_t>>>,
                                  reduction<direct_product<kelvin_t, kilogram_t, metre_t>>>);

    STATIC_CHECK(std::is_same_v<to_composite_space_t<reduction_t<direct_product<composite_unit<coulomb_t, kelvin_t>, composite_unit<kilogram_t, metre_t>>>>,
                                 composite_unit<coulomb_t, kelvin_t, kilogram_t, metre_t>>);

    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<coulomb_t, dual<kelvin_t>>>,
                                  reduction<direct_product<coulomb_t, dual<kelvin_t>>>>);

    STATIC_CHECK(std::is_same_v<reduction_t<direct_product<dual<kelvin_t>, coulomb_t>>,
                                  reduction<direct_product<coulomb_t, dual<kelvin_t>>>>);

    STATIC_CHECK(std::is_same_v<to_composite_space_t<reduction_t<direct_product<kelvin_t, dual<kelvin_t>>>>, no_unit_t>);
  }
}
