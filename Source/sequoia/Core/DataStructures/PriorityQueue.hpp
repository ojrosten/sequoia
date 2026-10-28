////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief A priority queue usable in a constant evaluation.
 */

#include "sequoia/PlatformSpecific/Preprocessor.hpp"

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sequoia::data_structures
{
  /** \brief A stand-in for std::priority_queue, usable in a constant
             evaluation.

      std::priority_queue is usable in a constant evaluation only from C++26
      (P3372).

      Queues holding the same elements may behave differently, so there is no
      `operator==`: elements which `Compare` finds equivalent may be
      distinguishable, and the pushes and pops which led to a queue's state
      decide which of them `top` returns.
   */
  template<class T, class Container = std::vector<T>, class Compare = std::ranges::less>
    requires std::same_as<T, typename Container::value_type>
          && std::ranges::random_access_range<Container>
          && std::sortable<std::ranges::iterator_t<Container>, Compare>
  class priority_queue
  {
  public:
    using value_type = T;
    using size_type  = Container::size_type;

    constexpr priority_queue() = default;

    constexpr explicit priority_queue(const Compare& compare)
      : m_Compare{compare}
    {}

    constexpr void push(const value_type& value)
    {
      m_Container.push_back(value);
      std::ranges::push_heap(m_Container, m_Compare);
    }

    constexpr void push(value_type&& value)
    {
      m_Container.push_back(std::move(value));
      std::ranges::push_heap(m_Container, m_Compare);
    }

    constexpr void pop()
    {
      if(empty())
        throw std::logic_error{"Attempting to pop an empty priority_queue"};

      std::ranges::pop_heap(m_Container, m_Compare);
      m_Container.pop_back();
    }

    [[nodiscard]]
    constexpr const value_type& top() const
    {
      if(empty())
        throw std::logic_error{"Attempting to access the top of an empty priority_queue"};

      return m_Container.front();
    }

    [[nodiscard]]
    constexpr bool empty() const { return m_Container.empty(); }

    [[nodiscard]]
    constexpr size_type size() const { return m_Container.size(); }
  private:
    Container m_Container{};

    SEQUOIA_NO_UNIQUE_ADDRESS Compare m_Compare{};
  };
}
