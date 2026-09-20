////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file Stack.hpp
    \brief A stack which may be used in a constant evaluation, standing in for std::stack.
 */

#include <vector>

namespace sequoia::data_structures
{
  /** \brief A stand-in for std::stack, usable in a constant evaluation.

      std::stack becomes usable in a constant evaluation in C++26 (P3372); neither libc++ 23 nor
      libstdc++ 16 has it yet. Once both do, the graph traversal returns to std::stack and this is
      deleted. It has the part of std::stack's interface which the traversal uses, plus `size` and
      `operator==`, which make it regular.

      \pre `top` and `pop` require the stack to be non-empty.
   */
  template<class T>
  class stack
  {
  public:
    using value_type = T;
    using size_type  = std::size_t;

    constexpr stack() = default;

    constexpr void push(const T& value) { m_Elements.push_back(value); }

    constexpr void push(T&& value) { m_Elements.push_back(std::move(value)); }

    constexpr void pop() noexcept { m_Elements.pop_back(); }

    [[nodiscard]]
    constexpr const T& top() const noexcept { return m_Elements.back(); }

    [[nodiscard]]
    constexpr bool empty() const noexcept { return m_Elements.empty(); }

    [[nodiscard]]
    constexpr size_type size() const noexcept { return m_Elements.size(); }

    [[nodiscard]]
    friend constexpr bool operator==(const stack&, const stack&) noexcept = default;
  private:
    std::vector<T> m_Elements{};
  };
}
