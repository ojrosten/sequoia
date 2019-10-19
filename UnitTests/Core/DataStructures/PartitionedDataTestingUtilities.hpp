////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"
#include "UnitTestUtilities.hpp"

#include "PartitionedData.hpp"

namespace sequoia::unit_testing
{  
  namespace impl
  {
    template<class Logger, class PartitionedData>
    void check_details(std::string_view description, Logger& logger, const PartitionedData& data, const PartitionedData& prediction)
    {
      check_equality(combine_messages(description, "Size different"), logger, data.size(), prediction.size());
      if(check_equality(combine_messages(description, "Number of partitions different"), logger, data.num_partitions(), prediction.num_partitions()))
      {
        for(std::size_t i{}; i<prediction.num_partitions(); ++i)
        {
          const std::string message{combine_messages(description,"Partition " + std::to_string(i))};
          if(check_range(combine_messages(message, "iterator (const)"), logger, data.begin_partition(i), data.end_partition(i), prediction.begin_partition(i), prediction.end_partition(i)))
          {
            for(int64_t j{}; j<distance(prediction.begin_partition(i), prediction.end_partition(i)); ++j)
            {
              check_equality(combine_messages(message,"[] (const)"), logger, data[i][j], prediction[i][j]);
            }
          }
          
          check_range(combine_messages(message, "r_iterator (const)"), logger, data.rbegin_partition(i), data.rend_partition(i), prediction.rbegin_partition(i), prediction.rend_partition(i));
          check_range(combine_messages(message, "c_iterator"), logger, data.cbegin_partition(i), data.cend_partition(i), prediction.cbegin_partition(i), prediction.cend_partition(i));
          check_range(combine_messages(message, "cr_iterator"), logger, data.crbegin_partition(i), data.crend_partition(i), prediction.crbegin_partition(i), prediction.crend_partition(i));

          auto& r{const_cast<PartitionedData&>(prediction)};
          auto& d{const_cast<PartitionedData&>(data)};
          if(check_range(combine_messages(message, "iterator"), logger, d.begin_partition(i), d.end_partition(i), r.begin_partition(i), r.end_partition(i)))
          {
            for(int64_t j{}; j<distance(r.begin_partition(i), r.end_partition(i)); ++j)
            {
              check_equality(combine_messages(message,"[]"), logger, d[i][j], r[i][j]);
            }
          }
          
          check_range(combine_messages(message, "r_iterator"), logger, d.rbegin_partition(i), d.rend_partition(i), r.rbegin_partition(i), r.rend_partition(i));
        }
      }
    }

    template<class Logger, class PartitionedData, class T=typename PartitionedData::value_type>
    void check_equivalence(std::string_view description, Logger& logger, const PartitionedData& data, std::initializer_list<std::initializer_list<T>> prediction)
    {
      const auto numElements{std::accumulate(prediction.begin(), prediction.end(), std::size_t{},
                                             [](std::size_t val, std::initializer_list<T> partition) { return val += partition.size();})};

      check_equality(combine_messages(description, "Number of elements"), logger, data.size(), numElements);

      if(check_equality(combine_messages(description, "Number of partitions"), logger, data.num_partitions(), prediction.size()))
      {
        for(std::size_t i{}; i<prediction.size(); ++i)
        {
          const std::string message{combine_messages(description, "Partition " + std::to_string(i))};
          check_range(message + ": iterator", logger, data.begin_partition(i), data.end_partition(i), (prediction.begin() + i)->begin(), (prediction.begin() + i)->end());

          check_range(message + ": riterator", logger, data.rbegin_partition(i), data.rend_partition(i), std::rbegin(*(prediction.begin() + i)), std::rend(*(prediction.begin() + i)));
        }
      }
    }
  }
  
  template<class T, class SharingPolicy, class Traits>
  struct detailed_equality_checker<data_structures::bucketed_storage<T, SharingPolicy, Traits>>
  {
    using type = data_structures::bucketed_storage<T, SharingPolicy, Traits>;
    
    template<class Logger>
    static void check(std::string_view description, Logger& logger, const type& data, const type& prediction)
    {
      impl::check_details(description, logger, data, prediction);
    }
  };

  template<class T, class SharingPolicy, class Traits>
  struct equivalence_checker<data_structures::bucketed_storage<T, SharingPolicy, Traits>>
  {
    using type = data_structures::bucketed_storage<T, SharingPolicy, Traits>;
    
    template<class Logger>
    static void check(std::string_view description, Logger& logger, const type& data, std::initializer_list<std::initializer_list<T>> prediction)
    {
      impl::check_equivalence(description, logger, data, prediction);
    }
  };

  template<class T, class SharingPolicy, class Traits>
  struct detailed_equality_checker<data_structures::contiguous_storage<T, SharingPolicy, Traits>>
  {
    using type = data_structures::contiguous_storage<T, SharingPolicy, Traits>;
    
    template<class Logger>
    static void check(std::string_view description, Logger& logger, const type& data, const type& prediction)
    {
      impl::check_details(description, logger, data, prediction);
    }
  };

  template<class T, class SharingPolicy, class Traits>
  struct equivalence_checker<data_structures::contiguous_storage<T, SharingPolicy, Traits>>
  {
    using type = data_structures::contiguous_storage<T, SharingPolicy, Traits>;
    using equivalent_type = std::initializer_list<std::initializer_list<T>>;
    
    template<class Logger>
    static void check(std::string_view description, Logger& logger, const type& data, equivalent_type prediction)
    {
      impl::check_equivalence(description, logger, data, prediction);
    }
  };

  template<class T, std::size_t Npartitions, std::size_t Nelements, class IndexType>
  struct detailed_equality_checker<data_structures::static_contiguous_storage<T, Npartitions, Nelements, IndexType>>
  {
    using type = data_structures::static_contiguous_storage<T, Npartitions, Nelements, IndexType>;
    
    template<class Logger>
    static void check(std::string_view description, Logger& logger, const type& data, const type& prediction)
    {
      impl::check_details(description, logger, data, prediction);
    }
  };

  template<class T, std::size_t Npartitions, std::size_t Nelements, class IndexType>
  struct equivalence_checker<data_structures::static_contiguous_storage<T, Npartitions, Nelements, IndexType>>
  {
    using type = data_structures::static_contiguous_storage<T, Npartitions, Nelements, IndexType>;
    
    template<class Logger>
    static void check(std::string_view description, Logger& logger, const type& data, std::initializer_list<std::initializer_list<T>> prediction)
    {
      impl::check_equivalence(description, logger, data, prediction);
    }
  };
  
  // Custom allocator traits

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

    template<class S> using buckets_type   = std::vector<S, shared_counting_allocator<S, PropagateCopy, PropagateMove, PropagateSwap>>; 
    template<class S> using container_type = std::vector<S, shared_counting_allocator<S, true, true, true>>; 
  };

  template
  <
    class T,
    class SharingPolicy,    
    bool PropagateCopy=true,
    bool PropagateMove=true,
    bool PropagateSwap=true
  >
  struct custom_contiguous_storage_traits
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
