////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "PartitionedDataTestingUtilities.hpp"

#include <experimental/memory_resource>

namespace sequoia::unit_testing
{  
  class partitioned_data_test : public unit_test
  {
  public:
    using unit_test::unit_test;

  private:      
    void run_tests() override;

    void test_storage();
      
    template <class Storage> Storage test_generic_storage();

    void test_static_storage();

    template<class T, class SharingPolicy, bool ThrowOnRangeError>
    void test_contiguous_capacity();

    template<class T, class SharingPolicy, bool ThrowOnRangeError>
    void test_bucketed_capacity();
      
    template<class Traits, template<class> class SharingPolicy, template<class> class ReferencePolicy>
    void test_generic_iterator_properties();
      
    template<template<class> class SharingPolicy> void test_iterators();

    struct traits
    {
      template<class S> using container_type = std::vector<S, std::allocator<S>>;
    };

    template<class T, class SharingPolicy> struct bucketed_pmr_storage_traits
    {
      constexpr static bool throw_on_range_error{true};

      template<class S> using buckets_type   = std::vector<S, std::experimental::pmr::polymorphic_allocator<S>>; 
      template<class S> using container_type = std::vector<S, std::experimental::pmr::polymorphic_allocator<S>>; 
    };

    template<class T, class SharingPolicy> struct contiguous_pmr_storage_traits
    {
      constexpr static bool static_storage_v{false};
      constexpr static bool throw_on_range_error{true};

      using index_type = std::size_t;
      using partition_index_type = std::size_t;

      using partitions_container = std::vector<partition_index_type, std::experimental::pmr::polymorphic_allocator<partition_index_type>>;
      
      using partitions_type = maths::monotonic_sequence<partition_index_type, std::greater<partition_index_type>, partitions_container>;
      
      template<class S> using container_type = std::vector<S, std::experimental::pmr::polymorphic_allocator<S>>;
    };
  };
}
