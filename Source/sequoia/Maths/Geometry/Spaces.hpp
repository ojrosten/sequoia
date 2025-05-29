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


#include "sequoia/Core/Meta/TypeAlgorithms.hpp"
#include "sequoia/Core/Meta/TypeTraits.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <complex>
#include <format>
#include <ranges>
#include <span>

namespace sequoia::maths
{
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
  inline constexpr bool addable_v{
    requires(T& t) {
      { t += t } -> std::same_as<T&>;
      { t + t }  -> std::same_as<T>;
    }
  };

  template<class T>
  inline constexpr bool subtractable_v{
    requires(T& t) {
      { t -= t } -> std::same_as<T&>;
      { t - t }  -> std::same_as<T>;
    }
  };

  template<class T>
  inline constexpr bool multiplicable_v{
    requires(T& t) {
      { t *= t } -> std::same_as<T&>;
      { t * t }  -> std::same_as<T>;
    }
  };

  template<class T>
  inline constexpr bool divisible_v{
    requires(T& t) {
      { t /= t } -> std::same_as<T&>;
      { t / t }  -> std::same_as<T>;
    }
  };

  template<class T>
  struct weakly_abelian_group_under_addition : std::bool_constant<addable_v<T>> {};

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
  concept weak_commutative_ring 
    =    std::regular<T>
      && weakly_abelian_group_under_addition_v<T>
      && multiplication_weakly_distributive_over_addition_v<T>
      && subtractable_v<T>
      && multiplicable_v<T>;
  
