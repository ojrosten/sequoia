////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AffineCoordinatesTestingDiagnostics.hpp"

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path affine_coordinates_false_negative_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void affine_coordinates_false_negative_test::run_tests()
  {
    {
      using space_t       = my_affine_space<sets::R<1>, commutative_rings::reals<1>, 1>;
      using free_module_t = free_module_type_of_t<space_t>;      
      using basis_t       = general_basis<free_module_t>;
      using rep_t         = canonical_representation<float, no_bounds<float>>;
      using coords_t      = affine_coordinates<space_t, basis_t, rep_t, alice, identity_validator>;

      test_affine_1<coords_t>();
    }

    {
      using basis_t  =  general_basis<euclidean_vector_space<1>>;
      using rep_t    = canonical_representation<float, no_bounds<float>>;
      using coords_t = euclidean_affine_coordinates<1, basis_t, rep_t, alice, identity_validator>;
      test_affine_1<coords_t>();
    }
  }

  template<class AffineCoords>
  void affine_coordinates_false_negative_test::test_affine_1()
  {
    using value_t = AffineCoords::value_type;
    using array_t = std::array<value_t, 1>;

    AffineCoords x{}, y{value_t(1)};
    check(equivalence, "", x, array_t{1});
    check(equality, "", x, y);
  }
}
