////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Classes implementing the concept of a linear sequence.
 */

namespace sequoia::maths
{
  template<class T, std::integral Index>
  class linear_sequence
  {
  public:
    using value_type = T;
    using size_type = Index;

    constexpr linear_sequence(T start, T step)
      : m_Start{std::move(start)}
      , m_Step{std::move(step)}
    {}

    [[nodiscard]]
    constexpr T operator[](const size_type i) const { return m_Start + i * m_Step; }

    [[nodiscard]]
    constexpr T start() const noexcept { return m_Start; }

    [[nodiscard]]
    constexpr T step() const noexcept { return m_Step; }

    [[nodiscard]]
    friend constexpr bool operator==(const linear_sequence&, const linear_sequence&) noexcept = default;

    [[nodiscard]]
    friend constexpr bool operator!=(const linear_sequence&, const linear_sequence&) noexcept = default;
  private:
    T m_Start, m_Step;
  };


  template<class T, T Start, T Step, std::size_t Size, std::integral Index>
  struct static_linear_sequence
  {
    using value_type = T;
    using size_type = Index;

    [[nodiscard]]
    constexpr T start() const noexcept { return Start; }

    [[nodiscard]]
    constexpr T step() const noexcept { return Step; }

    [[nodiscard]]
    constexpr std::size_t size() const noexcept { return Size; }

    [[nodiscard]]
    constexpr T operator[](const size_type i) const { return Start + i * Step; }

    [[nodiscard]]
    friend constexpr bool operator==(const static_linear_sequence&, const static_linear_sequence&) noexcept = default;
  };
}
