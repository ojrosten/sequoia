////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "FreeModuleCoordinatesTest.hpp"

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    struct my_random_set {};
  }

  [[nodiscard]]
  std::filesystem::path free_module_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void free_module_coordinates_test::run_tests()
  {    
    test_free_module<sets::Z<1>   , commutative_rings::integers<1>, int, 1>();
    test_free_module<my_random_set, commutative_rings::integers<1>, int, 1>();
  }

  /*! A free module is an M-affine space over itself, so points and displacements
      coincide. Relative to a vector space, what is lost is division by a scalar,
      the ring not being a field.
   */
  template<class Set, class Ring, class SetRep, std::size_t D>
  void free_module_coordinates_test::test_free_module()
  {
    using free_module_t = my_module<Set, Ring, D>;
    using basis_t       = canonical_basis;

    STATIC_CHECK(!vector_space<free_module_t>);
    STATIC_CHECK(free_module<free_module_t>);
    STATIC_CHECK(m_affine_space<free_module_t>);
    STATIC_CHECK(!affine_space<free_module_t>);
    STATIC_CHECK(std::same_as<free_module_type_of_t<free_module_t>, free_module_t>);
    STATIC_CHECK(basis_data_for<basis_t, free_module_t>);
    
    using module_coords_t = free_module_coordinates<free_module_t, basis_t, canonical_representation<SetRep, no_bounds<SetRep>>, identity_validator>;
    using displacement_value_t = module_coords_t::displacement_coordinates_type::value_type;
    STATIC_CHECK(maths::weak_representation_for<displacement_value_t, Ring>);

    operator_checks<module_coords_t, operator_expectations{
        .point_plus_point               = admits::yes,
        .point_plus_displacement        = admits::yes,
        .point_minus_point              = admits::yes,
        .point_minus_displacement       = admits::yes,
        .point_unary_plus               = admits::yes,
        .point_unary_minus              = admits::yes,
        .point_times_scalar             = admits::yes,
        .point_over_scalar              = admits::no,
        .point_over_point               = admits::no,
        .point_over_displacement        = admits::no,
        .displacement_over_point        = admits::no,
        .displacement_times_scalar      = admits::yes,
        .displacement_over_scalar       = admits::no,
        .displacement_over_displacement = admits::no,
        .displacement_unary_minus       = admits::yes
      }
    >{*this}.execute();

    coordinates_operations<module_coords_t>{*this}.execute();
  }
}
