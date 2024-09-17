////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "VecTestingDiagnostics.hpp"

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path vec_false_positive_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void vec_false_positive_test::run_tests()
  {
    {
      using coords = vector_coordinates<my_vec_space<sets::R<1, float>, float, 1>, canonical_basis<sets::R<1, float>, float, 1>>;
      test_vec_1<coords>();
    }

    {
      using coords = euclidean_vector_coordinates<1, float, standard_basis<1, float>>;
      test_vec_1<coords>();
    }
  }

  template<class VecCoords>
  void vec_false_positive_test::test_vec_1()
  {
    using field_t = VecCoords::field_type;
    using array_t = std::array<field_t, 1>;

    VecCoords x{}, y{field_t(1)};
    check(equivalence, report_line(""), x, array_t{1});
    check(equality, report_line(""), x, y);
  }
}
