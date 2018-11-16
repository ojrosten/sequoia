#pragma once

#include "ArrayUtilities.hpp"

namespace sequoia::data_structures
{
  template<class T, std::size_t MaxDepth>
  class static_stack
  {
  public:
    constexpr static_stack(std::initializer_list<T> l)
      : m_Stack{utilities::to_array<MaxDepth>(l)}
      , m_End{l.size()}
    {
    }

    constexpr static_stack(const static_stack&)    = default;
    constexpr static_stack(static_stack&) noexcept = default;
    ~static_stack() = default;

    constexpr static_stack& operator=(const static_stack&)    = default;
    constexpr static_stack& operator=(static_stack&) noexcept = default;
    
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

    constexpr void pop() noexcept
    {
      --m_End;
    }

    constexpr bool empty() const noexcept
    {
      return m_End == 0u;
    }

    constexpr std::size_t size() const noexcept
    {
      return m_End;
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
