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
  class vector_coordinates_test final : public regular_test
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

    template<class Set, class Field, std::floating_point Rep>
      requires maths::identifies_as_field_v<Field>
    void test_real_vec_1_inner_prod();

    template<class Set, class Field, class Rep>
      requires maths::identifies_as_field_v<Field>
    void test_complex_vec_1_inner_prod();
  };
}
