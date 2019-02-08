////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file StaticPriorityQueue.hpp
    \brief A constexpr prority queue.

 */

#include "Algorithms.hpp"
#include "ArrayUtilities.hpp"

#include <functional>

namespace sequoia::data_structures
{
  /*! \class static_priority_queue
      \brief A priority_queue suitable for constexpr contexts.

   */
  
  template<class T, std::size_t MaxDepth, class Compare=std::less<T>>
  class static_priority_queue : private Compare
  {
  public:
    constexpr static_priority_queue() = default;

    constexpr explicit static_priority_queue(const Compare& compare) : Compare{compare} {}

    constexpr static_priority_queue(std::initializer_list<T> l)
      : m_Q{make_Q(l, Compare{})}
      , m_End{l.size()}
    {}

    constexpr static_priority_queue(std::initializer_list<T> l, const Compare& compare)
      : Compare{compare}
      , m_Q{make_Q(l, compare)}
      , m_End{l.size()}
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

      bubble_up(m_Q.begin(), m_Q.begin() + m_End - 1, static_cast<Compare&>(*this));
    }

    [[nodiscard]]
    constexpr const T& top() const noexcept
    {
      return *m_Q.begin();
    }

    constexpr void pop() noexcept
    {
      sequoia::iter_swap(m_Q.begin(), m_Q.begin() + m_End -1);
      --m_End;
      sequoia::make_heap(m_Q.begin(), m_Q.begin() + m_End, static_cast<Compare&>(*this));
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
      return (lhs.m_End == rhs.m_End) && sequoia::equal(lhs.m_Q.begin(), lhs.m_Q.begin() + lhs.m_End, rhs.m_Q.begin(), rhs.m_Q.begin() + rhs.m_End);
    }

    [[nodiscard]]
    friend constexpr bool operator!=(const static_priority_queue& lhs, const static_priority_queue& rhs) noexcept
    {
      return !(lhs == rhs);
    }
  private:
    std::array<T, MaxDepth> m_Q{};

    std::size_t m_End{};

    static constexpr std::array<T, MaxDepth> make_Q(std::initializer_list<T> l, const Compare& compare)
    {
      auto q{utilities::to_array<T, MaxDepth>(l)};
      sequoia::make_heap(q.begin(), q.end(), compare);

      return q;
    }
  };
}
