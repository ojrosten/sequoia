////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

import std;
import sequoia.test_framework;

/** \file */

namespace sequoia::testing
{
  /** \brief The scalars beneath the spaces: which types add and multiply, which of them form
      commutative rings, and which of them cover which.

      Split out of `spaces_meta_free_test`, whose subject begins one level up. Nothing here mentions
      a space; everything here is about `int`, `float`, `std::complex` and the covering relation,
      which is what a space's scalars are drawn from. `test_ring_traits` stayed behind for exactly
      that reason: extracting a ring *from a space* is a question about the space.
   */
  class numeric_rings_meta_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    void test_arithmetic_traits();

    void test_commutative_rings();

    void test_coverings();

    void test_integral_covering_invariants();
  };
}
