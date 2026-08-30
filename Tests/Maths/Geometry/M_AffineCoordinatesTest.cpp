////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "M_AffineCoordinatesTest.hpp"

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path m_affine_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void m_affine_coordinates_test::run_tests()
  {
    test_m_affine<sets::Z<1>, commutative_rings::integers<1>, 1, int>();
    test_m_affine<sets::Z<2>, commutative_rings::integers<2>, 2, int>();
  }

  /** An M-affine space is an affine space over a free module rather than a vector
      space. The action of the module remains total, so the arithmetic available on
      points is exactly that of the affine case; the sole difference lies with the
      displacements, which may no longer be divided by a ring element.
   */
  template<class Set, class Ring, std::size_t D, class Rep>
    requires (!maths::identifies_as_field_v<Ring>)
  void m_affine_coordinates_test::test_m_affine()
  {
    using space_t      = my_m_affine_space<Set, Ring, D>;
    using module_t     = free_module_type_of_t<space_t>;
    using basis_data_t = canonical_basis_data<D>;
    using rep_t        = canonical_representation<Rep, no_bounds<to_bounds_value_type_t<Rep>>>;
    using m_affine_t   = m_affine_coordinates<space_t, basis_data_t, rep_t, alice, identity_validator>;

    check_static<(m_affine_space<space_t>)>();
    check_static<(!affine_space<space_t>)>();
    check_static<(!free_module<space_t>)>();
    check_static<(free_module<module_t>)>();
    check_static<(!vector_space<module_t>)>();
    check_static<(basis_data_for<basis_data_t, module_t>)>();
    check_static<(not defines_rank_v<space_t>)>();
    check_static<(dimension_of_v<space_t> == D)>();

    // Identical to the affine table save for one row: displacement / scalar,
    // which needs the ring to be a field. That single difference is the whole
    // content of the distinction between affine and M-affine.
    operator_checks<m_affine_t, operator_expectations{
        .point_plus_point               = admits::no,
        .point_plus_displacement        = admits::yes,
        .point_minus_point              = admits::yes,
        .point_minus_displacement       = admits::yes,
        .point_unary_plus               = admits::yes,
        .point_unary_minus              = admits::no,
        .point_times_scalar             = admits::no,
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

    coordinates_operations<m_affine_t>{*this}.execute();
  }
}
