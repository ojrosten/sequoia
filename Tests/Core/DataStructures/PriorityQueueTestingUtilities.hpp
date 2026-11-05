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

#include <functional>
#include <utility>
#include <vector>

namespace sequoia::testing
{
  /** \brief A priority queue with an `operator==`.

      Two compare equal precisely when no sequence of operations can tell them
      apart, provided that elements which `Compare` finds equivalent are
      equal.
   */
  template<class T, class Compare = std::ranges::less>
  class observed_priority_queue
  {
  public:
    using priority_queue_type = data_structures::priority_queue<T, std::vector<T>, Compare>;
    using value_type          = T;
    using size_type           = priority_queue_type::size_type;

    constexpr void push(const T& value) { m_Queue.push(value); }

    constexpr void push(T&& value) { m_Queue.push(std::move(value)); }

    constexpr void pop() { m_Queue.pop(); }

    [[nodiscard]]
    constexpr const T& top() const { return m_Queue.top(); }

    [[nodiscard]]
    constexpr bool empty() const noexcept { return m_Queue.empty(); }

    [[nodiscard]]
    constexpr size_type size() const noexcept { return m_Queue.size(); }

    [[nodiscard]]
    friend constexpr bool operator==(const observed_priority_queue& lhs, const observed_priority_queue& rhs)
    {
      return elements_in_pop_order(lhs) == elements_in_pop_order(rhs);
    }
  private:
    priority_queue_type m_Queue{};
  };

  template<class T, class Compare>
  struct value_tester<observed_priority_queue<T, Compare>>
    : container_adaptor_value_tester<observed_priority_queue<T, Compare>>
  {};
}
