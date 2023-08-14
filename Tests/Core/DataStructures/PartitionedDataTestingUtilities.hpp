////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include "sequoia/Core/DataStructures/PartitionedData.hpp"

namespace sequoia::testing
{
  namespace impl
  {
    template<class CheckType, test_mode Mode, class PartitionedData>
    void check_details(CheckType flavour, test_logger<Mode>& logger, const PartitionedData& data, const PartitionedData& prediction)
    {
      check(equality, "Emptiness incorrect", logger, data.empty(), prediction.empty());

      check(equality, "Size incorrect", logger, data.size(), prediction.size());

      if(check(equality, "Number of partitions incorrect", logger, data.num_partitions(), prediction.num_partitions()))
      {
        for(std::size_t i{}; i<prediction.num_partitions(); ++i)
        {
          const auto message{std::string{"Partition "}.append(std::to_string(i))};
          check(equality, append_lines(message, "size_of_partition"), logger, data.size_of_partition(i), prediction.size_of_partition(i));

          if(check(flavour, append_lines(message, "iterator (const)"), logger, data.begin_partition(i), data.end_partition(i), prediction.begin_partition(i), prediction.end_partition(i)))
          {
            for(int64_t j{}; j<std::ranges::distance(prediction.partition(i)); ++j)
            {
              check(flavour, append_lines(message,"[] (const)"), logger, data[i][j], prediction[i][j]);
            }
          }

          check(flavour, append_lines(message, "r_iterator (const)"), logger, data.rbegin_partition(i), data.rend_partition(i), prediction.rbegin_partition(i), prediction.rend_partition(i));
          check(flavour, append_lines(message, "c_iterator"), logger, data.cbegin_partition(i), data.cend_partition(i), prediction.cbegin_partition(i), prediction.cend_partition(i));
          check(flavour, append_lines(message, "cr_iterator"), logger, data.crbegin_partition(i), data.crend_partition(i), prediction.crbegin_partition(i), prediction.crend_partition(i));
          // TO DO: just pass the subranges as arguments; for now call begin() and end() to
          // defer a difficult bit of cross-platform consistent demangling
          check(flavour, append_lines(message, "subrange (const)"), logger, data.partition(i).begin(), data.partition(i).end(), prediction.partition(i).begin(), prediction.partition(i).end());

          auto& r{const_cast<PartitionedData&>(prediction)};
          auto& d{const_cast<PartitionedData&>(data)};
          if(check(flavour, append_lines(message, "iterator"), logger, d.begin_partition(i), d.end_partition(i), r.begin_partition(i), r.end_partition(i)))
          {
            for(int64_t j{}; j<std::ranges::distance(r.partition(i)); ++j)
            {
              check(flavour, append_lines(message,"[]"), logger, d[i][j], r[i][j]);
            }
          }

          check(flavour, append_lines(message, "r_iterator"), logger, d.rbegin_partition(i), d.rend_partition(i), r.rbegin_partition(i), r.rend_partition(i));
          // TO DO: just pass the subranges as arguments; for now call begin() and end() to
          // defer a difficult bit of cross-platform consistent demangling
          check(flavour, append_lines(message, "subrange"), logger, d.partition(i).begin(), d.partition(i).end(), r.partition(i).begin(), r.partition(i).end());
        }
      }
    }

    template<test_mode Mode, class PartitionedData, class T=typename PartitionedData::value_type>
    void check(equivalence_check_t, test_logger<Mode>& logger, const PartitionedData& data, std::initializer_list<std::initializer_list<T>> prediction)
    {
      const auto numElements{std::accumulate(prediction.begin(), prediction.end(), std::size_t{},
        [](std::size_t val, std::initializer_list<T> partition) { return val += partition.size();})};

      check(equality, "Number of elements incorrect", logger, data.size(), numElements);

      if(check(equality, "Number of partitions incorrect", logger, data.num_partitions(), prediction.size()))
      {
        for(std::size_t i{}; i<prediction.size(); ++i)
        {
          const auto message{std::string{"Partition "}.append(std::to_string(i))};
          check(equality, append_lines(message, "size_of_partition"), logger, data.size_of_partition(i), (prediction.begin() + i)->size());

          check(with_best_available, message + ": iterator", logger, data.begin_partition(i), data.end_partition(i), (prediction.begin() + i)->begin(), (prediction.begin() + i)->end());
          check(with_best_available, message + ": riterator", logger, data.rbegin_partition(i), data.rend_partition(i), std::rbegin(*(prediction.begin() + i)), std::rend(*(prediction.begin() + i)));
        }
      }
    }
  }

  template<class T, class Handler, class Traits>
  struct value_tester<data_structures::bucketed_sequence<T, Handler, Traits>>
  {
    using type = data_structures::bucketed_sequence<T, Handler, Traits>;

    template<class CheckType, test_mode Mode>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& data, const type& prediction)
    {
      impl::check_details(flavour, logger, data, prediction);
    }

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& data, std::initializer_list<std::initializer_list<T>> prediction)
    {
      impl::check(equivalence, logger, data, prediction);
    }
  };

  template<class T, class Handler, class Traits>
  struct value_tester<data_structures::partitioned_sequence<T, Handler, Traits>>
  {
    using type = data_structures::partitioned_sequence<T, Handler, Traits>;
    using equivalent_type = std::initializer_list<std::initializer_list<T>>;

    template<class CheckType, test_mode Mode>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& data, const type& prediction)
    {
      impl::check_details(flavour, logger, data, prediction);
    }

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& data, equivalent_type prediction)
    {
      impl::check(equivalence, logger, data, prediction);
    }
  };

  template<class T, std::size_t Npartitions, std::size_t Nelements, std::integral IndexType>
  struct value_tester<data_structures::static_partitioned_sequence<T, Npartitions, Nelements, IndexType>>
  {
    using type = data_structures::static_partitioned_sequence<T, Npartitions, Nelements, IndexType>;

    template<class CheckType, test_mode Mode>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& data, const type& prediction)
    {
      impl::check_details(flavour, logger, data, prediction);
    }

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& data, std::initializer_list<std::initializer_list<T>> prediction)
    {
      impl::check(equivalence, logger, data, prediction);
    }
  };

  template<std::input_or_output_iterator I, class Handler, template<class> class RefType, class IndexPolicy>
    requires object::handler<Handler>
  struct value_tester<utilities::iterator<I, data_structures::partition_impl::dereference_policy_for<Handler, RefType, IndexPolicy>>>
  {
    using type = utilities::iterator<I, data_structures::partition_impl::dereference_policy_for<Handler, RefType, IndexPolicy>>;

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& data, const type& prediction)
    {
      const auto dist{std::ranges::distance(data, prediction)};
      using dist_t = decltype(dist);
      check(equality, "Distance from prediction", logger, dist, dist_t{});
      check(equality, "Partition Index", logger, data.partition_index(), prediction.partition_index());
    }
  };
}
