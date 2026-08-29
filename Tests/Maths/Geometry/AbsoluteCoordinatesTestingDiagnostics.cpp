////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "AbsoluteCoordinatesTestingDiagnostics.hpp"


namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path absolute_coordinates_false_negative_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void absolute_coordinates_false_negative_test::run_tests()
  {
    test_absolute<float , 1>();
    test_absolute<double, 2>();
  }

  template<std::floating_point T, std::size_t D>
  void absolute_coordinates_false_negative_test::test_absolute()
  {
    using space_t      = euclidean_nonnegative_space<D, mathematical_arena>;
    using basis_data_t = canonical_basis_data<D>;
    using coords_t     = coordinates<space_t, basis_data_t, canonical_representation<T, no_bounds<T>>, identity_validator>;

    const auto vals{utilities::make_array<T, D>([](auto) { return T(1); })};
    
    coords_t x{}, y{vals};
    check(equivalence, "", x, vals);
    check(equality, "", x, y);
  }
}
