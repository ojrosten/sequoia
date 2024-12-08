////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities relating to vector sets
 */

#include "sequoia/Core/Meta/TypeTraits.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
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
  inline constexpr bool has_plus_v{
    requires(T& t) {
      { t += t } -> std::same_as<T&>;
      { t + t }  -> std::same_as<T>;
    }
  };

  template<class T>
  inline constexpr bool has_minus_v{
    requires(T & t) {
      { t -= t } -> std::same_as<T&>;
      { t - t }  -> std::same_as<T>;
    }
  };

  template<class T>
  inline constexpr bool has_multiply_v{
    requires(T & t) {
      { t *= t } -> std::same_as<T&>;
      { t * t }  -> std::same_as<T>;
    }
  };

  template<class T>
  inline constexpr bool has_divide_v{
    requires(T & t) {
      { t /= t } -> std::same_as<T&>;
      { t / t }  -> std::same_as<T>;
    }
  };

  template<class T>
  struct weakly_abelian_group_under_addition : std::bool_constant<has_plus_v<T>> {};

  template<class T>
  using weakly_abelian_group_under_addition_t = typename weakly_abelian_group_under_addition<T>::type;

  template<class T>
  inline constexpr bool weakly_abelian_group_under_addition_v{weakly_abelian_group_under_addition<T>::value};

  template<class T>
  struct weakly_abelian_group_under_multiplication : std::false_type {};

  template<class T>
  using weakly_abelian_group_under_multiplication_t = typename weakly_abelian_group_under_multiplication<T>::type;

  template<class T>
  inline constexpr bool weakly_abelian_group_under_multiplication_v{weakly_abelian_group_under_multiplication<T>::value};

  template<std::floating_point T>
  struct weakly_abelian_group_under_multiplication<T> : std::true_type {};

  template<std::floating_point T>
  struct weakly_abelian_group_under_multiplication<std::complex<T>> : std::true_type {};

  template<class T>
  struct multiplication_weakly_distributive_over_addition : std::true_type {};

  template<class T>
  using multiplication_weakly_distributive_over_addition_t = typename multiplication_weakly_distributive_over_addition<T>::type;

  template<class T>
  inline constexpr bool multiplication_weakly_distributive_over_addition_v{multiplication_weakly_distributive_over_addition<T>::value};

  template<class T>
  concept weak_field 
    =    std::regular<T>
      && weakly_abelian_group_under_addition_v<T>
      && weakly_abelian_group_under_multiplication_v<T>
      && multiplication_weakly_distributive_over_addition_v<T>
      && has_plus_v<T>
      && has_minus_v<T>
      && has_multiply_v<T>
      && has_divide_v<T>;

  template<class T>
  inline constexpr bool has_dimension{
    requires { { T::dimension } -> std::convertible_to<std::size_t>; }
  };

  template<class T>
  inline constexpr bool has_field_type{
    requires { 
      typename T::field_type;
      requires weak_field<typename T::field_type>;
    }
  };

  template<class T>
  inline constexpr bool has_element_type{
    requires { typename T::element_type; }
  };

  template<class T>
  inline constexpr bool has_set_type{
    requires { typename T::set_type; }
  };

  template<class T>
  concept vector_space = has_set_type<T> && has_field_type<T> && has_dimension<T>; // TO DO: refer to vector axioms?

  template<class B>
  concept basis = requires { 
    typename B::vector_space_type;
    requires vector_space<typename B::vector_space_type>;
  };

  template<basis B, vector_space V>
  inline constexpr bool basis_for{std::is_same_v<typename B::vector_space_type, V>};

  template<class T>
  concept convex_space = requires {
    typename T::set_type;
    typename T::vector_space_type;
    requires vector_space<typename T::vector_space_type>;
  };

  template<class T>
  concept affine_space = convex_space<T>; //TO DO: more than a semantic difference?

  template<affine_space AffineSpace, basis Basis, class Origin>
    requires basis_for<Basis, typename AffineSpace::vector_space_type>
  class affine_coordinates;

  struct intrinsic_origin {};

  template<vector_space VectorSpace, basis Basis>
    requires basis_for<Basis, typename VectorSpace::vector_space_type>
  using vector_coordinates = affine_coordinates<VectorSpace, Basis, intrinsic_origin>;

  template<affine_space AffineSpace, basis Basis, class Origin>
    requires basis_for<Basis, typename AffineSpace::vector_space_type>
  class affine_coordinates
  {
  public:
    using affine_space_type = AffineSpace;
    using set_type          = typename AffineSpace::set_type;
    using vector_space_type = typename AffineSpace::vector_space_type;
    using basis_type        = Basis;
    using field_type        = typename vector_space_type::field_type;
    using value_type        = field_type;

    constexpr static bool is_vector_space{vector_space<AffineSpace> && std::is_same_v<Origin, intrinsic_origin>};
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
    friend constexpr auto operator<=>(const affine_coordinates& lhs, const affine_coordinates& rhs) noexcept
      requires (D == 1) && std::totally_ordered<value_type>
    {
      return lhs.value() <=> rhs.value();
    }
  private:
    std::array<value_type, D> m_Values{};
  };

  namespace sets
  {
    template<std::size_t N, std::floating_point T>
    struct R
    {
      constexpr static std::size_t dimension{N};
      using element_type = T;
    };

    template<std::size_t N, class T>
    struct C;

    template<std::size_t N, std::floating_point T>
    struct C<N, std::complex<T>>
    {
      constexpr static std::size_t dimension{N};
      using element_type = std::complex<T>;
    };
  }

  template<class B>
  inline constexpr bool is_orthonormal_basis_v{
    requires {
      typename B::orthonormal;
      requires std::same_as<typename B::orthonormal, std::true_type>;
    }
  };

  template<vector_space V>
  struct arbitary_basis {};

  template<vector_space V>
  inline constexpr bool has_norm_v{
    requires (const vector_coordinates<vector_space, arbitary_basis<V>>& v) {
      { norm(v) } -> std::convertible_to<typename V::field_type>;
    }
  };

  template<vector_space V>
  inline constexpr bool has_inner_product_v{
    requires (const vector_coordinates<vector_space, arbitary_basis<V>>& v) {
      { inner_product(v, v) } -> std::convertible_to<typename V::field_type>;
    }
  };

  template<class V>
  concept normed_vector_space = vector_space<V> && has_norm_v<V>;

  template<class V>
  concept inner_product_space = vector_space<V> && has_inner_product_v<V>;

  template<std::size_t D, std::floating_point T>
  struct euclidean_vector_space
  {
    using set_type          = sets::R<D, T>;
    using field_type        = T;
    using vector_space_type = euclidean_vector_space;
    constexpr static std::size_t dimension{D};

    template<basis Basis>
      requires is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr field_type inner_product(const vector_coordinates<euclidean_vector_space, Basis>& v, const vector_coordinates<euclidean_vector_space, Basis>& w)
    {
      return std::ranges::fold_left(std::views::zip(v.values(), w.values()), field_type{}, [](field_type f, const auto& z){ return f + std::get<0>(z) * std::get<1>(z); });
    }

    template<basis Basis>
      requires is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr field_type dot(const vector_coordinates<euclidean_vector_space, Basis>& v, const vector_coordinates<euclidean_vector_space, Basis>& w)
    {
      return inner_product(v, w);
    }

    template<basis Basis>
      requires is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr field_type norm(const vector_coordinates<euclidean_vector_space, Basis>& v)
    {
      if constexpr(D == 1)
      {
        return std::abs(v.value());
      }
      else
      {
        return std::sqrt(inner_product(v, v));
      }
    }
  };

  template<std::size_t D, std::floating_point T>
  struct euclidean_affine_space
  {
    using set_type          = sets::R<D, T>;
    using vector_space_type = euclidean_vector_space<D, T>;
  };

  template<std::size_t D, std::floating_point T, basis Basis, class Origin>
  using euclidean_affine_coordinates = affine_coordinates<euclidean_affine_space<D, T>, Basis, Origin>;

  template<std::size_t D, std::floating_point T, basis Basis>
  using euclidean_vector_coordinates = vector_coordinates<euclidean_vector_space<D, T>, Basis>;

  template<std::size_t D, std::floating_point T>
  struct standard_basis
  {
    using vector_space_type = euclidean_vector_space<D, T>;
    using orthonormal = std::true_type;
  };

  template<std::size_t D, std::floating_point T>
  using vec_coords = euclidean_vector_coordinates<D, T, standard_basis<D, T>>;
}
