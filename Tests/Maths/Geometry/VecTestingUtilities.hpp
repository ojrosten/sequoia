////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "GeometryTestingUtilities.hpp"

#include <complex>

namespace sequoia::testing
{
  /*template<std::floating_point Element, maths::field Field>
    requires std::is_same_v<Element, Field>
  struct canonical_basis<Element, Field, 1>
  {
    constexpr static std::array<std::array<Element, 1>, 1> basis_vectors{{1}};
  };

  template<std::floating_point T, maths::field Field>
    requires std::is_same_v<T, Field>
  struct canonical_basis<std::array<T, 2>, Field, 2>
  {
    constexpr static std::array<std::array<T, 2>, 2> basis_vectors{std::array<T, 2>{1, 0}, std::array<T, 2>{0, 1}};
  };*/

 /* template<maths::vector_space VectorSpace, maths::basis<VectorSpace> Basis>
  struct value_tester<maths::affine_coordinates<maths::vector_space_as_affine_space<VectorSpace>, Basis, maths::intrinsic_origin>>
    : geometrical_space_tester<maths::affine_coordinates<maths::vector_space_as_affine_space<VectorSpace>, Basis, maths::intrinsic_origin>>>
  {};*/
}
