////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Classes implementing the concept of a linearly partitioned sequence of data.
 */

#include "PartitionedData.hpp"
#include "LinearSequence.hpp"

namespace sequoia::data_structures
{
  template<class T, std::size_t Npartitions, std::size_t NelementsPerPartition, class IndexType>
  struct static_linearly_partitioned_sequence_traits
  {
    constexpr static bool static_storage_v{true};
    constexpr static bool throw_on_range_error{true};
    constexpr static std::size_t num_partitions_v{Npartitions};
    constexpr static std::size_t num_elements_v{Npartitions * NelementsPerPartition};
      
    using index_type           = IndexType;
    using partition_index_type = std::make_unsigned_t<IndexType>;      
    using partitions_type      = maths::static_linear_sequence<partition_index_type, NelementsPerPartition, NelementsPerPartition, partition_index_type>;
      
    template<class S> using container_type = std::array<S, num_elements_v>;
  };

  template<class T, std::size_t Npartitions, std::size_t NelementsPerPartition, class IndexType=std::size_t>
  class static_linearly_partitioned_sequence :
    public partitioned_sequence_base<T, ownership::independent<T>, static_linearly_partitioned_sequence_traits<T, Npartitions, NelementsPerPartition, IndexType>>
  {
  public:
    using partitioned_sequence_base<
      T,
      ownership::independent<T>,
      static_linearly_partitioned_sequence_traits<T, Npartitions, NelementsPerPartition, IndexType>
    >::partitioned_sequence_base;
  };
}
