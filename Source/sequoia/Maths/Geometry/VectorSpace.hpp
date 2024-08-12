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
  inline constexpr bool has_dimension{
    requires { { T::dimension } -> std::convertible_to<std::size_t>; }
  };

  template<class T>
  inline constexpr bool has_field_type{
    requires { 
      typename T::field_type;
      requires field<typename T::field_type>;
    }
  };

  template<class T>
  inline constexpr bool has_element_type{
    requires { typename T::element_type; }
  };

  template<class T>
  concept vector_space = has_element_type<T> && has_field_type<T> && has_dimension<T>;

  template<class B, class VectorSpace>
  concept basis = vector_space<VectorSpace>; // TO DO

  template<class T, std::size_t D, class Fn>
      requires std::invocable<Fn, T&, T>
  constexpr void apply_to_each_element(std::array<T, D>& lhs, std::span<const T, D> rhs, Fn f)
  {
    std::ranges::for_each(std::views::zip(lhs, rhs), [&f](auto&& z){ f(std::get<0>(z), std::get<1>(z)); });
  }

  template<class T, std::size_t D, class Fn=std::identity>
      requires std::is_invocable_r_v<T, Fn, T>
  [[nodiscard]]
  constexpr std::array<T, D> to_array(std::span<const T, D> a, Fn f = {})
  {
      return[&] <std::size_t... Is> (std::index_sequence<Is...>){
          return std::array<T, D>{f(a[Is])...};
      }(std::make_index_sequence<D>{});
  }

  template<vector_space VectorSpace, basis<VectorSpace> Basis>
  class vector_representation
  {
  public:
    using vector_space_type = VectorSpace;
    using element_type      = typename VectorSpace::element_type;
    using field_type        = typename VectorSpace::field_type;
    using value_type        = field_type;
    using basis_type        = Basis;

    constexpr static std::size_t dimension{VectorSpace::dimension};
    constexpr static std::size_t D{dimension};

    constexpr vector_representation() = default;

    constexpr explicit vector_representation(std::span<const value_type, D> d) noexcept
      : m_Values{to_array(d)}
    {}

    constexpr explicit vector_representation(std::span<value_type, D> d) noexcept
      : m_Values{to_array(d)}
    {}

    template<class... Ts>
      requires (sizeof...(Ts) == D) && (is_initializable_v<value_type, Ts> && ...)
    constexpr vector_representation(Ts... ts) noexcept
      : m_Values{ts...}
    {}

    constexpr vector_representation& operator+=(const vector_representation& t) noexcept
    {
      apply_to_each_element(m_Values, t.values(), [](value_type& lhs, value_type rhs){ lhs += rhs; });
      return *this;
    }

    constexpr vector_representation& operator-=(const vector_representation& t) noexcept
    {
      apply_to_each_element(m_Values, t.values(), [](value_type& lhs, value_type rhs){ lhs -= rhs; });
      return *this;
    }

    constexpr vector_representation& operator*=(value_type u) noexcept
    {
      std::ranges::for_each(m_Values, [u](value_type& x) { return x *= u; });
      return *this;
    }

    constexpr vector_representation& operator/=(value_type u)
    {
      std::ranges::for_each(m_Values, [u](value_type& x) { return x /= u; });
      return *this;
    }

    [[nodiscard]]
    friend constexpr vector_representation operator+(const vector_representation& lhs, const vector_representation& rhs) noexcept
    {
      return vector_representation{lhs} += rhs;
    }

    [[nodiscard]]
    friend constexpr vector_representation operator-(const vector_representation& lhs, const vector_representation& rhs) noexcept
    {
      return vector_representation{lhs} -= rhs;
    }

    [[nodiscard]]
    constexpr vector_representation operator+() const noexcept
    {
      return vector_representation{values()};
    }

    [[nodiscard]]
    constexpr vector_representation operator-() const noexcept
    {
      return vector_representation{to_array(values(), [](value_type t) { return -t; })};
    }

    [[nodiscard]]
    friend constexpr vector_representation operator*(vector_representation v, value_type u) noexcept
    {
      return v *= u;
    }

    [[nodiscard]]
    friend constexpr vector_representation operator*(value_type u, vector_representation v) noexcept
    {
      return v * u;
    }

    [[nodiscard]]
    friend constexpr vector_representation operator/(vector_representation v, value_type u)
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
    constexpr explicit operator bool() const noexcept requires (D == 1) && std::convertible_to<value_type, bool>
    {
      return m_Values[0];
    }

    [[nodiscard]]
    constexpr value_type operator[](std::size_t i) const { return m_Values[i]; }

    [[nodiscard]]
    constexpr value_type& operator[](std::size_t i) { return m_Values[i]; }

    [[nodiscard]]
    friend constexpr bool operator==(const vector_representation&, const vector_representation&) noexcept = default;

    [[nodiscard]]
    friend constexpr auto operator<=>(const vector_representation& lhs, const vector_representation& rhs) noexcept requires (D == 1) && std::totally_ordered<value_type>
    {
      return lhs.value() <=> rhs.value();
    }
  private:
    std::array<value_type, D> m_Values{};
  };
}
