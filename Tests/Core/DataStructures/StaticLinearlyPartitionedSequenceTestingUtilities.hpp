////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "PartitionedDataTestingUtilities.hpp"

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <compare>
#include <concepts>
#include <execution>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <optional>
#include <scoped_allocator>
#include <source_location>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

import sequoia.core.data_structures;
import sequoia.test_framework;

namespace sequoia::testing
{
  template<class T, std::size_t Npartitions, std::size_t NelementsPerPartition, std::integral IndexType>
  struct value_tester<sequoia::data_structures::static_linearly_partitioned_sequence<T, Npartitions, NelementsPerPartition, IndexType>>
  {
    using type = sequoia::data_structures::static_linearly_partitioned_sequence<T, Npartitions, NelementsPerPartition, IndexType>;

    template<class CheckType, test_mode Mode>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      impl::check_details(flavour, logger, actual, prediction);
    }

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const std::array<std::array<T, NelementsPerPartition>, Npartitions>& prediction)
    {
      for (std::size_t i{}; i < prediction.size(); ++i)
      {
        const auto message{ std::string{"Partition "}.append(std::to_string(i)) };
        check(with_best_available, message + ": iterator", logger, actual.begin_partition(i), actual.end_partition(i), (prediction.begin() + i)->begin(), (prediction.begin() + i)->end());

        check(with_best_available, message + ": riterator", logger, actual.rbegin_partition(i), actual.rend_partition(i), std::rbegin(*(prediction.begin() + i)), std::rend(*(prediction.begin() + i)));
      }
    }
  };
}
