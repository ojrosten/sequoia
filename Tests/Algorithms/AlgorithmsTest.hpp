////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
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
  class algorithms_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:
    void algorithm_constraints();

    void permute_basic_type();

    void permute_swap_count();

    void permute_leaves_destinations_unchanged();

    void permute_allocation_count();

    template<class Form>
    void make_heap_by_second_element(Form form);

    void bubble_down_after_removing_the_top();

    template<class Form>
    void stable_sort_basic_type(Form form);

    template<class Form>
    void stable_sort_partial_edge(Form form);

    template<class Form>
    void stable_sort_by_second_element(Form form);

    template<class Form>
    void stable_sort_stability(Form form);

    template<class Form>
    void stable_sort_with_buffer(Form form);

    template<class Form>
    void cluster_basic_type(Form form);

    template<class Form>
    void cluster_partial_edge(Form form);

    template<class Form>
    void cluster_by_first_element(Form form);

    void permute_refuses_non_permutations();
  };
}
