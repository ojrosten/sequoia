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
  [[nodiscard]]
  std::filesystem::path affine_coordinates_false_positive_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void affine_coordinates_false_positive_test::run_tests()
  {
    test_affine_1<float, float>();
  }

  template<class Element, maths::field Field>
  void affine_coordinates_false_positive_test::test_affine_1()
  {
    using array_t = std::array<Field, 1>;

    maths::affine_coordinates<my_affine_space<Element, Field, 1>, canonical_basis<Element, Field, 1>, alice> x{}, y{Field(1)};
    check(equivalence, report_line(""), x, array_t{1});
    check(equality, report_line(""), x, y);
  }
}
