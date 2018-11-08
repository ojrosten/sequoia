#pragma once

#include <array>

namespace sequoia::data_structures
{
  template<class T, std::size_t MaxPushes>
  class static_queue
  {
  public:
    constexpr void push(const T& val)
    {
      if(m_End == MaxPushes)
        throw std::logic_error("Attempting to exceed max queue depth");
      
      m_Queue[m_End] = val;
      ++m_End;
    }

    constexpr const T& back() const noexcept
    {
      return m_Queue[m_End - 1];
    }

    constexpr const T& front() const noexcept
    {
      return m_Queue[m_Begin];
    }

    constexpr void pop() noexcept
    {
      ++m_Begin;
    }

    constexpr bool empty() const noexcept
    {
      return m_Begin == m_End;
    }

    constexpr std::size_t size() const noexcept
    {
      return m_End - m_Begin;
    }

    friend constexpr bool operator==(const static_queue& lhs, const static_queue& rhs) noexcept
    {
      return (lhs.m_Begin == rhs.m_Begin) && (lhs.m_End == rhs.m_End) && (lhs.m_Queue == rhs.m_Queue);
    }

    friend constexpr bool operator!=(const static_queue& lhs, const static_queue& rhs) noexcept
    {
      return !(lhs == rhs);
    }
  private:
    std::array<T, MaxPushes> m_Queue{};

    std::size_t m_Begin{}, m_End{};
  };
}
