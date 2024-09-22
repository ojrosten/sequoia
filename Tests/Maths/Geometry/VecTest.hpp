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
  class vec_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();

    template<class Set, maths::field Field>
    void test_vec_1_orderable();

    template<class Set, maths::field Field>
    void test_vec_1_unorderable();

    template<class Set, maths::field Field>
    void test_vec_2();

    template<class Set, std::floating_point Field>
    void test_real_vec_1_inner_prod();

    template<class Set, class Field>
      requires is_complex_v<Field>
    void test_complex_vec_1_inner_prod();

    void test_masses();
  };
}
