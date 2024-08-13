////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "GeometryTestingUtilities.hpp"

namespace sequoia::testing
{
  template<class Origin, maths::vector_space VectorSpace, maths::basis<VectorSpace> Basis>
  struct value_tester<maths::affine_coordinates<Origin, VectorSpace, Basis>> : geometrical_space_tester<maths::affine_coordinates<Origin, VectorSpace, Basis>>
  {};
}
