////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

#include "PartitionedData.hpp"

namespace sequoia::unit_testing
{
  namespace impl
  {
    template<class Logger, class PartitionedData>
    bool check(Logger& logger, const PartitionedData& reference, const PartitionedData& actual, std::string_view description="")
    {

    }

    template<class Logger, class PartitionedData, class T=typename PartitionedData::value_type>
    bool check(Logger& logger, const PartitionedData& data, std::initializer_list<std::initializer_list<T>> refVals, std::string_view description="")
    {
      const auto numElements{std::accumulate(refVals.begin(), refVals.end(), std::size_t{},
                                          [](std::initializer_list<T> partition) { return partition.size();})};

      check_equality(logger, numElements, data.size(), "Number of elements");

      if(check_equality(logger, refVals.size(), data.num_partitions("Number of partitions")))
      {
        for(std::size_t i{}; i<refVals.size(); ++i)
        {
          check_range(logger, refVals[i].begin(), refVals[i].end(), data.begin_partition(i), data.end_partition(i), "Partition " + std::to_string(i));
        }
      }
    }
  }
  
  template<class T, class SharingPolicy, class Traits>
  struct details_checker<data_structures::bucketed_storage<T, SharingPolicy, Traits>>
  {
    using type = data_structures::bucketed_storage<T, SharingPolicy, Traits>;
    
    template<class Logger>
    static void check(Logger& logger, const type& reference, const type& actual, std::string_view description="")
    {
      // TO DO
    }
  };

  template<class T, class SharingPolicy, class Traits>
  struct equivalence_checker<data_structures::bucketed_storage<T, SharingPolicy, Traits>>
  {
    using type = data_structures::bucketed_storage<T, SharingPolicy, Traits>;
    
    template<class Logger>
    static void check(Logger& logger, const type& reference, const type& actual, std::string_view description="")
    {
      // TO DO
    }
  };

  template<class T, class SharingPolicy, class Traits>
  struct details_checker<data_structures::contiguous_storage<T, SharingPolicy, Traits>>
  {
    using type = data_structures::contiguous_storage<T, SharingPolicy, Traits>;
    
    template<class Logger>
    static void check(Logger& logger, const type& reference, const type& actual, std::string_view description="")
    {
      // TO DO
    }
  };

  template<class T, class SharingPolicy, class Traits>
  struct equivalence_checker<data_structures::contiguous_storage<T, SharingPolicy, Traits>>
  {
    using type = data_structures::contiguous_storage<T, SharingPolicy, Traits>;
    
    template<class Logger>
    static void check(Logger& logger, const type& data, std::initializer_list<std::initializer_list<T>> refVals, std::string_view description="")
    {
      impl::check(logger, data, refVals, description);
    }
  };

  template<class T, std::size_t Npartitions, std::size_t Nelements, class IndexType>
  struct details_checker<data_structures::static_contiguous_storage<T, Npartitions, Nelements, IndexType>>
  {
    using type = data_structures::static_contiguous_storage<T, Npartitions, Nelements, IndexType>;
    
    template<class Logger>
    static void check(Logger& logger, const type& reference, const type& actual, std::string_view description="")
    {
      // TO DO
    }
  };

  template<class T, std::size_t Npartitions, std::size_t Nelements, class IndexType>
  struct equivalence_checker<data_structures::static_contiguous_storage<T, Npartitions, Nelements, IndexType>>
  {
    using type = data_structures::static_contiguous_storage<T, Npartitions, Nelements, IndexType>;
    
    template<class Logger>
    static void check(Logger& logger, const type& data, std::initializer_list<std::initializer_list<T>> refVals, std::string_view description="")
    {
      impl::check(logger, data, refVals, description);
    }
  };
}
