////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <execution>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

export module sequoia.core.data_structures:StaticLinearlyPartitionedSequence;

import :PartitionedData;
import :PartitionedDataDetails;
export import sequoia.algorithms;
export import sequoia.core.container_utilities;
export import sequoia.core.meta;
export import sequoia.maths.sequences;
export import sequoia.platform_specific;

/** \file
    \brief Classes implementing the concept of a linearly partitioned sequence of data.
 */


export namespace sequoia::data_structures
{
  template<std::integral IndexType, std::size_t Npartitions, std::size_t NelementsPerPartition>
  struct static_partitions_maker<maths::static_linear_sequence<IndexType, NelementsPerPartition, NelementsPerPartition, Npartitions, IndexType>>
  {
    using partitions_type = maths::static_linear_sequence<IndexType, NelementsPerPartition, NelementsPerPartition, Npartitions, IndexType>;

    template<class T>
    constexpr static partitions_type make_partitions(std::initializer_list<std::initializer_list<T>>)
    {
      return {};
    }
  };

  template<class T, std::size_t Npartitions, std::size_t NelementsPerPartition, std::integral IndexType=std::size_t>
  using static_linearly_partitioned_sequence = static_partitioned_sequence<T, Npartitions, Npartitions*NelementsPerPartition, maths::static_linear_sequence<IndexType, NelementsPerPartition, NelementsPerPartition, Npartitions, IndexType>>;
}
