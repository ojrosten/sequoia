////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "ContainerAdaptorTestingUtilities.hpp"

#include "sequoia/Core/DataStructures/PriorityQueue.hpp"

namespace sequoia::testing
{
  /** \brief A priority queue of `int`s with an `operator==`.

      Two compare equal precisely when no sequence of operations can tell them
      apart, since `int`s which `std::ranges::less` finds equivalent are equal.
   */
  struct observed_priority_queue : data_structures::priority_queue<int>
  {
    [[nodiscard]]
    friend constexpr bool operator==(const observed_priority_queue& lhs, const observed_priority_queue& rhs)
    {
      return elements_in_pop_order(lhs) == elements_in_pop_order(rhs);
    }
  };

  template<>
  struct value_tester<observed_priority_queue> : container_adaptor_value_tester<observed_priority_queue>
  {};
}
