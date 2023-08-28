////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "PartitionedDataTestingUtilities.hpp"
#include "sequoia/TestFramework/RegularAllocationTestCore.hpp"

namespace sequoia::testing
{
  template
  <
    class T,
    bool PropagateCopy=true,
    bool PropagateMove=false,
    bool PropagateSwap=false
  >
  struct custom_bucketed_storage_generator
  {
    constexpr static bool throw_on_range_error{true};

    using value_type = T;

    template<class S>
    using allocator_template = shared_counting_allocator<S, PropagateCopy, PropagateMove, PropagateSwap>;

    template<class S>
    using allocator_type
      = std::scoped_allocator_adaptor<
          allocator_template<S>,
          allocator_template<typename S::value_type>
        >;

    using bucket_type = std::vector<T, allocator_template<T>>;
    using storage_type = data_structures::bucketed_sequence<T, std::vector<bucket_type, allocator_type<bucket_type>>>;

    template<class S>
    using container_type = std::vector<S, allocator_template<S>>;

    template<class S>
    using buckets_type = std::vector<container_type<S>, allocator_type<container_type<S>>>;
  };

  template<class Storage>
  struct bucket_alloc_getter
  {
    using allocator = typename Storage::allocator_type;
    using alloc_equivalence_class = allocation_equivalence_classes::container_of_values<allocator>;

    [[nodiscard]]
    allocator operator()(const Storage& s) const
    {
      return s.get_allocator();
    }
  };

  template<class Storage>
  struct contiguous_alloc_getter
  {
    using allocator = typename Storage::allocator_type;
    using alloc_equivalence_class = allocation_equivalence_classes::container_of_values<allocator>;

    [[nodiscard]]
    allocator operator()(const Storage& s) const
    {
      return s.get_allocator();
    }
  };

  template<class Storage>
  struct partitions_alloc_getter
  {
    using allocator = typename Storage::partitions_allocator_type;
    using alloc_equivalence_class = allocation_equivalence_classes::container_of_values<allocator>;

    [[nodiscard]]
    allocator operator()(const Storage& s) const
    {
      return s.get_partitions_allocator();
    }
  };
}
