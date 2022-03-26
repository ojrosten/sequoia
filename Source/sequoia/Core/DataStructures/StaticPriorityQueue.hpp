////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file StaticPriorityQueue.hpp
    \brief A constexpr prority queue.

 */

#include "sequoia/Algorithms/Algorithms.hpp"
#include "sequoia/Core/ContainerUtilities/ArrayUtilities.hpp"

#include <functional>

namespace sequoia::data_structures
{
  /*! \class static_priority_queue
      \brief A priority_queue suitable for constexpr contexts.

   */

  template<class T, std::size_t MaxDepth, class Compare=std::less<T>>
  class static_priority_queue
  {
  public:
    constexpr static_priority_queue() = default;

    constexpr explicit static_priority_queue(const Compare& compare) : m_Compare{compare} {}

    constexpr static_priority_queue(std::initializer_list<T> l)
      : m_Q{make_Q(l, Compare{})}
      , m_End{l.size()}
    {}

    constexpr static_priority_queue(std::initializer_list<T> l, const Compare& compare)
      : m_Q{make_Q(l, compare)}
      , m_End{l.size()}
      , m_Compare{compare}
    {}

    constexpr static_priority_queue(const static_priority_queue&)     = default;
    constexpr static_priority_queue(static_priority_queue&&) noexcept = default;

    ~static_priority_queue() = default;

    constexpr static_priority_queue& operator=(const static_priority_queue&)     = default;
    constexpr static_priority_queue& operator=(static_priority_queue&&) noexcept = default;

    constexpr void push(const T& val)
    {
      if(m_End == MaxDepth)
        throw std::logic_error("Attempting to exceed max priority_queue depth");

      m_Q[m_End++] = val;

      bubble_up(m_Q.begin(), m_Q.begin() + m_End - 1, m_Compare);
    }

    [[nodiscard]]
    constexpr const T& top() const noexcept
    {
      return *m_Q.begin();
    }

    constexpr void pop() noexcept
    {
      std::iter_swap(m_Q.begin(), m_Q.begin() + m_End -1);
      --m_End;
      sequoia::make_heap(m_Q.begin(), m_Q.begin() + m_End, m_Compare);
    }

    [[nodiscard]]
    constexpr bool empty() const noexcept
    {
      return m_End == 0;
    }

    [[nodiscard]]
    constexpr std::size_t size() const noexcept
    {
      return m_End;
    }

    [[nodiscard]]
    friend constexpr bool operator==(const static_priority_queue& lhs, const static_priority_queue& rhs) noexcept
    {
      return (lhs.m_End == rhs.m_End) && std::equal(lhs.m_Q.begin(), lhs.m_Q.begin() + lhs.m_End, rhs.m_Q.begin(), rhs.m_Q.begin() + rhs.m_End);
    }

    [[nodiscard]]
    friend constexpr bool operator!=(const static_priority_queue& lhs, const static_priority_queue& rhs) noexcept
    {
      return !(lhs == rhs);
    }
  private:
    std::array<T, MaxDepth> m_Q{};

    std::size_t m_End{};

    NO_UNIQUE_ADDRESS Compare m_Compare;

    constexpr static std::array<T, MaxDepth> make_Q(std::initializer_list<T> l, const Compare& compare)
    {
      auto q{utilities::to_array<T, MaxDepth>(l)};
      sequoia::make_heap(q.begin(), q.end(), compare);

      return q;
    }
  };
}
