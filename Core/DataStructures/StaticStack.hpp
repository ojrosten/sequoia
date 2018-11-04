#pragma once

namespace sequoia::data_structures
{
  template<class T, std::size_t MaxDepth>
  class static_stack
  {
    constexpr void push(const T& val)
    {
      if(m_End == MaxDepth)
        throw std::logic_error("Attempting to exceed max stack depth");
      
      m_Stack[m_End] = val;
      ++m_End;
    }

    constexpr const T& top() const noexcept
    {
      return m_Stack[m_End - 1].value();
    }

    void pop() const noexcept
    {
      --m_End;
    }

    constexpr bool empty() const noexcept
    {
      return m_End == 0u;
    }
  private:
    std::array<T, MaxDepth> m_Stack{};

    std::size_t m_End{};
  };
}
