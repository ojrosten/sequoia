////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "PartitionedDataTestingUtilities.hpp"
#include "RegularAllocationTestCore.hpp"

namespace sequoia::testing
{
  template
  <
    class T,
    class SharingPolicy,    
    bool PropagateCopy=true,
    bool PropagateMove=true,
    bool PropagateSwap=true
  >
  struct custom_bucketed_storage_traits
  {
    constexpr static bool throw_on_range_error{true};

    template<class S>
    using allocator_template = shared_counting_allocator<S, PropagateCopy, PropagateMove, PropagateSwap>;

    template<class S>
    using allocator_type
      = std::scoped_allocator_adaptor<
          allocator_template<S>,
          allocator_template<typename S::value_type>
        >;

    template<class S>
    using container_type = std::vector<S, allocator_template<S>>;
    
    template<class S>
    using buckets_type = std::vector<container_type<S>, allocator_type<container_type<S>>>;
  };

  template
  <
    class T,
    class SharingPolicy,    
    bool PropagateCopy=true,
    bool PropagateMove=true,
    bool PropagateSwap=true
  >
  struct custom_partitioned_sequence_traits
  {
    constexpr static bool static_storage_v{false};
    constexpr static bool throw_on_range_error{true};

    using index_type = std::size_t;
    using partition_index_type = std::size_t;
    using partitions_type
      = maths::monotonic_sequence<
          partition_index_type,
          std::greater<partition_index_type>,
          std::vector<partition_index_type, shared_counting_allocator<partition_index_type, PropagateCopy, PropagateMove, PropagateSwap>>
        >;
    
    using partitions_allocator_type = typename partitions_type::allocator_type;
      
    template<class S> using container_type = std::vector<S, shared_counting_allocator<S, PropagateCopy, PropagateMove, PropagateSwap>>;
  };
}
