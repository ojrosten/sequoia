////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities relating to vector spaces
 */

#include "sequoia/Core/Meta/TypeTraits.hpp"

#include <span>

namespace sequoia::maths
{
  template<class value_type>
  inline constexpr bool has_cardinality{
    requires {
      { value_type::cardinality } -> std::convertible_to<std::size_t>;
    }
  };

  template<class value_type>
  concept vector_space = has_value_type<value_type> && has_cardinality<value_type>;

  template<vector_space VectorSpace>
  class vec
  {
  public:
    using vector_space_type = VectorSpace;
    using value_type        = typename VectorSpace::value_type;

    constexpr static std::size_t cardinality{VectorSpace::cardinality};
    constexpr static std::size_t D{cardinality};

    constexpr vec() noexcept = default;

    constexpr explicit vec(std::span<const value_type, D> d) noexcept
      : m_Values{to_array(d)}
    {}

    constexpr explicit vec(std::span<value_type, D> d) noexcept
      : m_Values{to_array(d)}
    {}

    template<class... Ts>
      requires (sizeof...(Ts) == D) && (is_initializable_v<value_type, Ts> && ...)
    constexpr vec(Ts... ts) noexcept
      : m_Values{ts...}
    {}

    constexpr vec(const vec&) = default;

    constexpr vec& operator=(const vec&) = default;

    constexpr vec& operator+=(const vec& t) noexcept
    {
      apply_to_each_element(m_Values, t.values(), [](value_type& lhs, value_type rhs){ lhs += rhs; });
      return *this;
    }

    constexpr vec& operator-=(const vec& t) noexcept
    {
      apply_to_each_element(m_Values, t.values(), [](value_type& lhs, value_type rhs){ lhs -= rhs; });
      return *this;
    }

    template<class U>
      requires is_compatible_scalar<value_type, U>
    constexpr vec& operator*=(U u) noexcept
    {
      std::ranges::for_each(m_Values, [u](value_type& x) { return x *= u; });
      return *this;
    }

    template<class U>
      requires is_compatible_scalar<value_type, U>
    constexpr vec& operator/=(U u)
    {
      std::ranges::for_each(m_Values, [u](value_type& x) { return x /= u; });
      return *this;
    }

    [[nodiscard]]
    friend constexpr vec operator+(const vec& lhs, const vec& rhs) noexcept
    {
      return lhs += rhs;
    }

    [[nodiscard]]
    friend constexpr vec operator-(const vec& lhs, const vec& rhs) noexcept
    {
      return lhs -= rhs;
    }

    [[nodiscard]]
    constexpr vec operator-() const noexcept
    {
      return vec{make_from(values(), [](value_type t) { return -t; })};
    }

    template<class U>
      requires is_compatible_scalar<value_type, U>
    [[nodiscard]]
    friend constexpr vec operator*(vec v, U u) noexcept
    {
      return v *= u;
    }

    template<class U>
      requires is_compatible_scalar<value_type, U>
    [[nodiscard]]
    friend constexpr vec operator*(U u, vec v) noexcept
    {
      return v * u;
    }

    template<class U>
      requires is_compatible_scalar<value_type, U>
    [[nodiscard]]
    friend constexpr vec operator/(vec v, U u)
    {
      return v /= u;
    }

    [[nodiscard]]
    constexpr std::span<const value_type, D> values() const noexcept { return m_Values; }

    [[nodiscard]]
    constexpr std::span<value_type, D> values() noexcept { return m_Values; }

    [[nodiscard]]
    constexpr value_type value() const noexcept requires (D == 1) { return m_Values[0]; }

    // Make this explicit since otherwise, given two vectors a,b, a/b is well-formed due to implicit boolean conversion
    [[nodiscard]]
    constexpr explicit operator bool() const noexcept requires (D == 1)
    {
      return m_Values[0];
    }

    [[nodiscard]]
    constexpr value_type operator[](std::size_t i) const { return m_Values[i]; }

    [[nodiscard]]
    constexpr value_type& operator[](std::size_t i) { return m_Values[i]; }

    [[nodiscard]]
    friend constexpr bool operator==(const vec&, const vec&) noexcept requires (D>1) = default;

    [[nodiscard]]
    friend constexpr auto operator<=>(const vec&, const vec&) noexcept requires (D == 1) = default;
  private:
    std::array<value_type, D> m_Values{};
  };
}
