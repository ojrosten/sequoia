////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class graph_constraints_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:
    void test_copyability();

    void test_weight_update_constraints();

    void test_join_constraints();

    void test_mutator_constraints();

    template<class EdgeStorageConfig>
    void test_mutation_results();

    void test_shared_move_only_weights();

    void test_move_only_meta_data();

    void test_shared_weight_copies();

    void test_mutation_by_member_functions();
  };
}
