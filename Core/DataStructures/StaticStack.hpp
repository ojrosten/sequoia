#pragma once

#include <array>

namespace sequoia::data_structures
{
  template<class T, std::size_t MaxDepth>
  class static_stack
  {
  public:
    constexpr void push(const T& val)
    {
      if(m_End == MaxDepth)
        throw std::logic_error("Attempting to exceed max stack depth");
      
      m_Stack[m_End] = val;
      ++m_End;
    }

    constexpr const T& top() const noexcept
    {
      return m_Stack[m_End - 1];
    }

    constexpr void pop() const noexcept
    {
      --m_End;
    }

    constexpr bool empty() const noexcept
    {
      return m_End == 0u;
    }

    friend constexpr bool operator==(const static_stack& lhs, const static_stack& rhs) noexcept
    {
      return (lhs.m_End == rhs.m_End) && (lhs.m_Stack == rhs.m_Stack);
    }

    friend constexpr bool operator!=(const static_stack& lhs, const static_stack& rhs) noexcept
    {
      return !(lhs == rhs);
    }
  private:
    std::array<T, MaxDepth> m_Stack{};

    std::size_t m_End{};
  };
}