  template<class T>
  concept weak_field = weak_commutative_ring<T> && weakly_abelian_group_under_multiplication_v<T> && divisible_v<T>;

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
  inline constexpr bool has_commutative_ring_type{
        has_field_type<T>
    ||  requires { 
          typename T::commutative_ring_type;
          requires weak_commutative_ring<typename T::commutative_ring_type>;
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
  inline constexpr bool has_vector_space_type_v{
    requires { typename T::vector_space_type; }
  };

  template<class T>
  inline constexpr bool has_free_module_type_v{
    requires { typename T::free_module_type; }
  };

  template<class T>
  inline constexpr bool identifies_as_vector_space_v{
    has_vector_space_type_v<T> && requires { requires std::same_as<T, typename T::vector_space_type>; }
  };

  template<class T>
  inline constexpr bool identifies_as_free_module_v{
       identifies_as_vector_space_v<T>
    || (has_free_module_type_v<T> && requires { requires std::same_as<T, typename T::free_module_type>; })
  };

  template<class T>
  concept vector_space = has_set_type<T> && has_dimension<T> && has_field_type<T>  && identifies_as_vector_space_v<T>;

  template<class T>
  concept free_module = has_set_type<T> && has_dimension<T> && has_commutative_ring_type<T> && identifies_as_free_module_v<T>;

  // Universal template parameters will obviate the need for this
  template<class T>
  struct is_vector_space : std::integral_constant<bool, vector_space<T>> {};

  template<class B>
  concept basis =
      (has_free_module_type_v<B>  && requires { requires  free_module<typename B::free_module_type>; })
    ||(has_vector_space_type_v<B> && requires { requires vector_space<typename B::vector_space_type>; });

  template<class B, class M>
  concept basis_for = basis<B> && (    requires { requires std::is_same_v<typename B::free_module_type, M> ; }
                                    || requires { requires std::is_same_v<typename B::vector_space_type, M>; });

  template<class T>
  inline constexpr bool identifies_as_convex_space_v{
    requires {
      typename T::convex_space_type;
      requires std::same_as<T, typename T::convex_space_type>;
    }
  };

  template<class T>
  inline constexpr bool identifies_as_affine_space_v{
    requires {
      typename T::affine_space_type;
      requires std::same_as<T, typename T::affine_space_type>;
    }
  };

  template<class T>
  concept convex_space
    =    (identifies_as_convex_space_v<T> || identifies_as_affine_space_v<T> || identifies_as_vector_space_v<T> || identifies_as_free_module_v<T>)
      && requires {  typename T::set_type; }
      && (    requires {
                typename T::vector_space_type;
                requires vector_space<typename T::vector_space_type>;
              }
          || requires {
                typename T::free_module_type;
                requires free_module<typename T::free_module_type>;
              }
         );

  template<convex_space ConvexSpace>
  struct free_module_type_of
  {
    using type = ConvexSpace::free_module_type;
  };

  template<convex_space ConvexSpace>
    requires has_vector_space_type_v<ConvexSpace>
  struct free_module_type_of<ConvexSpace>
  {
    using type = ConvexSpace::vector_space_type;
  };

  template<convex_space ConvexSpace>
  using free_module_type_of_t = free_module_type_of<ConvexSpace>::type;

  template<convex_space ConvexSpace>
  struct commutative_ring_type_of
  {
    using type = ConvexSpace::free_module_type::commutative_ring_type;
  };

  template<convex_space ConvexSpace>
    requires has_vector_space_type_v<ConvexSpace>
  struct commutative_ring_type_of<ConvexSpace>
  {
    using type = ConvexSpace::vector_space_type::field_type;
  };

  template<convex_space ConvexSpace>
  using commutative_ring_type_of_t = commutative_ring_type_of<ConvexSpace>::type;

  template<convex_space ConvexSpace>
  using space_value_type = commutative_ring_type_of_t<ConvexSpace>;

  template<convex_space ConvexSpace>
  inline constexpr std::size_t dimension_of{free_module_type_of_t<ConvexSpace>::dimension};

  template<class T>
  concept affine_space = convex_space<T> && (identifies_as_affine_space_v<T> || identifies_as_vector_space_v<T> || identifies_as_free_module_v<T>);

  template<class V, class ConvexSpace>
  inline constexpr bool validator_for_single_value{
       (dimension_of<ConvexSpace> == 1)
    && requires(V& v, const space_value_type<ConvexSpace>& val) { { v(val) } -> std::convertible_to<decltype(val)>; }
  };

  template<class V, class ConvexSpace>
  inline constexpr bool validator_for_array{
    requires (V& v, std::array<space_value_type<ConvexSpace>, dimension_of<ConvexSpace>> values) {
      { v(values) } -> std::convertible_to<decltype(values)>;
    }
  };

  template<class V, class ConvexSpace>
  concept validator_for =
       convex_space<ConvexSpace>
    && std::default_initializable<V>
    && std::constructible_from<V, V>
    && (validator_for_single_value<V, ConvexSpace> || validator_for_array<V, ConvexSpace>);

  struct half_line_validator
  {
    template<std::floating_point T>
    [[nodiscard]]
    constexpr T operator()(const T val) const
    {
      if(val < T{}) throw std::domain_error{std::format("Value {} less than zero", val)};

      return val;
    }
  };

  template<class T>
  struct defines_half_line : std::false_type {};

  template<class T>
  using defines_half_line_t = typename defines_half_line<T>::type;

  template<class T>
  inline constexpr bool defines_half_line_v{defines_half_line<T>::value};

  template<>
  struct defines_half_line<half_line_validator> : std::true_type {};

  struct intrinsic_origin {};

  template<
    convex_space ConvexSpace,
    basis_for<free_module_type_of_t<ConvexSpace>>  Basis,
    class Origin,
    validator_for<ConvexSpace> Validator,
    class DisplacementCoordinates
  >
  class coordinates_base;

  template<
    convex_space ConvexSpace,
    basis_for<free_module_type_of_t<ConvexSpace>> Basis,
    class Origin,
    validator_for<ConvexSpace> Validator
  >
  class coordinates;

  template<affine_space AffineSpace, basis_for<free_module_type_of_t<AffineSpace>> Basis, class Origin>
  using affine_coordinates = coordinates<AffineSpace, Basis, Origin, std::identity>;

  template<vector_space VectorSpace, basis_for<free_module_type_of_t<VectorSpace>> Basis>
  using vector_coordinates = affine_coordinates<VectorSpace, Basis, intrinsic_origin>;

  template<free_module FreeModule, basis_for<free_module_type_of_t<FreeModule>> Basis>
  using free_module_coordinates = affine_coordinates<FreeModule, Basis, intrinsic_origin>;

  //============================== direct_product ==============================//

  template<class... Ts>
  struct direct_product;
  
  template<class... Ts>
  using direct_product_set_t = direct_product<Ts...>::set_type;  

  template<vector_space T, vector_space U>
  struct direct_product<T, U>
  {
    using set_type   = direct_product<typename T::set_type, typename U::set_type>;
    using field_type = std::common_type_t<typename T::field_type, typename U::field_type>;
    constexpr static std::size_t dimension{T::dimension + U::dimension};
    using vector_space_type = direct_product<T, U>;
  };
  
  template<affine_space T, affine_space U>
    requires (!vector_space<T> && !vector_space<U>)
  struct direct_product<T, U>
  {
    using set_type          = direct_product<typename T::set_type, typename U::set_type>;
    using vector_space_type = direct_product<typename T::vector_space_type, typename U::vector_space_type>;
    using affine_space_type = direct_product<T, U>;
  };

  template<convex_space T, convex_space U>
    requires (!affine_space<T> && !affine_space<U>)
  struct direct_product<T, U>
  {
    using set_type          = direct_product<typename T::set_type, typename U::set_type>;
    using vector_space_type = direct_product<typename T::vector_space_type, typename U::vector_space_type>;
    using convex_space_type = direct_product<T, U>;
  };

  // Types assumed to be ordered
  // Could add this as a constraint, at the cost of changing
  // the algorithmic complexity...
  template<vector_space... Ts>
  struct direct_product<std::tuple<Ts...>>
  {
    using set_type   = std::tuple<typename Ts::set_type...>;
    using field_type = std::common_type_t<typename Ts::field_type...>;
    constexpr static auto dimension{(Ts::dimension + ...)};
  };
  
  // Types assumed to be ordered wrt type_comparator, but dependent types may not be against the same comparator
  template<convex_space... Ts>
    requires (!vector_space<Ts> && ...)
  struct direct_product<std::tuple<Ts...>>
  {
    using set_type          = std::tuple<typename Ts::set_type...>;
    using vector_space_type = direct_product<typename Ts::vector_space_type...>;
  };

  template<class>
  struct extract_common_field_type;

  template<class T>
  using extract_common_field_type_t = extract_common_field_type<T>::type;

  template<convex_space... Ts>
  struct extract_common_field_type<std::tuple<Ts...>>
  {
    using type = std::common_type_t<typename Ts::field_type...>;
  };

  template<convex_space... Ts>
    requires (vector_space<Ts> || ...) && (!vector_space<Ts> || ...)
  struct direct_product<std::tuple<Ts...>>
  {
    using set_type          = std::tuple<typename Ts::set_type...>;
    using field_type        = extract_common_field_type_t<meta::filter_by_trait_t<std::tuple<Ts...>, is_vector_space>>;
    using vector_space_type = direct_product<typename Ts::vector_space_type...>;
  };

  //==============================  dual spaces ============================== //
  template<class>
  struct dual;
  
  template<convex_space C>
    requires (!affine_space<C>)
  struct dual<C>
  {
    using set_type          = C::set_type;
    using vector_space_type = dual<typename C::vector_space_type>;
    using convex_space_type = dual<C>;
  };

  template<affine_space C>
    requires (!vector_space<C>)
  struct dual<C>
  {
    using set_type          = C::set_type;
    using vector_space_type = dual<typename C::vector_space_type>;
    using affine_space_type = dual<C>;
  };

  template<vector_space V>
  struct dual<V>
  {
    using set_type   = V::set_type; // TO DO: think about this
    using field_type = V::field_type;
    constexpr static auto dimension{V::dimension};
    using vector_space_type = dual<V>;
  };

  template<class C>
  struct is_dual : std::false_type {};

  template<class C>
  struct is_dual<dual<C>> : std::true_type {};

  template<class C>
  using is_dual_t = is_dual<C>::type;

  template<class C>
  inline constexpr bool is_dual_v{is_dual<C>::value};

  template<class>
  struct dual_of;

  template<class T>
  using dual_of_t = dual_of<T>::type;

  template<class C>
  struct dual_of {
    using type = dual<C>;
  };

  template<class C>
  struct dual_of<dual<C>> {
    using type = C;
  };
}

namespace sequoia::maths
{  
  //============================== coordinates_base definition  ==============================//

  template<
    convex_space ConvexSpace,
    basis_for<free_module_type_of_t<ConvexSpace>> Basis,
    class Origin,
    validator_for<ConvexSpace> Validator,
    class DisplacementCoordinates=free_module_coordinates<free_module_type_of_t<ConvexSpace>, Basis>
  >
  class coordinates_base
  {
  public:
    using convex_space_type             = ConvexSpace;   
    using basis_type                    = Basis;
    using validator_type                = Validator;
    using origin_type                   = Origin;
    using set_type                      = ConvexSpace::set_type;
    using free_module_type              = free_module_type_of_t<ConvexSpace>;
    using commutative_ring_type         = commutative_ring_type_of_t<ConvexSpace>;
    using value_type                    = commutative_ring_type;
    using displacement_coordinates_type = DisplacementCoordinates;

    constexpr static bool has_intrinsic_origin{std::is_same_v<Origin, intrinsic_origin>};
    constexpr static bool has_identity_validator{std::is_same_v<Validator, std::identity>};
    constexpr static std::size_t dimension{free_module_type::dimension};
    constexpr static std::size_t D{dimension};

    constexpr coordinates_base() noexcept = default;

    constexpr explicit coordinates_base(std::span<const value_type, D> vals) noexcept(has_identity_validator)
      : m_Values{validate(vals, m_Validator)}
    {}

    template<class... Ts>
      requires (D > 1) && (sizeof...(Ts) == D) && (std::convertible_to<Ts, value_type> && ...)
    constexpr coordinates_base(Ts... ts) noexcept(has_identity_validator)
      : m_Values{m_Validator(std::array<value_type, D>{ts...})}
    {}

    template<class T>
      requires (D == 1) && (std::convertible_to<T, value_type>)
    constexpr explicit coordinates_base(T val) noexcept(has_identity_validator)
      : m_Values{m_Validator(val)}
    {}

    template<class Self>
    constexpr Self& operator+=(this Self& self, const displacement_coordinates_type& v) noexcept(has_identity_validator)
    {
      self.apply_to_each_element(v.values(), [](value_type& lhs, value_type rhs){ lhs += rhs; });
      return self;
    }

    template<class Self>
      requires has_intrinsic_origin && (!std::is_same_v<coordinates_base, displacement_coordinates_type>)
    constexpr Self& operator+=(this Self& self, const coordinates_base& v) noexcept(has_identity_validator)
    {
      self.apply_to_each_element(v.values(), [](value_type& lhs, value_type rhs){ lhs += rhs; });
      return self;
    }

    template<class Self>
    constexpr Self& operator-=(this Self& self, const displacement_coordinates_type& v) noexcept(has_identity_validator)
    {
      self.apply_to_each_element(v.values(), [](value_type& lhs, value_type rhs){ lhs -= rhs; });
      return self;
    }

    template<class Self>
    constexpr Self& operator*=(this Self& self, value_type u) noexcept(has_identity_validator)
      requires has_intrinsic_origin
    {
      self.for_each_element([u](value_type& x) { return x *= u; });
      return self;
    }

    template<class Self>
    constexpr Self& operator/=(this Self& self, value_type u)
      requires vector_space<free_module_type> && has_intrinsic_origin
    {
      self.for_each_element([u](value_type& x) { return x /= u; });
      return self;
    }

    template<class Derived>
      requires std::is_base_of_v<coordinates_base, Derived>
    [[nodiscard]]
    friend constexpr Derived operator+(Derived c, const displacement_coordinates_type& v) noexcept(has_identity_validator) { return c += v; }

    template<class Derived>
    requires std::is_base_of_v<coordinates_base, Derived> && (!std::is_same_v<Derived, displacement_coordinates_type>)
    [[nodiscard]]
    friend constexpr Derived operator+(const displacement_coordinates_type& v, Derived c) noexcept(has_identity_validator)
    {
      return c += v;
    }
  
    template<class Derived>
      requires std::is_base_of_v<coordinates_base, Derived> && (!std::is_same_v<Derived, displacement_coordinates_type>) && has_intrinsic_origin 
    [[nodiscard]]
    friend constexpr Derived operator+(Derived c, const Derived& v) noexcept(has_identity_validator)
    {
      return c += v;
    }

    template<class Derived>
      requires std::is_base_of_v<coordinates_base, Derived>
    [[nodiscard]]
    friend constexpr Derived operator-(Derived c, const displacement_coordinates_type& v) noexcept(has_identity_validator)
    {
      return c -= v;
    }

    template<class Derived>
      requires std::is_base_of_v<coordinates_base, Derived>
    [[nodiscard]]
    friend constexpr Derived operator*(Derived v, value_type u) noexcept(has_identity_validator)
      requires has_intrinsic_origin
    {
      return v *= u;
    }

    template<class Derived>
      requires std::is_base_of_v<coordinates_base, Derived>
    [[nodiscard]]
    friend constexpr Derived operator*(value_type u, Derived v) noexcept(has_identity_validator)
      requires has_intrinsic_origin
    {
      return v * u;
    }

    template<class Derived>
      requires std::is_base_of_v<coordinates_base, Derived> && vector_space<free_module_type>
    [[nodiscard]]
    friend constexpr Derived operator/(Derived v, value_type u)
      requires has_intrinsic_origin
    {
      return v /= u;
    }

    [[nodiscard]]
    constexpr const validator_type& validator() const noexcept { return m_Validator; }

    [[nodiscard]]
    constexpr std::span<const value_type, D> values() const noexcept { return m_Values; }

    [[nodiscard]]
    constexpr std::span<value_type, D> values() noexcept requires(has_identity_validator) { return m_Values; }

    [[nodiscard]]
    constexpr const value_type& value() const noexcept requires (D == 1) { return m_Values[0]; }

    [[nodiscard]]
    constexpr value_type& value() noexcept requires (D == 1) && has_identity_validator { return m_Values[0]; }

    // Make this explicit since otherwise, given two vectors a,b, a/b is well-formed due to implicit boolean conversion
    [[nodiscard]]
    constexpr explicit operator bool() const noexcept requires (D == 1) && std::convertible_to<value_type, bool>
    {
      return m_Values[0];
    }

    [[nodiscard]]
    constexpr value_type operator[](std::size_t i) const { return m_Values[i]; }

    [[nodiscard]]
    constexpr value_type& operator[](std::size_t i) requires(has_identity_validator) { return m_Values[i]; }

    [[nodiscard]]
    friend constexpr bool operator==(const coordinates_base& lhs, const coordinates_base& rhs) noexcept { return lhs.m_Values == rhs.m_Values; }

    [[nodiscard]]
    friend constexpr auto operator<=>(const coordinates_base& lhs, const coordinates_base& rhs) noexcept
      requires (D == 1) && std::totally_ordered<value_type>
    {
      return lhs.value() <=> rhs.value();
    }
  protected:
    coordinates_base(const coordinates_base&)     = default;
    coordinates_base(coordinates_base&&) noexcept = default;

    coordinates_base& operator=(const coordinates_base&)     = default;
    coordinates_base& operator=(coordinates_base&&) noexcept = default;

    ~coordinates_base() = default;
    
    template<class Self, class Fn>
      requires std::invocable<Fn, value_type&, value_type>
    constexpr void apply_to_each_element(this Self& self, std::span<const value_type, D> rhs, Fn f)
    {
      if constexpr(std::same_as<Validator, std::identity>)
      {
        std::ranges::for_each(std::views::zip(self.values(), rhs), [&f](auto&& z){ f(std::get<0>(z), std::get<1>(z)); });
      }
      else
      {
        auto tmp{self.m_Values};
        std::ranges::for_each(std::views::zip(tmp, rhs), [&f](auto&& z){ f(std::get<0>(z), std::get<1>(z)); });
        self.m_Values = validate(tmp, self.m_Validator);
      }
    }

    template<class Self, class Fn>
      requires std::invocable<Fn, value_type&>
    constexpr void for_each_element(this Self& self, Fn f)
    {
      if constexpr(std::same_as<Validator, std::identity>)
      {
        std::ranges::for_each(self.values(), f);
      }
      else
      {
        auto tmp{self.m_Values};
        std::ranges::for_each(tmp, f);
        self.m_Values = validate(tmp, self.m_Validator);
      }
    }
  private:
    SEQUOIA_NO_UNIQUE_ADDRESS validator_type m_Validator;
    std::array<value_type, D> m_Values{};

    [[nodiscard]]
    static std::array<value_type, D> validate(std::span<const value_type, D> vals, Validator& validator)
    {
      return validate(to_array(vals), validator);
    }

    [[nodiscard]]
    static std::array<value_type, D> validate(std::array<value_type, D> vals, Validator& validator)
    {
      if constexpr(validator_for_array<Validator, ConvexSpace>)
        return validator(vals);
      else
        return {validator(vals.front())};
    }
  };
  
  template<convex_space ConvexSpace, basis_for<free_module_type_of_t<ConvexSpace>> Basis, class Origin, validator_for<ConvexSpace> Validator>
  class coordinates final : public coordinates_base<ConvexSpace, Basis, Origin, Validator>
  {
  public:
    using base_type = coordinates_base<ConvexSpace, Basis, Origin, Validator>;
    using base_type::base_type;
    using displacement_coordinates_type = base_type::displacement_coordinates_type;
    using value_type                    = base_type::value_type;
    constexpr static bool has_identity_validator{base_type::has_identity_validator};

    [[nodiscard]]
    friend constexpr displacement_coordinates_type operator-(const coordinates& lhs, const coordinates& rhs) noexcept(has_identity_validator)
    {
      return[&] <std::size_t... Is>(std::index_sequence<Is...>) {
        return displacement_coordinates_type{(lhs.values()[Is] - rhs.values()[Is])...};
      }(std::make_index_sequence<base_type::D>{});
    }

    [[nodiscard]]
    constexpr coordinates operator+() const noexcept(has_identity_validator)
    {
      return coordinates{this->values()};
    }

    [[nodiscard]]
    constexpr coordinates operator-() const noexcept(has_identity_validator)
    {
      return coordinates{to_array(this->values(), [](value_type t) { return -t; })};
    }
  };

  namespace sets
  {
    template<std::size_t N, class T>
      requires std::is_same_v<T, short> || std::is_same_v<T, int> || std::is_same_v<T, long> || std::is_same_v<T, long long>
    struct Z
    {
      constexpr static std::size_t dimension{N};
      using element_type = T;
    };
    
    template<std::size_t N, std::floating_point T>
    struct R
    {
      constexpr static std::size_t dimension{N};
      using element_type = T;
    };

    template<std::size_t N, std::floating_point T>
    struct half_space
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
    requires (const vector_coordinates<V, arbitary_basis<V>>& v) {
      { norm(v) } -> std::convertible_to<typename V::field_type>;
    }
  };

  template<vector_space V>
  inline constexpr bool has_inner_product_v{
    requires (const vector_coordinates<V, arbitary_basis<V>>& v) {
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
    using affine_space_type = euclidean_affine_space;
  };

  template<std::size_t D, std::floating_point T>
  struct euclidean_half_space
  {
    using set_type          = sets::half_space<D, T>;
    using vector_space_type = euclidean_vector_space<D, T>;
    using convex_space_type = euclidean_half_space;
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
