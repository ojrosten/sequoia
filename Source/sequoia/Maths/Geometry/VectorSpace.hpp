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

#include <algorithm>
#include <ranges>
#include <span>

namespace sequoia::maths
{
  template<class T, std::size_t D, class Fn>
    requires std::invocable<Fn, T&, T>
  constexpr void apply_to_each_element(std::array<T, D>& lhs, std::span<const T, D> rhs, Fn f)
  {
    //std::ranges::for_each(std::views::zip(lhs, rhs), [&f](auto&& z){ f(std::get<0>(z), std::get<1>(z)); });
    [&] <std::size_t... Is>(std::index_sequence<Is...>){
      (f(lhs[Is], rhs[Is]), ...);
    }(std::make_index_sequence<D>{});
  }

  template<class T, std::size_t D, class Fn = std::identity>
    requires std::is_invocable_r_v<T, Fn, T>
  [[nodiscard]]
  constexpr std::array<T, D> to_array(std::span<const T, D> a, Fn f = {})
  {
    return[&] <std::size_t... Is> (std::index_sequence<Is...>){
      return std::array<T, D>{f(a[Is])...};
    }(std::make_index_sequence<D>{});
  }

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
  concept basis = vector_space<VectorSpace>; // TO DO; does it even make sense to have a basis concept?

  template<class T>
  inline constexpr bool has_set_type{
    requires { typename T::set_type; }
  };

  template<class T>
  inline constexpr bool has_vector_space_type{
    requires { typename T::vector_space_type; }
  };

  template<class T>
  concept affine_space = requires {
    typename T::set_type;
    typename T::vector_space_type;
    requires vector_space<typename T::vector_space_type>;
  };

  template</*vector_space*/ class VectorSpace>
  struct vector_space_as_affine_space
  {
    using set_type          = VectorSpace;
    using vector_space_type = VectorSpace;
  };

  template<affine_space AffineSpace, basis<typename AffineSpace::vector_space_type> Basis, class Origin>
  class affine_coordinates;

  struct intrinsic_origin {};

  template<vector_space VectorSpace, basis<VectorSpace> Basis>
  using vector_coordinates = affine_coordinates<vector_space_as_affine_space<VectorSpace>, Basis, intrinsic_origin>;

  template<affine_space AffineSpace, basis<typename AffineSpace::vector_space_type> Basis, class Origin>
  class affine_coordinates
  {
  public:
    using affine_space_type = AffineSpace;
    using set_type          = typename AffineSpace::set_type;
    using vector_space_type = typename AffineSpace::vector_space_type;
    using basis_type        = Basis;
    using field_type        = typename vector_space_type::field_type;
    using value_type        = field_type;

    constexpr static bool is_vector_space{std::is_same_v<set_type, vector_space_type> && std::is_same_v<Origin, intrinsic_origin>};
    constexpr static std::size_t dimension{vector_space_type::dimension};
    constexpr static std::size_t D{dimension};

    constexpr affine_coordinates() noexcept = default;

    constexpr explicit affine_coordinates(std::span<const value_type, D> d) noexcept
      : m_Values{to_array(d)}
    {}

    constexpr explicit affine_coordinates(std::span<value_type, D> d) noexcept
      : m_Values{to_array(d)}
    {}

    template<class... Ts>
      requires (sizeof...(Ts) == D) && (is_initializable_v<value_type, Ts> && ...)
    constexpr affine_coordinates(Ts... ts) noexcept
      : m_Values{ts...}
    {}

    constexpr affine_coordinates& operator+=(const vector_coordinates<vector_space_type, Basis>& v) noexcept {
      apply_to_each_element(m_Values, v.values(), [](value_type& lhs, value_type rhs){ lhs += rhs; });
      return *this;
    }

    constexpr affine_coordinates& operator-=(const vector_coordinates<vector_space_type, Basis>& v) noexcept {
      apply_to_each_element(m_Values, v.values(), [](value_type& lhs, value_type rhs){ lhs -= rhs; });
      return *this;
    }

    constexpr affine_coordinates& operator*=(value_type u) noexcept
      requires is_vector_space
    {
      std::ranges::for_each(m_Values, [u](value_type& x) { return x *= u; });
      return *this;
    }

    constexpr affine_coordinates& operator/=(value_type u)
      requires is_vector_space
    {
      std::ranges::for_each(m_Values, [u](value_type& x) { return x /= u; });
      return *this;
    }

    [[nodiscard]]
    friend constexpr affine_coordinates operator+(affine_coordinates c, const vector_coordinates<vector_space_type, Basis>& v) noexcept { return c += v; }

    [[nodiscard]]
    friend constexpr affine_coordinates operator+(const vector_coordinates<vector_space_type, Basis>& v, affine_coordinates c) noexcept
      requires (!is_vector_space)
    {
      return c += v;
    }

    [[nodiscard]]
    friend constexpr affine_coordinates operator-(affine_coordinates c, const vector_coordinates<vector_space_type, Basis>& v) noexcept
      requires (!is_vector_space)
    {
      return c -= v;
    }

    [[nodiscard]]
    friend constexpr affine_coordinates operator-(const vector_coordinates<vector_space_type, Basis>& v, affine_coordinates c) noexcept
      requires (!is_vector_space)
    {
      return c -= v;
    }

    [[nodiscard]]
    friend constexpr vector_coordinates<vector_space_type, Basis> operator-(const affine_coordinates& lhs, const affine_coordinates& rhs) noexcept
    {
      return[&] <std::size_t... Is>(std::index_sequence<Is...>) {
        return vector_coordinates<vector_space_type, Basis>{(lhs.values()[Is] - rhs.values()[Is])...};
      }(std::make_index_sequence<D>{});
    }

    [[nodiscard]]
    constexpr affine_coordinates operator+() const noexcept
    {
      return affine_coordinates{values()};
    }

    [[nodiscard]]
    constexpr affine_coordinates operator-() const noexcept
    {
      return affine_coordinates{to_array(values(), [](value_type t) { return -t; })};
    }

    [[nodiscard]]
    friend constexpr affine_coordinates operator*(affine_coordinates v, value_type u) noexcept
      requires is_vector_space
    {
      return v *= u;
    }

    [[nodiscard]]
    friend constexpr affine_coordinates operator*(value_type u, affine_coordinates v) noexcept
      requires is_vector_space
    {
      return v * u;
    }

    [[nodiscard]]
    friend constexpr affine_coordinates operator/(affine_coordinates v, value_type u)
      requires is_vector_space
    {
      return v /= u;
    }

    [[nodiscard]]
    constexpr std::span<const value_type, D> values() const noexcept { return m_Values; }

    [[nodiscard]]
    constexpr std::span<value_type, D> values() noexcept { return m_Values; }

    [[nodiscard]]
    constexpr const value_type& value() const noexcept requires (D == 1) { return m_Values[0]; }

    [[nodiscard]]
    constexpr value_type& value() noexcept requires (D == 1) { return m_Values[0]; }

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
    friend constexpr bool operator==(const affine_coordinates&, const affine_coordinates&) noexcept = default;

    [[nodiscard]]
    friend constexpr auto operator<=>(const affine_coordinates& lhs, const affine_coordinates& rhs) noexcept requires (D == 1) && std::totally_ordered<value_type>
    {
      return lhs.value() <=> rhs.value();
    }
  private:
    std::array<value_type, D> m_Values{};
  };
}
