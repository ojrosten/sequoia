////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file Queue.hpp
    \brief A queue which may be used in a constant evaluation, standing in for std::queue.
 */

#include <algorithm>
#include <span>
#include <vector>

namespace sequoia::data_structures
{
  /** \brief A stand-in for std::queue, usable in a constant evaluation.

      std::queue becomes usable in a constant evaluation in C++26 (P3372); neither libc++ 23 nor
      libstdc++ 16 has it yet. Once both do, the graph traversal returns to std::queue and this is
      deleted. It has the part of std::queue's interface which the traversal uses, plus `size` and
      `operator==`, which make it regular, and `back`, which its tests need.

      Popping advances a head rather than erasing, and the elements are cleared when the queue
      empties, so it holds at most the longest run of pushes between one empty state and the next.

      \pre `front`, `back` and `pop` require the queue to be non-empty.
   */
  template<class T>
  class queue
  {
  public:
    using value_type = T;
    using size_type  = std::size_t;

    constexpr queue() = default;

    constexpr void push(const T& value) { m_Elements.push_back(value); }

    constexpr void push(T&& value) { m_Elements.push_back(std::move(value)); }

    constexpr void pop() noexcept
    {
      ++m_Head;
      if(m_Head == m_Elements.size())
      {
        m_Elements.clear();
        m_Head = 0;
      }
    }

    [[nodiscard]]
    constexpr const T& front() const noexcept { return m_Elements[m_Head]; }

    [[nodiscard]]
    constexpr const T& back() const noexcept { return m_Elements.back(); }

    [[nodiscard]]
    constexpr bool empty() const noexcept { return m_Head == m_Elements.size(); }

    [[nodiscard]]
    constexpr size_type size() const noexcept { return m_Elements.size() - m_Head; }

    [[nodiscard]]
    friend constexpr bool operator==(const queue& lhs, const queue& rhs) noexcept
    {
      return std::ranges::equal(lhs.elements(), rhs.elements());
    }
  private:
    std::vector<T> m_Elements{};
    size_type m_Head{};

    [[nodiscard]]
    constexpr std::span<const T> elements() const noexcept { return std::span<const T>{m_Elements}.subspan(m_Head); }
  };
}
