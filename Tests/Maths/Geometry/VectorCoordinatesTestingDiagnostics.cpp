////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "VectorCoordinatesTestingDiagnostics.hpp"

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path vector_coordinates_false_negative_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void vector_coordinates_false_negative_test::run_tests()
  {
    {
      using basis_t  = canonical_basis<sets::R<1>, float, 1>;
      using coords_t = vector_coordinates<my_vec_space<sets::R<1>, float, 1>, basis_t, canonical_representation<no_bounds<float>>>;
      test_vec_1<coords_t>();
    }

    {
      test_vec_1<vec_coords<float, 1>>();
    }
  }

  template<class VecCoords>
  void vector_coordinates_false_negative_test::test_vec_1()
  {
    using field_t = VecCoords::commutative_ring_type;
    using array_t = std::array<field_t, 1>;

    VecCoords x{}, y{field_t(1)};
    check(equivalence, "", x, array_t{1});
    check(equality, "", x, y);
  }
}
