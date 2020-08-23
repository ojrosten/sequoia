////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "PartitionedDataTestingUtilities.hpp"

namespace sequoia::testing
{
  class partitioned_data_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    void test_storage();
      
    template <class Storage> Storage test_generic_storage();

    void test_static_storage();

    template<class T, class Handler, bool ThrowOnRangeError>
    void test_contiguous_capacity();

    template<class T, class Handler, bool ThrowOnRangeError>
    void test_bucketed_capacity();
      
    template<class Traits, template<class> class Handler, template<class> class ReferencePolicy>
    void test_generic_iterator_properties();
      
    template<template<class> class Handler> void test_iterators();

    struct traits
    {
      template<class S> using container_type = std::vector<S, std::allocator<S>>;
    };
  };
}
