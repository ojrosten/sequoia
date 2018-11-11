#pragma once

#include "Algorithms.hpp"

#include <array>
#include <functional>

namespace sequoia::data_structures
{
  template<class T, std::size_t MaxDepth, class Compare=std::less<T>>
  class static_priority_queue : private Compare
  {
  public:
    constexpr static_priority_queue() = default;

    constexpr explicit static_priority_queue(const Compare& compare) : Compare{compare} {}

    constexpr static_priority_queue(const static_priority_queue&)     = default;
    constexpr static_priority_queue(static_priority_queue&&) noexcept = default;

    ~static_priority_queue() = default;

    constexpr static_priority_queue& operator=(const static_priority_queue&)     = default;
    constexpr static_priority_queue& operator=(static_priority_queue&&) noexcept = default;
    
    constexpr void push(const T& val)
    {
      if(m_End == MaxDepth)
        throw std::logic_error("Attempting to exceed max priority_queue depth");
      
      m_Q[m_End] = val;
      ++m_End;

      bubble_up(m_Q.begin() + m_Begin, m_Q.begin() + m_End - 1, static_cast<Compare&>(*this));
    }

    constexpr const T& top() const noexcept
    {
      return m_Q[m_Begin];
    }

    constexpr void pop() noexcept
    {
      ++m_Begin;
      sequoia::make_heap(m_Q.begin() + m_Begin, m_Q.begin() + m_End, static_cast<Compare&>(*this));
    }

    constexpr bool empty() const noexcept
    {
      return m_Begin == m_End;
    }

    constexpr std::size_t size() const noexcept
    {
      return m_End - m_Begin;
    }

    friend constexpr bool operator==(const static_priority_queue& lhs, const static_priority_queue& rhs) noexcept
    {
      return (lhs.m_Begin == rhs.m_Begin) && (lhs.m_End == rhs.m_End) && (lhs.m_Q == rhs.m_Q);
    }

    friend constexpr bool operator!=(const static_priority_queue& lhs, const static_priority_queue& rhs) noexcept
    {
      return !(lhs == rhs);
    }
  private:
    std::array<T, MaxDepth> m_Q{};

    std::size_t m_Begin{}, m_End{};
  };
}
