////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Core/DataStructures/StaticLinearlyPartitionedSequence.hpp"
#include "PartitionedDataTestingUtilities.hpp"

namespace sequoia::testing
{
  template<class T, std::size_t Npartitions, std::size_t NelementsPerPartition, std::integral IndexType>
  struct value_tester<sequoia::data_structures::static_linearly_partitioned_sequence<T, Npartitions, NelementsPerPartition, IndexType>>
  {
    using type = sequoia::data_structures::static_linearly_partitioned_sequence<T, Npartitions, NelementsPerPartition, IndexType>;

    template<test_mode Mode>
    static void test_equality(test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      impl::check_details(logger, actual, prediction);
    }

    template<test_mode Mode>
    static void test_equivalence(test_logger<Mode>& logger, const type& actual, const std::array<std::array<T, NelementsPerPartition>, Npartitions>& prediction)
    {
      for (std::size_t i{}; i < prediction.size(); ++i)
      {
        const auto message{ std::string{"Partition "}.append(std::to_string(i)) };
        check_range(message + ": iterator", logger, actual.begin_partition(i), actual.end_partition(i), (prediction.begin() + i)->begin(), (prediction.begin() + i)->end());

        check_range(message + ": riterator", logger, actual.rbegin_partition(i), actual.rend_partition(i), std::rbegin(*(prediction.begin() + i)), std::rend(*(prediction.begin() + i)));
      }
    }
  };
}
