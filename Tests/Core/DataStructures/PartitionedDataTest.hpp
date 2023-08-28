////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "PartitionedDataTestingUtilities.hpp"

namespace sequoia::testing
{
  class partitioned_data_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:
    void test_static_storage();

    template<class Container, std::bidirectional_iterator I>
    void test_generic_iterator_properties();

    void test_iterators();

    template<class T>
    struct traits
    {
      using value_type = T;
      template<class S> using container_type = std::vector<S, std::allocator<S>>;
    };
  };
}
