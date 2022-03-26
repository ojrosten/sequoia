////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file StaticStack.hpp
    \brief A constexpr stack.

 */

#include "sequoia/Core/ContainerUtilities/ArrayUtilities.hpp"
#include "sequoia/Algorithms/Algorithms.hpp"

namespace sequoia::data_structures
{
  /*! \class static_stack
      \brief A stack suitable for constexpr contexts.

   */

  template<class T, std::size_t MaxDepth>
  class static_stack
  {
  public:
    constexpr static_stack(std::initializer_list<T> l)
    : m_Stack{utilities::to_array<T, MaxDepth>(l)}
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

    [[nodiscard]]
    constexpr const T& top() const noexcept
    {
      return m_Stack[m_End - 1];
    }

    constexpr void pop() noexcept
    {
      --m_End;
    }

    [[nodiscard]]
    constexpr bool empty() const noexcept
    {
      return m_End == 0u;
    }

    [[nodiscard]]
    constexpr std::size_t size() const noexcept
    {
      return m_End;
    }

    [[nodiscard]]
    friend constexpr bool operator==(const static_stack& lhs, const static_stack& rhs) noexcept
    {
      return (lhs.m_End == rhs.m_End)
          && std::equal(lhs.m_Stack.begin(), lhs.m_Stack.begin() + lhs.m_End, rhs.m_Stack.begin(), rhs.m_Stack.begin() + rhs.m_End);
    }

    [[nodiscard]]
    friend constexpr bool operator!=(const static_stack& lhs, const static_stack& rhs) noexcept
    {
      return !(lhs == rhs);
    }
  private:
    std::array<T, MaxDepth> m_Stack{};

    std::size_t m_End{};
  };
}
