////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

// TO DO: rename test --> Integral

#include "FreeModuleCoordinatesTest.hpp"

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    template<class Set, class Ring, std::size_t D>
    struct my_free_module
    {
      using set_type               = Set;
      using commutative_ring_type  = Ring;
      using structure              = free_module_tag_t;
      using admits_canonical_basis = std::true_type;
      constexpr static std::size_t dimension{D};
    };

    template<class Set, class Ring, std::size_t D>
    struct canonical_free_module_basis
    {
      using is_basis         = std::true_type;
      using free_module_type = my_free_module<Set, Ring, D>;
    };

    template<class Set, class Ring, std::size_t D>
    struct my_convex_space
    {
      constexpr static std::size_t dimension{D};
      using set_type              = Set;
      using free_module_type      = my_free_module<Set, Ring, D>;
      using structure             = convex_space_tag_t;
      using distinguished_origin  = std::true_type;
      using non_negative_orthant  = std::true_type;
    };

    struct my_random_set {};
  }

  [[nodiscard]]
  std::filesystem::path free_module_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void free_module_coordinates_test::run_tests()
  {    
    test_free_module<sets::Z<1>   , commutative_rings::integers<1>, int     , 1>();
    test_free_module<my_random_set, commutative_rings::integers<1>, int     , 1>();
    test_convex     <sets::N_0<1> , commutative_rings::integers<1>, unsigned, 1>();
  }

  template<class Set, class Ring, class SetRep, std::size_t D>
  void free_module_coordinates_test::test_free_module()
  {
    using free_module_t = my_free_module<Set, Ring, D>;
    using basis_t       = canonical_free_module_basis<Set, Ring, D>;
    STATIC_CHECK(!vector_space<free_module_t>);
    STATIC_CHECK(free_module<free_module_t>);
    STATIC_CHECK(std::same_as<free_module_type_of_t<free_module_t>, free_module_t>);
    STATIC_CHECK(basis_for<basis_t, free_module_t>);
    
    using module_coords_t = free_module_coordinates<free_module_t, basis_t, canonical_representation<SetRep, no_bounds<SetRep>>, identity_validator>;
    using displacement_value_t = module_coords_t::displacement_coordinates_type::value_type;
    STATIC_CHECK(maths::weak_representation_for<displacement_value_t, Ring>);
    
    coordinates_operations<module_coords_t>{*this}.execute();
  }

  template<class Set, class Ring, class SetRep, std::size_t D>
  void free_module_coordinates_test::test_convex()
  {
    using space_t  = my_convex_space<Set, Ring, D>;
    using basis_t  = canonical_free_module_basis<Set, Ring, D>;
    using coords_t = coordinates<space_t, basis_t, canonical_representation<SetRep, half_line_bounds<SetRep>>, throwing_validator>;
    using displacement_value_t = coords_t::displacement_coordinates_type::value_type;
    STATIC_CHECK(maths::weak_representation_for<displacement_value_t, Ring>);

    coordinates_operations<coords_t>{*this}.execute();
  }
}
