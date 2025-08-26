////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AffineCoordinatesTest.hpp"

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    template<affine_space A>
    struct alice
    {
      using space_type = A;
    };

    template<affine_space A>
    struct bob
    {
      using space_type = A;
    };

    template<class From, class To>
    struct coordinate_transform;

    template<affine_space A, basis_for<free_module_type_of_t<A>> Basis>
    struct coordinate_transform<affine_coordinates<A, Basis, alice<A>>, affine_coordinates<A, Basis, bob<A>>>
    {
      using disp_type = affine_coordinates<A, Basis, alice<A>>::displacement_coordinates_type;

      disp_type displacement{};

      explicit coordinate_transform(const disp_type& d)
        : displacement{d}
      {}
      
      [[nodiscard]]
      constexpr affine_coordinates<A, Basis, bob<A>>
        operator()(const affine_coordinates<A, Basis, alice<A>>& c) const noexcept
      {
        
        return affine_coordinates<A, Basis, bob<A>>{(c + displacement).values()};
      }
    };
  }

  [[nodiscard]]
  std::filesystem::path affine_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void affine_coordinates_test::run_tests()
  {
    test_affine<float, float, 1>();
    test_affine<double, double, 1>();
    test_affine<std::complex<float>, std::complex<float>, 1>();
    test_affine<std::complex<float>, float, 2>();
  }

  template<class Element, maths::weak_field Field, std::size_t D>
  void affine_coordinates_test::test_affine()
  {
    using space_t  = my_affine_space<Element, Field, D>;
    using basis_t  = canonical_basis<Element, Field, D>;
    using affine_t = affine_coordinates<space_t, basis_t, alice<space_t>>;
    using delta_t  = affine_t::displacement_coordinates_type;
    using value_t  = Field;
    STATIC_CHECK(!can_multiply<affine_t, value_t>);
    STATIC_CHECK(!can_divide<affine_t, value_t>);
    STATIC_CHECK(!can_divide<affine_t, affine_t>);
    STATIC_CHECK(!can_divide<affine_t, delta_t>);
    STATIC_CHECK(!can_divide<delta_t, affine_t>);
    STATIC_CHECK(!can_divide<delta_t, delta_t>);
    STATIC_CHECK(!can_add<affine_t, affine_t>);
    STATIC_CHECK(can_add<affine_t, delta_t>);
    STATIC_CHECK(can_subtract<affine_t, affine_t>);
    STATIC_CHECK(can_subtract<affine_t, delta_t>);
    STATIC_CHECK(has_unary_plus<affine_t>);
    STATIC_CHECK(!has_unary_minus<affine_t>);
    
    coordinates_operations<affine_t>{*this}.execute();

    using affine2_t = affine_coordinates<space_t, basis_t, bob<space_t>>;
    affine2_t bob_coords{coordinate_transform<affine_t, affine2_t>{delta_t{Field{-1.0}}}(affine_t{})};

    check(equality, "", bob_coords, affine2_t{Field{-1.0}});
  }
}
