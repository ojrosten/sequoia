////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "GeometryTestingUtilities.hpp"

namespace sequoia::testing
{
  /** \brief Coordinates on vector spaces over the complex numbers, including the case of
      \f$\mathbb{C}\f$ regarded as a two-dimensional space over \f$\mathbb{R}\f$.
   */
  class complex_vector_coordinates_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    template<class Set, class Field, std::size_t D, class Rep>
      requires maths::identifies_as_field_v<Field>
    void test_vec();

    template<class Set, class Field, class Rep>
      requires maths::identifies_as_field_v<Field>
    void test_complex_vec_1_inner_prod();
  };
}
