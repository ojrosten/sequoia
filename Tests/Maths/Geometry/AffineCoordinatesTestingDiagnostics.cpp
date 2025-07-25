////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AffineCoordinatesTestingDiagnostics.hpp"

namespace
{
  struct alice {};
}

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
      using coords = affine_coordinates<my_affine_space<float, float, 1>, canonical_basis<float, float, 1>, alice>;
      test_affine_1<coords>();
    }

    {
      using coords = euclidean_affine_coordinates<float, 1, canonical_right_handed_basis<euclidean_vector_space<float, 1>>, alice>;
      test_affine_1<coords>();
    }
  }

  template<class AffineCoords>
  void affine_coordinates_false_negative_test::test_affine_1()
  {
    using field_t = AffineCoords::commutative_ring_type;
    using array_t = std::array<field_t, 1>;

    AffineCoords x{}, y{field_t(1)};
    check(equivalence, "", x, array_t{1});
    check(equality, "", x, y);
  }
}
