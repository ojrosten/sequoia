////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "VectorCoordinatesTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <complex>

namespace sequoia::testing
{
  using namespace maths;

  // TO DO: move this to physics examples
  namespace
  {
    template<std::size_t D, std::floating_point T>
    struct world_displacements {};

    template<std::size_t D, std::floating_point T, class Atlas>
    struct world_vector_space
    {
      using set_type          = world_displacements<D, T>;
      using field_type        = T;
      using atlas_type        = Atlas;
      using vector_space_type = world_vector_space;
      constexpr static std::size_t dimension{D};

      template<maths::basis Basis>
        requires is_orthonormal_basis_v<Basis>
      [[nodiscard]]
      // We want Units^2
      friend constexpr field_type inner_product(const maths::vector_coordinates<world_vector_space, Basis>& lhs, const maths::vector_coordinates<world_vector_space, Basis>& rhs)
      {
        return std::ranges::fold_left(std::views::zip(lhs.values(), rhs.values()), field_type{}, [](field_type f, const auto& z){ return f + std::get<0>(z) * std::get<1>(z); });
      }
    };

    template<std::size_t D, std::floating_point T, class Units>
    struct world_affine_space
    {
      using set_type = sets::R<D>;
      using vector_space_type = world_vector_space<D, T, Units>;
    };

    template<std::size_t D, std::floating_point T, class Units, basis Basis, class Origin>
    using world_affine_coordinates = affine_coordinates<world_affine_space<D, T, Units>, Basis, Origin>;

    template<std::size_t D, std::floating_point T, class Units, basis Basis>
    using world_vector_coordinates = vector_coordinates<world_vector_space<D, T, Units>, Basis>;
  }

  [[nodiscard]]
  std::filesystem::path vector_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  } 

  void vector_coordinates_test::run_tests()
  {
    test_vec<sets::R<1>, float, 1>();
    test_vec<sets::R<1>, double, 1>();
    test_vec<sets::C<1>, std::complex<float>, 1>();

    test_vec<sets::R<2>, float, 2>();
    test_vec<sets::C<2>, std::complex<double>, 2>();
    test_vec<sets::C<1>, double, 2>(); // Complex numbers over the reals

    test_real_vec_1_inner_prod<sets::R<1>, float>();
    test_complex_vec_1_inner_prod<sets::C<1>, std::complex<double>>();

    test_free_module<sets::Z<1>, int, 1>();
    test_free_module<sets::Z_0<1>, unsigned long, 1>();
  }

  template<class Set, maths::weak_commutative_ring Ring, std::size_t D>
  void vector_coordinates_test::test_free_module()
  {
    using free_module_t = my_free_module<Set, Ring, D>;
    STATIC_CHECK(!vector_space<free_module_t>);
    using module_t      = free_module_coordinates<free_module_t, canonical_free_module_basis<Set, Ring, D>>;
    coordinates_operations<module_t>{*this}.execute();
  }

  template<class Set, maths::weak_field Field, std::size_t D>
  void vector_coordinates_test::test_vec()
  {
    using vec_space_t = my_vec_space<Set, Field, D>;
    using vec_t       = vector_coordinates<vec_space_t, canonical_basis<Set, Field, D>>;
    coordinates_operations<vec_t>{*this}.execute();

    static_assert(vector_space<direct_product<vec_space_t, vec_space_t>>);
    static_assert(!vector_space<direct_product<vec_t, vec_t>>);
    static_assert(vector_space<direct_product<direct_product<vec_space_t, vec_space_t>, vec_space_t>>);
  }

  template<class Set, std::floating_point Field>
  void vector_coordinates_test::test_real_vec_1_inner_prod()
  {
    using vec_t = vector_coordinates<my_vec_space<Set, Field, 1>, canonical_basis<Set, Field, 1>>;

    static_assert(basis_for<canonical_basis<Set, Field, 1>, my_vec_space<Set, Field, 1>>);

    check(equality, "", inner_product(vec_t{}, vec_t{Field(1)}), Field{});
    check(equality, "", inner_product(vec_t{Field(1)}, vec_t{}), Field{});
    check(equality, "", inner_product(vec_t{Field(-1)}, vec_t{Field(1)}), Field{-1});
    check(equality, "", inner_product(vec_t{Field(1)}, vec_t{Field(-1)}), Field{-1});
    check(equality, "", inner_product(vec_t{Field(1)}, vec_t{Field(1)}), Field{1});
    check(equality, "", inner_product(vec_t{Field(-7)}, vec_t{Field(42)}), Field{-294});
  }

  template<class Set, class Field>
    requires is_complex_v<Field>
  void vector_coordinates_test::test_complex_vec_1_inner_prod()
  {
    using vec_t = vector_coordinates<my_vec_space<Set, Field, 1>, canonical_basis<Set, Field, 1>>;

    check(equality, "", inner_product(vec_t{Field(1, 1)}, vec_t{Field(1, 1)}), Field{2});
    check(equality, "", inner_product(vec_t{Field(1, -1)}, vec_t{Field(1, 1)}), Field{0, 2});
  }
}
