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

  [[nodiscard]]
  std::filesystem::path free_module_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void free_module_coordinates_test::run_tests()
  {    
    test_free_module<sets::Z<1>, int, 1>();
    test_free_module<sets::N_0<1>, unsigned, 1>();
  }

  template<class Set, maths::weak_commutative_ring Ring, std::size_t D>
  void free_module_coordinates_test::test_free_module()
  {
    using free_module_t = my_free_module<Set, Ring, D>;
    using basis_t       = canonical_free_module_basis<Set, Ring, D>;
    STATIC_CHECK(!vector_space<free_module_t>);
    STATIC_CHECK(free_module<free_module_t>);
    STATIC_CHECK(std::same_as<free_module_type_of_t<free_module_t>, free_module_t>);
    STATIC_CHECK(basis_for<basis_t, free_module_t>);
    
    using module_coords_t = free_module_coordinates<free_module_t, basis_t, canonical_representation<no_bounds<Ring>>, identity_validator>;
    coordinates_operations<module_coords_t>{*this}.execute();
  }
}
