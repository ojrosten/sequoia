////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include "sequoia/Core/DataStructures/PartitionedData.hpp"

namespace sequoia::testing
{  
  namespace impl
  {
    template<test_mode Mode, class PartitionedData>
    void check_details(test_logger<Mode>& logger, const PartitionedData& data, const PartitionedData& prediction)
    {
      check_equality("Size different", logger, data.size(), prediction.size());
      if(check_equality("Number of partitions different", logger, data.num_partitions(), prediction.num_partitions()))
      {
        for(std::size_t i{}; i<prediction.num_partitions(); ++i)
        {
          const auto message{std::string{"Partition "}.append(std::to_string(i))};
          if(check_range(append_lines(message, "iterator (const)"), logger, data.begin_partition(i), data.end_partition(i), prediction.begin_partition(i), prediction.end_partition(i)))
          {
            for(int64_t j{}; j<distance(prediction.begin_partition(i), prediction.end_partition(i)); ++j)
            {
              check_equality(append_lines(message,"[] (const)"), logger, data[i][j], prediction[i][j]);
            }
          }
          
          check_range(append_lines(message, "r_iterator (const)"), logger, data.rbegin_partition(i), data.rend_partition(i), prediction.rbegin_partition(i), prediction.rend_partition(i));
          check_range(append_lines(message, "c_iterator"), logger, data.cbegin_partition(i), data.cend_partition(i), prediction.cbegin_partition(i), prediction.cend_partition(i));
          check_range(append_lines(message, "cr_iterator"), logger, data.crbegin_partition(i), data.crend_partition(i), prediction.crbegin_partition(i), prediction.crend_partition(i));

          auto& r{const_cast<PartitionedData&>(prediction)};
          auto& d{const_cast<PartitionedData&>(data)};
          if(check_range(append_lines(message, "iterator"), logger, d.begin_partition(i), d.end_partition(i), r.begin_partition(i), r.end_partition(i)))
          {
            for(int64_t j{}; j<distance(r.begin_partition(i), r.end_partition(i)); ++j)
            {
              check_equality(append_lines(message,"[]"), logger, d[i][j], r[i][j]);
            }
          }
          
          check_range(append_lines(message, "r_iterator"), logger, d.rbegin_partition(i), d.rend_partition(i), r.rbegin_partition(i), r.rend_partition(i));
        }
      }
    }

    template<test_mode Mode, class PartitionedData, class T=typename PartitionedData::value_type>
    void check_equivalence(test_logger<Mode>& logger, const PartitionedData& data, std::initializer_list<std::initializer_list<T>> prediction)
    {
      const auto numElements{std::accumulate(prediction.begin(), prediction.end(), std::size_t{},
        [](std::size_t val, std::initializer_list<T> partition) { return val += partition.size();})};

      check_equality("Number of elements", logger, data.size(), numElements);

      if(check_equality("Number of partitions", logger, data.num_partitions(), prediction.size()))
      {
        for(std::size_t i{}; i<prediction.size(); ++i)
        {
          const auto message{std::string{"Partition "}.append(std::to_string(i))};
          check_range(message + ": iterator", logger, data.begin_partition(i), data.end_partition(i), (prediction.begin() + i)->begin(), (prediction.begin() + i)->end());

          check_range(message + ": riterator", logger, data.rbegin_partition(i), data.rend_partition(i), std::rbegin(*(prediction.begin() + i)), std::rend(*(prediction.begin() + i)));
        }
      }
    }
  }
  
  template<class T, class Handler, class Traits>
  struct detailed_equality_checker<data_structures::bucketed_storage<T, Handler, Traits>>
  {
    using type = data_structures::bucketed_storage<T, Handler, Traits>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& data, const type& prediction)
    {
      impl::check_details(logger, data, prediction);
    }
  };

  template<class T, class Handler, class Traits>
  struct equivalence_checker<data_structures::bucketed_storage<T, Handler, Traits>>
  {
    using type = data_structures::bucketed_storage<T, Handler, Traits>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& data, std::initializer_list<std::initializer_list<T>> prediction)
    {
      impl::check_equivalence(logger, data, prediction);
    }
  };

  template<class T, class Handler, class Traits>
  struct detailed_equality_checker<data_structures::partitioned_sequence<T, Handler, Traits>>
  {
    using type = data_structures::partitioned_sequence<T, Handler, Traits>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& data, const type& prediction)
    {
      impl::check_details(logger, data, prediction);
    }
  };

  template<class T, class Handler, class Traits>
  struct equivalence_checker<data_structures::partitioned_sequence<T, Handler, Traits>>
  {
    using type = data_structures::partitioned_sequence<T, Handler, Traits>;
    using equivalent_type = std::initializer_list<std::initializer_list<T>>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& data, equivalent_type prediction)
    {
      impl::check_equivalence(logger, data, prediction);
    }
  };

  template<class T, std::size_t Npartitions, std::size_t Nelements, class IndexType>
  struct detailed_equality_checker<data_structures::static_partitioned_sequence<T, Npartitions, Nelements, IndexType>>
  {
    using type = data_structures::static_partitioned_sequence<T, Npartitions, Nelements, IndexType>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& data, const type& prediction)
    {
      impl::check_details(logger, data, prediction);
    }
  };

  template<class T, std::size_t Npartitions, std::size_t Nelements, class IndexType>
  struct equivalence_checker<data_structures::static_partitioned_sequence<T, Npartitions, Nelements, IndexType>>
  {
    using type = data_structures::static_partitioned_sequence<T, Npartitions, Nelements, IndexType>;
    
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const type& data, std::initializer_list<std::initializer_list<T>> prediction)
    {
      impl::check_equivalence(logger, data, prediction);
    }
  };
}
