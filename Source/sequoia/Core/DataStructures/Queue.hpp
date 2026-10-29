////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief A queue usable in a constant evaluation.
 */

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sequoia::data_structures
{
  /** \brief A stand-in for std::queue, usable in a constant evaluation.

      std::queue is usable in a constant evaluation only from C++26 (P3372).
   */
  template<class T>
  class queue
  {
  public:
    using value_type = T;
    using size_type  = std::size_t;

    constexpr queue() = default;

    constexpr void push(const T& value) { m_Incoming.push_back(value); }

    constexpr void push(T&& value) { m_Incoming.push_back(std::move(value)); }

    constexpr void pop()
    {
      if(empty())
        throw std::logic_error{"Attempting to pop an empty queue"};

      if(m_Outgoing.empty())
      {
        // Reversing into a new vector leaves the queue unchanged if copying an
        // element throws
        const auto moveIfNoexcept{[](T& element) -> decltype(auto) { return std::move_if_noexcept(element); }};

        m_Outgoing =   m_Incoming
                     | std::views::reverse
                     | std::views::transform(moveIfNoexcept)
                     | std::ranges::to<std::vector>();
        m_Incoming.clear();
      }

      m_Outgoing.pop_back();
    }

    [[nodiscard]]
    constexpr const T& front() const
    {
      if(empty())
        throw std::logic_error{"Attempting to access the front of an empty queue"};

      return m_Outgoing.empty() ? m_Incoming.front() : m_Outgoing.back();
    }

    [[nodiscard]]
    constexpr const T& back() const
    {
      if(empty())
        throw std::logic_error{"Attempting to access the back of an empty queue"};

      return m_Incoming.empty() ? m_Outgoing.front() : m_Incoming.back();
    }

    [[nodiscard]]
    constexpr bool empty() const { return m_Outgoing.empty() && m_Incoming.empty(); }

    [[nodiscard]]
    constexpr size_type size() const { return m_Outgoing.size() + m_Incoming.size(); }

    [[nodiscard]]
    friend constexpr bool operator==(const queue& lhs, const queue& rhs)
    {
      const auto sameElementAt{
        [&lhs, &rhs](const size_type i) { return lhs.element_from_front(i) == rhs.element_from_front(i); }
      };

      return (lhs.size() == rhs.size()) && std::ranges::all_of(std::views::iota(size_type{}, lhs.size()), sameElementAt);
    }
  private:
    // The queue holds m_Outgoing reversed, followed by m_Incoming. Pushes go
    // to the back of m_Incoming and pops come from the back of m_Outgoing, so
    // both cost amortised O(1) and a pop immediately destroys the element it
    // removes
    std::vector<T> m_Incoming{},
                   m_Outgoing{};

    /// Returns the element `i` places behind the front
    [[nodiscard]]
    constexpr const T& element_from_front(const size_type i) const
    {
      return i < m_Outgoing.size() ? m_Outgoing[m_Outgoing.size() - 1 - i] : m_Incoming[i - m_Outgoing.size()];
    }
  };
}
