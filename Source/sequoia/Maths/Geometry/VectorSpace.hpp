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
  template<class T>
  inline constexpr bool has_cardinality{
    requires {
      { T::cardinality } -> std::convertible_to<std::size_t>;
    }
  };

  template<class T>
  concept field = std::regular<T> &&
    requires(T& t) {
      { t += t } -> std::convertible_to<T>;
      { t -= t } -> std::convertible_to<T>;
      { t *= t } -> std::convertible_to<T>;
      { t /= t } -> std::convertible_to<T>;

      { t + t } -> std::convertible_to<T>;
      { t - t } -> std::convertible_to<T>;
      { t * t } -> std::convertible_to<T>;
      { t / t } -> std::convertible_to<T>;
    };

  template<class T>
  concept vector_space = has_value_type<T> && has_cardinality<T> && field<typename T::value_type>;

  template<class T, std::size_t D, class Fn>
      requires std::invocable<Fn, T&, T>
  constexpr void apply_to_each_element(std::array<T, D>& lhs, std::span<const T, D> rhs, Fn f)
  {
      [&] <std::size_t... Is> (std::index_sequence<Is...>){
          (f(lhs[Is], rhs[Is]), ...);
      }(std::make_index_sequence<D>{});
  }

  template<class T, std::size_t D, class Fn>
      requires std::is_invocable_r_v<T, Fn, T>
  [[nodiscard]]
  constexpr std::array<T, D> make_from(std::span<T, D> a, Fn f)
  {
      return[&] <std::size_t... Is> (std::index_sequence<Is...>){
          return std::array<T, D>{f(a[Is])...};
      }(std::make_index_sequence<D>{});
  }

  template<class T, std::size_t D>
  [[nodiscard]]
  constexpr std::array<std::remove_cvref_t<T>, D> to_array(std::span<T, D> s)
  {
      return[&] <std::size_t... Is> (std::index_sequence<Is...>){
          return std::array<std::remove_cvref_t<T>, D>{s[Is]...};
      }(std::make_index_sequence<D>{});
  }

  template<vector_space VectorSpace>
  class vec
  {
  public:
    using vector_space_type = VectorSpace;
    using value_type        = typename VectorSpace::value_type;

    constexpr static std::size_t cardinality{VectorSpace::cardinality};
    constexpr static std::size_t D{cardinality};

    constexpr vec() = default;

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

    constexpr vec& operator*=(value_type u) noexcept
    {
      std::ranges::for_each(m_Values, [u](value_type& x) { return x *= u; });
      return *this;
    }

    constexpr vec& operator/=(value_type u)
    {
      std::ranges::for_each(m_Values, [u](value_type& x) { return x /= u; });
      return *this;
    }

    [[nodiscard]]
    friend constexpr vec operator+(const vec& lhs, const vec& rhs) noexcept
    {
      return vec{lhs} += rhs;
    }

    [[nodiscard]]
    friend constexpr vec operator-(const vec& lhs, const vec& rhs) noexcept
    {
      return vec{lhs} -= rhs;
    }

    [[nodiscard]]
    constexpr vec operator+() const noexcept
    {
      return vec{values()};
    }

    [[nodiscard]]
    constexpr vec operator-() const noexcept
    {
      return vec{make_from(values(), [](value_type t) { return -t; })};
    }

    [[nodiscard]]
    friend constexpr vec operator*(vec v, value_type u) noexcept
    {
      return v *= u;
    }

    [[nodiscard]]
    friend constexpr vec operator*(value_type u, vec v) noexcept
    {
      return v * u;
    }

    [[nodiscard]]
    friend constexpr vec operator/(vec v, value_type u)
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
    friend constexpr bool operator==(const vec&, const vec&) noexcept = default;

    [[nodiscard]]
    friend constexpr auto operator<=>(const vec& lhs, const vec& rhs) noexcept requires (D == 1) && std::totally_ordered<value_type>
    {
      return lhs.value() <=> rhs.value();
    }
  private:
    std::array<value_type, D> m_Values{};
  };
}
