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
    void check_details(Logger& logger, const PartitionedData& data, const PartitionedData& prediction, std::string_view description="")
    {
      check_equality(logger, data.size(), prediction.size(), concat_messages(description, "Size different"));
      if(check_equality(logger, data.num_partitions(), prediction.num_partitions(), concat_messages(description, "Number of partitions different")))
      {
        for(std::size_t i{}; i<prediction.num_partitions(); ++i)
        {
          const std::string message{concat_messages(description,"Partition " + std::to_string(i))};
          if(check_range(logger, data.begin_partition(i), data.end_partition(i), prediction.begin_partition(i), prediction.end_partition(i), concat_messages(message, "iterator (const)")))
          {
            for(int64_t j{}; j<distance(prediction.begin_partition(i), prediction.end_partition(i)); ++j)
            {
              check_equality(logger, data[i][j], prediction[i][j], concat_messages(message,"[] (const)"));
            }
          }
          
          check_range(logger, data.rbegin_partition(i), data.rend_partition(i), prediction.rbegin_partition(i), prediction.rend_partition(i), concat_messages(message, "r_iterator (const)"));
          check_range(logger, data.cbegin_partition(i), data.cend_partition(i), prediction.cbegin_partition(i), prediction.cend_partition(i), concat_messages(message, "c_iterator"));
          check_range(logger, data.crbegin_partition(i), data.crend_partition(i), prediction.crbegin_partition(i), prediction.crend_partition(i), concat_messages(message, "cr_iterator"));

          auto& r{const_cast<PartitionedData&>(prediction)};
          auto& d{const_cast<PartitionedData&>(data)};
          if(check_range(logger, d.begin_partition(i), d.end_partition(i), r.begin_partition(i), r.end_partition(i), concat_messages(message, "iterator")))
          {
            for(int64_t j{}; j<distance(r.begin_partition(i), r.end_partition(i)); ++j)
            {
              check_equality(logger, d[i][j], r[i][j], concat_messages(message,"[]"));
            }
          }
          
          check_range(logger, d.rbegin_partition(i), d.rend_partition(i), r.rbegin_partition(i), r.rend_partition(i), concat_messages(message, "r_iterator"));          
        }
      }
    }

    template<class Logger, class PartitionedData, class T=typename PartitionedData::value_type>
    void check_equivalence(Logger& logger, const PartitionedData& data, std::initializer_list<std::initializer_list<T>> prediction, std::string_view description="")
    {
      const auto numElements{std::accumulate(prediction.begin(), prediction.end(), std::size_t{},
                                             [](std::size_t val, std::initializer_list<T> partition) { return val += partition.size();})};

      check_equality(logger, data.size(), numElements, "Number of elements");

      if(check_equality(logger, data.num_partitions(), prediction.size(), "Number of partitions"))
      {
        for(std::size_t i{}; i<prediction.size(); ++i)
        {
          const std::string message{concat_messages(description,"Partition " + std::to_string(i))};
          check_range(logger, data.begin_partition(i), data.end_partition(i), (prediction.begin() + i)->begin(), (prediction.begin() + i)->end(), message + ": iterator");

          check_range(logger, data.rbegin_partition(i), data.rend_partition(i), std::rbegin(*(prediction.begin() + i)), std::rend(*(prediction.begin() + i)), message + ": riterator");
        }
      }
    }
  }
  
  template<class T, class SharingPolicy, class Traits>
  struct details_checker<data_structures::bucketed_storage<T, SharingPolicy, Traits>>
  {
    using type = data_structures::bucketed_storage<T, SharingPolicy, Traits>;
    
    template<class Logger>
    static void check(Logger& logger, const type& data, const type& prediction, std::string_view description="")
    {
      impl::check_details(logger, data, prediction, description);
    }
  };

  template<class T, class SharingPolicy, class Traits>
  struct equivalence_checker<data_structures::bucketed_storage<T, SharingPolicy, Traits>>
  {
    using type = data_structures::bucketed_storage<T, SharingPolicy, Traits>;
    
    template<class Logger>
    static void check(Logger& logger, const type& data, std::initializer_list<std::initializer_list<T>> prediction, std::string_view description="")
    {
      impl::check_equivalence(logger, data, prediction, description);
    }
  };

  template<class T, class SharingPolicy, class Traits>
  struct details_checker<data_structures::contiguous_storage<T, SharingPolicy, Traits>>
  {
    using type = data_structures::contiguous_storage<T, SharingPolicy, Traits>;
    
    template<class Logger>
    static void check(Logger& logger, const type& data, const type& prediction, std::string_view description="")
    {
      impl::check_details(logger, data, prediction, description);
    }
  };

  template<class T, class SharingPolicy, class Traits>
  struct equivalence_checker<data_structures::contiguous_storage<T, SharingPolicy, Traits>>
  {
    using type = data_structures::contiguous_storage<T, SharingPolicy, Traits>;
    using equivalent_type = std::initializer_list<std::initializer_list<T>>;
    
    template<class Logger>
    static void check(Logger& logger, const type& data, equivalent_type prediction, std::string_view description="")
    {
      impl::check_equivalence(logger, data, prediction, description);
    }
  };

  template<class T, std::size_t Npartitions, std::size_t Nelements, class IndexType>
  struct details_checker<data_structures::static_contiguous_storage<T, Npartitions, Nelements, IndexType>>
  {
    using type = data_structures::static_contiguous_storage<T, Npartitions, Nelements, IndexType>;
    
    template<class Logger>
    static void check(Logger& logger, const type& data, const type& prediction, std::string_view description="")
    {
      impl::check_details(logger, data, prediction, description);
    }
  };

  template<class T, std::size_t Npartitions, std::size_t Nelements, class IndexType>
  struct equivalence_checker<data_structures::static_contiguous_storage<T, Npartitions, Nelements, IndexType>>
  {
    using type = data_structures::static_contiguous_storage<T, Npartitions, Nelements, IndexType>;
    
    template<class Logger>
    static void check(Logger& logger, const type& data, std::initializer_list<std::initializer_list<T>> prediction, std::string_view description="")
    {
      impl::check_equivalence(logger, data, prediction, description);
    }
  };
}
