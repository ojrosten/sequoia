////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"

namespace sequoia::testing
{
  class algorithms_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const noexcept final;
  private:
    void run_tests() final;

    template<class Stability>
    void sort_basic_type(Stability stability);

    template<class Stability>
    void sort_faithful_wrapper(Stability stability);

    template<class Stability>
    void sort_partial_edge(Stability stability);

    void stable_sort_stability();

    void cluster_basic_type();
  };
}
