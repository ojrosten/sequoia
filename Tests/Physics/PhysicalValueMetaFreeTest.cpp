////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "PhysicalValueMetaFreeTest.hpp"
#include "PhysicalValueTestingUtilities.hpp"
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

    using mass_space_t   =        mass_space<implicit_common_arena>;
    using length_space_t =               length_space<implicit_common_arena>;
    using abs_temp_space_t   = absolute_temperature_space<implicit_common_arena>;
    using time_space_t   =        time_space<implicit_common_arena>;
    using electrical_current_space_t
                  = electrical_current_space<implicit_common_arena>;

    using euc_half_space_t = euclidean_half_line<implicit_common_arena>;
    using euc_vec_space_t  = euclidean_vector_space<1, implicit_common_arena>;

    using delta_mass_space_t = associated_displacement_space<mass_space_t>;
    using delta_len_space_t  = associated_displacement_space<length_space_t>;
    using delta_abs_temp_space_t = associated_displacement_space<abs_temp_space_t>;
    using delta_time_space_t = associated_displacement_space<time_space_t>;
  }

  [[nodiscard]]
  std::filesystem::path physical_value_meta_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void physical_value_meta_free_test::run_tests()
  {
    test_defines_physical_value();
    test_type_comparator();
    test_space_properties();
    test_count_and_combine();
    test_reduce();
    test_space_reduction();
    test_units_reduction();
    test_simplify();
  }

  /** defines_physical_value_v must answer, not explode, for arguments which come
      nowhere near fitting: it exists to be asked about arguments which may not
      fit, and every use of it is a negative check. It was previously constrained
      on its own parameters, so such a question was a hard error.
   */
  void physical_value_meta_free_test::test_defines_physical_value()
  {
    check_static<(!defines_physical_value_v<int, int, int, int, int, int>)>();
    check_static<(!defines_physical_value_v<mass_space_t, int, int, int, int, int>)>();
  }

  void physical_value_meta_free_test::test_type_comparator()
  {
    check_static<(meta::type_comparator_v<mass_space_t, dual<mass_space_t>>)>();
    check_static<(!meta::type_comparator_v<dual<mass_space_t>, mass_space_t>)>();
    check_static<(meta::type_comparator_v<mass_space_t, delta_mass_space_t>)>();
    check_static<(!meta::type_comparator_v<delta_mass_space_t, mass_space_t>)>();
    check_static<(meta::type_comparator_v<mass_space_t, dual<delta_mass_space_t>>)>();
    check_static<(!meta::type_comparator_v<dual<delta_mass_space_t>, mass_space_t>)>();

    check_static<(meta::type_comparator_v<delta_mass_space_t, dual<mass_space_t>>)>();
    check_static<(!meta::type_comparator_v<dual<mass_space_t>, delta_mass_space_t>)>();    
    check_static<(meta::type_comparator_v<delta_mass_space_t, dual<delta_mass_space_t>>)>();
    check_static<(!meta::type_comparator_v<dual<delta_mass_space_t>, delta_mass_space_t>)>();
    check_static<(meta::type_comparator_v<dual<mass_space_t>, dual<delta_mass_space_t>>)>();
    check_static<(!meta::type_comparator_v<dual<delta_mass_space_t>, dual<mass_space_t>>)>();

    check_static<(meta::type_comparator_v<delta_mass_space_t, dual<delta_mass_space_t>>)>();
    check_static<(!meta::type_comparator_v<dual<delta_mass_space_t>, delta_mass_space_t>)>();
  }

  void physical_value_meta_free_test::test_space_properties()
  {
    check_static<(convex_space<mass_space_t>)>();    
    check_static<(is_non_negative_orthant_v<mass_space_t>)>();
    check_static<(!has_unary_minus<mass_space_t>)>();

    check_static<(vector_space<electrical_current_space_t>)>();
    check_static<(vector_space<delta_mass_space_t>)>();
    check_static<(vector_space<tensor_product<delta_mass_space_t, delta_len_space_t>>)>();

    check_static<(vector_space<dual<delta_mass_space_t>>)>();
    check_static<(vector_space<tensor_product<dual<delta_mass_space_t>, delta_mass_space_t>>)>();
          
    check_static<(convex_space<length_space_t>)>();
    check_static<(convex_space<mass_space_t>)>();
    check_static<(convex_space<tensor_product<mass_space_t, length_space_t>>)>();
    
    check_static<(convex_space<tensor_product<mass_space_t, dual<mass_space_t>>>)>();
  }

  void physical_value_meta_free_test::test_count_and_combine()
  {
    check_static<(std::is_same_v<count_and_combine_t<tensor_product<>>, tensor_product<>>)>();
    check_static<(std::is_same_v<count_and_combine_t<mass_space_t>, tensor_product<type_counter<mass_space_t, 1>>>)>();
    check_static<(std::is_same_v<count_and_combine_t<dual<mass_space_t>>, tensor_product<type_counter<dual<mass_space_t>, 1>>>)>();
    check_static<(std::is_same_v<count_and_combine_t<tensor_product<mass_space_t>>, tensor_product<type_counter<mass_space_t, 1>>>)>();
    check_static<(std::is_same_v<count_and_combine_t<tensor_product<dual<mass_space_t>>>, tensor_product<type_counter<dual<mass_space_t>, 1>>>)>();
    check_static<(std::is_same_v<count_and_combine_t<tensor_product<mass_space_t, mass_space_t>>, tensor_product<type_counter<mass_space_t, 2>>>)>();
    check_static<(std::is_same_v<count_and_combine_t<tensor_product<mass_space_t, dual<mass_space_t>>>, tensor_product<type_counter<mass_space_t, 0>>>)>();
    check_static<(std::is_same_v<count_and_combine_t<tensor_product<dual<mass_space_t>, dual<mass_space_t>>>, tensor_product<type_counter<dual<mass_space_t>,2 >>>)>();
    
    check_static<(std::is_same_v<count_and_combine_t<si::units::kilogram_t>, tensor_product<type_counter<si::units::kilogram_t, 1>>>)>();
    check_static<(std::is_same_v<count_and_combine_t<dual<si::units::kilogram_t>>, tensor_product<type_counter<dual<si::units::kilogram_t>, 1>>>)>();
    check_static<(std::is_same_v<count_and_combine_t<tensor_product<si::units::kilogram_t>>, tensor_product<type_counter<si::units::kilogram_t, 1>>>)>();
    check_static<(std::is_same_v<count_and_combine_t<tensor_product<dual<si::units::kilogram_t>>>, tensor_product<type_counter<dual<si::units::kilogram_t>, 1>>>)>();

    check_static<(std::is_same_v<count_and_combine_t<tensor_product<si::units::kilogram_t, si::units::kilogram_t>>, tensor_product<type_counter<si::units::kilogram_t, 2>>>)>();

    check_static<(std::is_same_v<count_and_combine_t<tensor_product<si::units::kilogram_t, dual<si::units::kilogram_t>>>, tensor_product<type_counter<si::units::kilogram_t, 0>>>)>();

    check_static<(std::is_same_v<count_and_combine_t<tensor_product<dual<si::units::kilogram_t>, dual<si::units::kilogram_t>>>, tensor_product<type_counter<dual<si::units::kilogram_t>,2 >>>)>();    

    check_static<(std::is_same_v< count_and_combine_t<tensor_product<mass_space_t, delta_mass_space_t>>, tensor_product<type_counter<delta_mass_space_t, 2>> >)>("Promotion of space to its associated displacement space");
    
    check_static<(std::is_same_v<count_and_combine_t<tensor_product<mass_space_t, dual<delta_mass_space_t>>>, tensor_product<type_counter<delta_mass_space_t, 0>>>)>();
    
    check_static<(std::is_same_v<count_and_combine_t<tensor_product<length_space_t, mass_space_t, dual<mass_space_t>>>, tensor_product<type_counter<mass_space_t, 0>, type_counter<length_space_t, 1>>>)>();

    check_static<(std::is_same_v<count_and_combine_t<tensor_product<dual<length_space_t>, mass_space_t, dual<mass_space_t>>>, tensor_product<type_counter<mass_space_t, 0>, type_counter<dual<length_space_t>, 1>>>)>();

    check_static<(std::is_same_v<count_and_combine_t<tensor_product<length_space_t, mass_space_t, dual<mass_space_t>, abs_temp_space_t>>, tensor_product<type_counter<abs_temp_space_t, 1>, type_counter<mass_space_t, 0>, type_counter<length_space_t, 1>>>)>();

    check_static<(std::is_same_v<count_and_combine_t<tensor_product<euc_vec_space_t, dual<mass_space_t>>>, tensor_product<type_counter<dual<mass_space_t>, 1>, type_counter<euc_vec_space_t, 1>>>)>();

    check_static<(std::is_same_v<count_and_combine_t<tensor_product<dual<mass_space_t>, euc_vec_space_t>>, tensor_product<type_counter<euc_vec_space_t, 1>, type_counter<dual<mass_space_t>, 1>>>)>();

    check_static<(std::is_same_v<count_and_combine_t<tensor_product<euc_vec_space_t, dual<delta_mass_space_t>>>, tensor_product<type_counter<dual<delta_mass_space_t>, 1>, type_counter<euc_vec_space_t, 1>>>)>();

    check_static<(std::is_same_v<count_and_combine_t<tensor_product<dual<delta_mass_space_t>, euc_vec_space_t>>, tensor_product<type_counter<euc_vec_space_t, 1>, type_counter<dual<delta_mass_space_t>, 1>>>)>();
  }

  void physical_value_meta_free_test::test_reduce()
  {
    check_static<(std::is_same_v<reduce_t<count_and_combine_t<tensor_product<si::units::kilogram_t, dual<si::units::kilogram_t>>>>, tensor_product<no_unit_t>>)>();

    check_static<(std::is_same_v<reduce_t<count_and_combine_t<mass_space_t>>, tensor_product<mass_space_t>>)>();

    check_static<(std::is_same_v<reduce_t<count_and_combine_t<dual<mass_space_t>>>, tensor_product<dual<mass_space_t>>>)>();

    check_static<(std::is_same_v<reduce_t<count_and_combine_t<tensor_product<mass_space_t, mass_space_t>>>, tensor_product<mass_space_t, mass_space_t>>)>();

    check_static<(std::is_same_v<reduce_t<count_and_combine_t<tensor_product<mass_space_t, dual<mass_space_t>>>>, tensor_product<euclidean_half_line<implicit_common_arena>>>)>();

    check_static<(std::is_same_v<reduce_t<count_and_combine_t<tensor_product<mass_space_t, dual<delta_mass_space_t>>>>, tensor_product<euclidean_vector_space<1, implicit_common_arena>>>)>();

    check_static<(std::is_same_v<reduce_t<count_and_combine_t<tensor_product<euc_vec_space_t, euc_vec_space_t>>>, tensor_product<euc_vec_space_t>>)>();

    check_static<(std::is_same_v< reduce_t<count_and_combine_t<tensor_product<euc_vec_space_t, dual<euc_vec_space_t>>>>, tensor_product<euc_vec_space_t> >)>("count_and_combine assumes ordering T, deltaT, dual<T>, dual<deltaT>");

    check_static<(std::is_same_v<reduce_t<count_and_combine_t<tensor_product<euc_half_space_t, euc_half_space_t>>>, tensor_product<euc_half_space_t>>)>();

    check_static<(std::is_same_v< reduce_t<count_and_combine_t<tensor_product<euc_half_space_t, dual<euc_half_space_t>>>>, tensor_product<euc_half_space_t> >)>("count_and_combine assumes ordering T, deltaT, dual<T>, dual<deltaT>");

    check_static<(std::is_same_v<reduce_t<count_and_combine_t<tensor_product<euc_half_space_t, euc_vec_space_t>>>, tensor_product<euc_vec_space_t>>)>();

    check_static<(std::is_same_v<reduce_t<count_and_combine_t<tensor_product<euc_vec_space_t, euc_half_space_t>>>, tensor_product<euc_vec_space_t>>)>();

    check_static<(std::is_same_v<reduce_t<count_and_combine_t<tensor_product<euc_half_space_t, dual<euc_vec_space_t>>>>, tensor_product<euc_vec_space_t>>)>();

    check_static<(std::is_same_v<reduce_t<count_and_combine_t<tensor_product<dual<euc_half_space_t>, euc_vec_space_t>>>, tensor_product<euc_vec_space_t>>)>();

    check_static<(std::is_same_v<reduce_t<count_and_combine_t<tensor_product<dual<euc_half_space_t>, dual<euc_vec_space_t>>>>, tensor_product<euc_vec_space_t>>)>();
  }

  void physical_value_meta_free_test::test_simplify()
  {    
    check_static<(std::is_same_v< simplify_t<tensor_product<length_space_t, mass_space_t>, tensor_product<abs_temp_space_t, dual<mass_space_t>>>, reduction<tensor_product<abs_temp_space_t, length_space_t>> >)>("Note assumption that tensor products are already sorted");
  }

  void physical_value_meta_free_test::test_space_reduction()
  {
    check_static<(std::is_same_v<reduction_t<tensor_product<delta_mass_space_t, delta_len_space_t>>, reduction<tensor_product<delta_len_space_t, delta_mass_space_t>>>)>();

    check_static<(std::is_same_v<reduction_t<tensor_product<delta_len_space_t, delta_mass_space_t>>, reduction<tensor_product<delta_len_space_t, delta_mass_space_t>>>)>();

    check_static<(std::is_same_v<reduction_t<tensor_product<length_space_t, mass_space_t>>, reduction<tensor_product<length_space_t, mass_space_t>>>)>();

    check_static<(std::is_same_v<reduction_t<tensor_product<mass_space_t, length_space_t>>, reduction<tensor_product<length_space_t, mass_space_t>>>)>();

    check_static<(std::is_same_v<reduction_t<tensor_product<mass_space_t, euc_half_space_t>>, reduction<tensor_product<mass_space_t>>>)>();

    check_static<(std::is_same_v<reduction_t<tensor_product<euc_half_space_t, length_space_t>>, reduction<tensor_product<length_space_t>>>)>();

    check_static<(std::is_same_v<reduction_t<tensor_product<euc_half_space_t, euc_half_space_t>>, reduction<tensor_product<euc_half_space_t>>>)>();

    check_static<(std::is_same_v<reduction_t<tensor_product<mass_space_t, euc_half_space_t>>, reduction<tensor_product<mass_space_t>>>)>();
      
    check_static<(std::is_same_v< reduction_t<tensor_product<mass_space_t, euc_vec_space_t>>, reduction<tensor_product<mass_space_t, euc_vec_space_t>> >)>("Mass space is not a vector space, so don't reduce away the vector space; note: maths::euclidean is alphabetically before physics::mass");

    check_static<(std::is_same_v<reduction_t<tensor_product<electrical_current_space_t, euc_vec_space_t>>, reduction<tensor_product<electrical_current_space_t>>>)>();

    check_static<(std::is_same_v<reduction_t<tensor_product<euc_vec_space_t, euc_vec_space_t>>, reduction<tensor_product<euc_vec_space_t>>>)>();

    check_static<(std::is_same_v<reduction_t<tensor_product<euc_half_space_t, euc_vec_space_t>>, reduction<tensor_product<euc_vec_space_t>>>)>();

    check_static<(std::is_same_v< reduction_t<tensor_product<mass_space_t, euc_half_space_t>>, reduction<tensor_product<mass_space_t>> >)>("Mass space is not a half space, so reduce away the additional half space");

    check_static<(std::is_same_v< reduction_t<tensor_product<euc_half_space_t, mass_space_t>>, reduction<tensor_product<mass_space_t>> >)>("Mass space is not a half space, so reduce away the additional half space");

    check_static<(std::is_same_v< reduction_t<tensor_product<mass_space_t, dual<euc_half_space_t>>>, reduction<tensor_product<mass_space_t>> >)>("Mass space is not a half space, so reduce away the additional half space");

    check_static<(std::is_same_v< reduction_t<tensor_product<dual<euc_half_space_t>, mass_space_t>>, reduction<tensor_product<mass_space_t>> >)>("Mass space is not a half space, so reduce away the additional half space");
   
    check_static<(std::is_same_v<to_composite_space_t<reduction_t<tensor_product<length_space_t, mass_space_t>>>, composite_space<length_space_t, mass_space_t>>)>();

    check_static<(std::is_same_v<to_composite_space_t<reduction_t<tensor_product<mass_space_t, length_space_t>>>, composite_space<length_space_t, mass_space_t>>)>();

    check_static<(std::is_same_v<to_composite_space_t<reduction_t<tensor_product<mass_space_t, dual<delta_mass_space_t>>>>, euclidean_vector_space<1, implicit_common_arena>>)>();
    
    check_static<(std::is_same_v< reduction_t<tensor_product<delta_abs_temp_space_t, composite_space<delta_len_space_t, delta_mass_space_t>>>, reduction<tensor_product<delta_abs_temp_space_t, delta_len_space_t, delta_mass_space_t>> >)>("Note assumption that tensor products are already sorted");

    check_static<(std::is_same_v< reduction_t<tensor_product<composite_space<delta_len_space_t, delta_mass_space_t>, delta_abs_temp_space_t>>, reduction<tensor_product<delta_abs_temp_space_t, delta_len_space_t, delta_mass_space_t>> >)>("Note assumption that tensor products are already sorted");
    
    check_static<(std::is_same_v< reduction_t<tensor_product<abs_temp_space_t, composite_space<length_space_t, mass_space_t>>>, reduction<tensor_product<abs_temp_space_t, length_space_t, mass_space_t>> >)>();

    check_static<(std::is_same_v< reduction_t<tensor_product<abs_temp_space_t, composite_space<length_space_t, mass_space_t>>>, reduction<tensor_product<abs_temp_space_t, length_space_t, mass_space_t>> >)>();
    
    check_static<(std::is_same_v< to_composite_space_t<reduction_t<tensor_product<composite_space<length_space_t, mass_space_t>, mass_space_t>>>, composite_space<length_space_t, mass_space_t, mass_space_t> >)>();

    check_static<(std::is_same_v< reduction_t< tensor_product< composite_space<delta_len_space_t, delta_mass_space_t>, composite_space<delta_abs_temp_space_t, delta_time_space_t> > >, reduction<tensor_product<delta_abs_temp_space_t, delta_len_space_t, delta_mass_space_t, delta_time_space_t>> >)>();

    
    check_static<(std::is_same_v< reduction_t<tensor_product<composite_space<abs_temp_space_t, length_space_t>, composite_space<electrical_current_space_t, mass_space_t>>>, reduction<tensor_product<abs_temp_space_t, electrical_current_space_t, length_space_t, mass_space_t>> >)>();

    check_static<(std::is_same_v< reduction_t< tensor_product< delta_len_space_t, to_composite_space_t<reduction_t<tensor_product<delta_mass_space_t, composite_space<delta_abs_temp_space_t, delta_time_space_t>>>> > >, reduction<tensor_product<delta_abs_temp_space_t, delta_len_space_t, delta_mass_space_t, delta_time_space_t>> >)>();

    check_static<(std::is_same_v<to_composite_space_t<reduction_t<tensor_product<composite_space<mass_space_t, mass_space_t>, composite_space<dual<mass_space_t>, dual<mass_space_t>>>>>, euclidean_half_line<implicit_common_arena>>)>();

    check_static<(std::is_same_v<to_composite_space_t<reduction_t<tensor_product<composite_space<mass_space_t, mass_space_t>, dual_of_t<composite_space<mass_space_t, mass_space_t>>>>>, euclidean_half_line<implicit_common_arena>>)>();
    
    check_static<(convex_space<to_composite_space_t<reduction_t<tensor_product<mass_space_t, length_space_t>>>>)>();
    check_static<(vector_space<to_composite_space_t<reduction_t<tensor_product<delta_mass_space_t, delta_len_space_t>>>>)>();
    check_static<(vector_space<to_composite_space_t<reduction_t<tensor_product<dual<delta_mass_space_t>, delta_mass_space_t>>>>)>();
  }

  void physical_value_meta_free_test::test_units_reduction()
  {
    using namespace si::units;

    check_static<(std::is_same_v<decltype(no_unit * metre), metre_t>)>();
    check_static<(std::is_same_v<decltype(metre * no_unit), metre_t>)>();
    check_static<(std::is_same_v<decltype(metre / no_unit), metre_t>)>();
    check_static<(std::is_same_v<decltype(no_unit / metre), dual<metre_t>>)>();
    check_static<(std::is_same_v<decltype(no_unit / dual<metre_t>{}), metre_t>)>();

    check_static<(std::is_same_v<reduction_t<tensor_product<metre_t, kilogram_t>>, reduction<tensor_product<kilogram_t, metre_t>>>)>();

    check_static<(std::is_same_v<reduction_t<tensor_product<no_unit_t, no_unit_t>>, reduction<tensor_product<no_unit_t>>>)>();
    check_static<(std::is_same_v<reduction_t<tensor_product<metre_t, no_unit_t>>, reduction<tensor_product<metre_t>>>)>();
    
    check_static<(std::is_same_v<reduction_t<tensor_product<kilogram_t, metre_t>>, reduction<tensor_product<kilogram_t, metre_t>>>)>();

    check_static<(std::is_same_v<to_composite_space_t<reduction_t<tensor_product<metre_t, kilogram_t>>>, composite_unit<kilogram_t, metre_t>>)>();

    check_static<(std::is_same_v<reduction_t<tensor_product<kelvin_t, composite_unit<kilogram_t, metre_t>>>, reduction<tensor_product<kelvin_t, kilogram_t, metre_t>>>)>();

    check_static<(std::is_same_v<to_composite_space_t<reduction_t<tensor_product<composite_unit<coulomb_t, kelvin_t>, composite_unit<kilogram_t, metre_t>>>>, composite_unit<coulomb_t, kelvin_t, kilogram_t, metre_t>>)>();

    check_static<(std::is_same_v<reduction_t<tensor_product<coulomb_t, dual<kelvin_t>>>, reduction<tensor_product<coulomb_t, dual<kelvin_t>>>>)>();

    check_static<(std::is_same_v<reduction_t<tensor_product<dual<kelvin_t>, coulomb_t>>, reduction<tensor_product<coulomb_t, dual<kelvin_t>>>>)>();

    check_static<(std::is_same_v<to_composite_space_t<reduction_t<tensor_product<kelvin_t, dual<kelvin_t>>>>, no_unit_t>)>();
  }
}
