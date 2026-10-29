////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief A stack usable in a constant evaluation.
 */

#include "sequoia/Core/ContainerUtilities/ContainerAdaptorUtilities.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sequoia::data_structures
{
  /** \brief A stand-in for std::stack, usable in a constant evaluation.

      std::stack is usable in a constant evaluation only from C++26 (P3372).
   */
  template<class T>
    requires utilities::gives_references_to_elements_v<std::vector<T>>
  class stack
  {
  public:
    using value_type = T;
    using size_type  = std::size_t;

    constexpr stack() = default;

    constexpr void push(const T& value) { m_Elements.push_back(value); }

    constexpr void push(T&& value) { m_Elements.push_back(std::move(value)); }

    constexpr void pop()
    {
      if(empty())
        throw std::logic_error{"Attempting to pop an empty stack"};

      m_Elements.pop_back();
    }

    [[nodiscard]]
    constexpr const T& top() const
    {
      if(empty())
        throw std::logic_error{"Attempting to access the top of an empty stack"};

      return m_Elements.back();
    }

    [[nodiscard]]
    constexpr bool empty() const { return m_Elements.empty(); }

    [[nodiscard]]
    constexpr size_type size() const { return m_Elements.size(); }

    [[nodiscard]]
    friend constexpr bool operator==(const stack&, const stack&) = default;
  private:
    std::vector<T> m_Elements{};
  };
}
