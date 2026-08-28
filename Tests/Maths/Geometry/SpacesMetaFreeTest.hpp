////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class spaces_meta_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    void test_arithmetic_traits();

    void test_coverings();

    void test_integral_covering_invariants();

    void test_structure_trait();

    void test_set_trait();

    void test_rank_traits();

    void test_origin_and_orthant_traits();

    void test_ring_traits();

    void test_commutative_rings();

    void test_free_module_traits();

    void test_basis_traits();

    void test_spaces_dag();

    void test_derived_spaces();

    void test_representation_traits();

    void test_validator_traits();
  };
}
