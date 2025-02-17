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
    requires(T & t) {
      { t -= t } -> std::same_as<T&>;
      { t - t }  -> std::same_as<T>;
    }
  };

  template<class T>
  inline constexpr bool multiplicable_v{
    requires(T & t) {
      { t *= t } -> std::same_as<T&>;
      { t * t }  -> std::same_as<T>;
    }
  };

  template<class T>
  inline constexpr bool divisible_v{
    requires(T & t) {
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
  concept weak_field 
    =    std::regular<T>
      && weakly_abelian_group_under_addition_v<T>
      && weakly_abelian_group_under_multiplication_v<T>
      && multiplication_weakly_distributive_over_addition_v<T>
      && addable_v<T>
      && subtractable_v<T>
      && multiplicable_v<T>
      && divisible_v<T>;

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
  inline constexpr bool identifies_as_vector_space_v{
    requires {
      typename T::vector_space_type;
      requires std::same_as<T, typename T::vector_space_type>;
    }
  };

  template<class T>
  concept vector_space = has_set_type<T> && has_field_type<T> && has_dimension<T> && identifies_as_vector_space_v<T>;

  // Universal template parameters will obviate the need for this
  template<class T>
  struct is_vector_space : std::integral_constant<bool, vector_space<T>> {};

  template<class B>
  concept basis = requires { 
    typename B::vector_space_type;
    requires vector_space<typename B::vector_space_type>;
  };

  template<class B, class V>
  concept basis_for = basis<B> && vector_space<V> && std::is_same_v<typename B::vector_space_type, V>;

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
    =    (identifies_as_convex_space_v<T> || identifies_as_affine_space_v<T> || identifies_as_vector_space_v<T>)
      && requires {
           typename T::set_type;
           typename T::vector_space_type;
           requires vector_space<typename T::vector_space_type>;
         };

  template<convex_space ConvexSpace>
  using vector_space_type_of = typename ConvexSpace::vector_space_type;

  template<convex_space ConvexSpace>
  using space_field_type = typename vector_space_type_of<ConvexSpace>::field_type;

  template<convex_space ConvexSpace>
  using space_value_type = space_field_type<ConvexSpace>;

  template<convex_space ConvexSpace>
  inline constexpr std::size_t dimension_of{vector_space_type_of<ConvexSpace>::dimension};

  template<class T>
  concept affine_space = convex_space<T> && (identifies_as_affine_space_v<T> || identifies_as_vector_space_v<T>);

  template<class V, class ConvexSpace>
  concept validator_for =
       convex_space<ConvexSpace>
    && std::default_initializable<V>
    && std::constructible_from<V, V>
    && (    requires (V& v, std::array<const space_value_type<ConvexSpace>, dimension_of<ConvexSpace>> values) {
              { v(values) } -> std::convertible_to<decltype(values)>;
            }
         || (   (dimension_of<ConvexSpace> == 1)
             && requires(V & v, const space_value_type<ConvexSpace>& val) { { v(val) } -> std::convertible_to<decltype(val)>; })
       );

  struct half_space_validator
  {
    template<std::floating_point T>
    [[nodiscard]]
    constexpr T operator()(const T val) const
    {
      if(val < T{}) throw std::domain_error{std::format("Value {} less than zero", val)};

      return val;
    }

    template<std::floating_point T>
    [[nodiscard]]
    constexpr std::array<T, 1> operator()(std::array<T, 1> val) const
    {
      return {operator()(val.front())};
    }
  };

  template<class T>
  struct defines_half_space : std::false_type {};

  template<class T>
  using defines_half_space_t = typename defines_half_space<T>::type;

  template<class T>
  inline constexpr bool defines_half_space_v{defines_half_space<T>::value};

  template<>
  struct defines_half_space<half_space_validator> : std::true_type {};

  struct intrinsic_origin {};

  template<
    convex_space ConvexSpace,
    basis_for<vector_space_type_of<ConvexSpace>>  Basis,
    class Origin,
    validator_for<ConvexSpace> Validator,
    class DisplacementCoordinates
  >
  class coordinates_base;

  template<
    convex_space ConvexSpace,
    basis_for<vector_space_type_of<ConvexSpace>> Basis,
    class Origin,
    validator_for<ConvexSpace> Validator
  >
  class coordinates;

  template<affine_space AffineSpace, basis_for<vector_space_type_of<AffineSpace>> Basis, class Origin>
  using affine_coordinates = coordinates<AffineSpace, Basis, Origin, std::identity>;

  template<vector_space VectorSpace, basis_for<vector_space_type_of<VectorSpace>> Basis>
  using vector_coordinates = affine_coordinates<VectorSpace, Basis, intrinsic_origin>;

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
  // TO DO: think about whether there's a proper mathematical representation for
  // convex_spaces. I have an informal notion of what I'm trying to do, but only
  // for vector spaces is this based on something concrete...
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
}

namespace sequoia::meta
{
  using namespace maths;

  template<class T, class U>
  struct type_comparator<dual<T>, U> : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
  struct type_comparator<T, dual<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T, class U>
  struct type_comparator<dual<T>, dual<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
  {};

  template<class T>
  struct type_comparator<dual<T>, T> : std::false_type
  {};

  template<class T>
  struct type_comparator<T, dual<T>> : std::true_type
  {};
}

namespace sequoia::maths
{
  // TO DO: consider reordering to avoid forward declaration here?
  template<std::size_t D, std::floating_point T>
  struct euclidean_vector_space;

  template<std::size_t D, std::floating_point T>
  struct euclidean_half_space;
  
  //========= reduction of direct products ostensibly to a lower dimensional space ========= //

  namespace impl
  {
    template<class T, int I>
    struct type_counter
    {
    };
    
    template<class...>
    struct counter;

    template<class... Ts>
    using counter_t = counter<Ts...>::type;

    template<>
    struct counter<std::tuple<>>
    {
      using type = std::tuple<>;
    };

    template<class T>
    struct counter<T>
    {
      using type = std::tuple<type_counter<T, 1>>;
    };

    template<class T>
    struct counter<std::tuple<T>>
    {
      using type = std::tuple<type_counter<T, 1>>;
    };
    
    template<class T, class... Ts>
    struct counter<std::tuple<T, Ts...>>
    {
      using type = counter_t<std::tuple<Ts...>, counter_t<T>>;
    };

    template<class T, class... Us, int... Is>
    struct counter<std::tuple<T>, std::tuple<type_counter<Us, Is>...>>
    {
      using type = counter_t<T, std::tuple<type_counter<Us, Is>...>>;
    };

    template<class T, class... Ts, class... Us, int... Is>
    requires (sizeof...(Ts) > 0)
    struct counter<std::tuple<T, Ts...>, std::tuple<type_counter<Us, Is>...>>
    {
      using type = counter_t<std::tuple<Ts...>, counter_t<T, std::tuple<type_counter<Us, Is>...>>>;
    };
   
    template<class S, class T, int I, class... Ts, int... Is>
      requires (!is_tuple_v<S> && !is_dual_v<S> && !std::is_same_v<S, T>)
    struct counter<S, std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>>
    {
      using type = std::tuple<type_counter<S, 1>, type_counter<T, I>, type_counter<Ts, Is>...>;
    };

    template<class S, class T, int I, class... Ts, int... Is>
      requires (!std::is_same_v<S, T> && !is_dual_v<T>)
    struct counter<dual<S>, std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>>
    {
      using type = std::tuple<type_counter<dual<S>, 1>, type_counter<T, I>, type_counter<Ts, Is>...>;
    };

    template<class T, int I, class... Ts, int... Is>
    struct counter<T, std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>>
    {
      using type = std::tuple<type_counter<T, I+1>, type_counter<Ts, Is>...>;
    };

    template<class T, int I, class... Ts, int... Is>
    struct counter<dual<T>, std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>>
    {
      using type = std::tuple<type_counter<T, I-1>, type_counter<Ts, Is>...>;
    };

    // TO DO: this unpacking roundtrip is probably unnecessary;
    // could just work in terms of counter.
    template<class...>
    struct unpack;

    template<class... Ts>
    using unpack_t = unpack<Ts...>::type;

    template<class T, int I>
      requires (I > 0)
    struct unpack<type_counter<T, I>>
    {
      using type = unpack_t<type_counter<T, I - 1>, std::tuple<T>>;
    };

    template<class T, int I, class... Ts>
    struct unpack<type_counter<T, I>, std::tuple<Ts...>>
    {
      using type = unpack_t<type_counter<T, I - 1>, std::tuple<T, Ts...>>;
    };

    template<class T, class... Ts>
    struct unpack<type_counter<T, 0>, std::tuple<Ts...>>
    {
      using type = std::tuple<Ts...>;
    };
    

    template<class...>
    struct reduce;

    template<class... Ts>
    using reduce_t = reduce<Ts...>::type;

    template<vector_space T>
    struct reduce<std::tuple<type_counter<T, 0>>>
    {
      using type = std::tuple<euclidean_vector_space<1, typename T::vector_space_type::field_type>>;
    };

    template<affine_space T>
      requires (!vector_space<T>)
    struct reduce<std::tuple<type_counter<T, 0>>>
    {
      using type = std::tuple<euclidean_vector_space<1, typename T::affine_space_type::field_type>>;
    };

    template<convex_space T>
      requires (!affine_space<T> && !vector_space<T>)
    struct reduce<std::tuple<type_counter<T, 0>>>
    {
      // TO DO: need a way of propagating the correct underlying euclidean subspace
      using type = std::tuple<euclidean_vector_space<1, typename T::vector_space_type::field_type>>;
    };

    template<class T, class... Ts, int... Is>
    struct reduce<std::tuple<type_counter<T, 0>, type_counter<Ts, Is>...>>
    {
      using type = reduce_t<std::tuple<type_counter<Ts, Is>...>>;
    };

    template<class T, int I, class... Ts, int... Is>
      requires (I > 0)
    struct reduce<std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>>
    {
      using type = reduce_t<std::tuple<type_counter<Ts, Is>...>, unpack_t<type_counter<T, I>>>;
    };

    template<class T, class... Ts, int... Is, class... Us>
    struct reduce<std::tuple<type_counter<T, 0>, type_counter<Ts, Is>...>, std::tuple<Us...>>
    {
      using type = reduce_t<std::tuple<type_counter<Ts, Is>...>, std::tuple<Us...>>;
    };

    template<class T, int I, class... Ts, int... Is, class... Us>
      requires (I > 0)
    struct reduce<std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>, std::tuple<Us...>>
    {
      using type = reduce_t<std::tuple<type_counter<Ts, Is>...>, unpack_t<type_counter<T, I>, std::tuple<Us...>>>;
    };

    template<class... Us>
    struct reduce<std::tuple<>, std::tuple<Us...>>
    {
      using type = std::tuple<Us...>;
    };

    template<class>
    struct simplify;

    template<class T>
    using simplify_t = simplify<T>::type;

    template<class... Ts>
    struct simplify<std::tuple<Ts...>>
    {
      using type = reduce_t<counter_t<std::tuple<Ts...>>>;
    };

    template<convex_space... Ts>
    struct simplify<direct_product<std::tuple<Ts...>>>
    {
      using reduced_tuple_type = simplify_t<std::tuple<Ts...>>;
      using type = std::conditional_t<std::tuple_size_v<reduced_tuple_type> == 1,
                                      std::tuple_element_t<0, reduced_tuple_type>,
                                      direct_product<reduced_tuple_type>>;
    };
  }

  template<class T>
  struct reduction;

  template<class T>
  using reduction_t = reduction<T>::type;

  template<class T>
  struct is_reduction : std::false_type {};

  template<class T>
  struct is_reduction<reduction<T>> : std::true_type {};

  template<class T>
  inline constexpr bool is_reduction_v{is_reduction<T>::value};

  template<class T>
  struct to_reduction
  {
    using type = T;
  };

  template<class T>
  using to_reduction_t = to_reduction<T>::type;

  template<class... Ts>
  struct to_reduction<direct_product<std::tuple<Ts...>>>
  {
    using type = reduction<direct_product<std::tuple<Ts...>>>;
  };

  template<convex_space... Ts>
    requires (vector_space<Ts> ||  ...)
  struct reduction<direct_product<std::tuple<Ts...>>>
  {    
    using tuple_type        = std::tuple<Ts...>;
    using direct_product_t  = direct_product<std::tuple<Ts...>>;
    using set_type          = reduction<typename direct_product_t::set_type>;
    using field_type        = typename direct_product_t::field_type;
    using vector_space_type = reduction<direct_product<std::tuple<Ts...>>>;
    constexpr static std::size_t dimension{std::ranges::max({dimension_of<Ts>...})};
  };

  template<convex_space... Ts>
    requires (!vector_space<Ts> &&  ...)
  struct reduction<direct_product<std::tuple<Ts...>>>
  {
    using direct_product_t  = direct_product<std::tuple<Ts...>>;
    using set_type          = reduction<typename direct_product_t::set_type>;
    using vector_space_type = reduction_t<typename direct_product_t::vector_space_type>;
    using convex_space_type = reduction<direct_product<std::tuple<Ts...>>>;
  };

  template<convex_space T, convex_space U>
    requires (!is_reduction_v<T>) && (!is_reduction_v<U>)
  struct reduction<direct_product<T, U>>
  {    
    using type = to_reduction_t<impl::simplify_t<direct_product<meta::merge_t<std::tuple<T>, std::tuple<U>, meta::type_comparator>>>>;
  };

  template<convex_space T, convex_space... Us>
    requires (!is_reduction_v<T>)
  struct reduction<direct_product<T, reduction<direct_product<std::tuple<Us...>>>>>
  {
    using type = to_reduction_t<impl::simplify_t<direct_product<meta::merge_t<std::tuple<T>, std::tuple<Us...>, meta::type_comparator>>>>;
  };

  template<convex_space... Ts, convex_space U>
    requires (!is_reduction_v<U>)
  struct reduction<direct_product<reduction<direct_product<std::tuple<Ts...>>>, U>>
  {
    using type = to_reduction_t<impl::simplify_t<direct_product<meta::merge_t<std::tuple<Ts...>, std::tuple<U>, meta::type_comparator>>>>;
  };

  template<convex_space... Ts, convex_space... Us>
    requires (sizeof...(Ts) > 1) && (sizeof...(Us) > 1)
  struct reduction<direct_product<reduction<direct_product<std::tuple<Ts...>>>, reduction<direct_product<std::tuple<Us...>>>>>
  {
    using type = to_reduction_t<impl::simplify_t<direct_product<meta::merge_t<std::tuple<Ts...>, std::tuple<Us...>, meta::type_comparator>>>>;
  };

  template<vector_space... Ts>
  struct reduction<direct_product<Ts...>>
  {
    using type = reduction<direct_product<std::tuple<Ts...>>>;
  };
  
  //============================== coordinates_base definition  ==============================//

  template<
    convex_space ConvexSpace,
    basis_for<vector_space_type_of<ConvexSpace>> Basis,
    class Origin,
    validator_for<ConvexSpace> Validator,
    class DisplacementCoordinates=vector_coordinates<vector_space_type_of<ConvexSpace>, Basis>
  >
  class coordinates_base
  {
  public:
    using convex_space_type = ConvexSpace;   
    using basis_type        = Basis;
    using validator_type    = Validator;
    using origin_type       = Origin;
    using set_type          = typename ConvexSpace::set_type;
    using vector_space_type = vector_space_type_of<ConvexSpace>;
    using field_type        = space_field_type<ConvexSpace>;
    using value_type        = field_type;
    using displacement_coordinates_type = DisplacementCoordinates;

    constexpr static bool has_intrinsic_origin{std::is_same_v<Origin, intrinsic_origin>};
    constexpr static bool has_identity_validator{std::is_same_v<Validator, std::identity>};
    constexpr static std::size_t dimension{vector_space_type::dimension};
    constexpr static std::size_t D{dimension};

    constexpr coordinates_base() noexcept = default;

    constexpr explicit coordinates_base(std::span<const value_type, D> d) noexcept(has_identity_validator)
      : m_Values{m_Validator(to_array(d))}
    {}

    template<class... Ts>
      requires (sizeof...(Ts) == D) && (std::convertible_to<Ts, value_type> && ...)
    constexpr explicit(sizeof...(Ts) == 1) coordinates_base(Ts... ts) noexcept(has_identity_validator)
      : m_Values{m_Validator(std::array<value_type, D>{ts...})}
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
      requires has_intrinsic_origin
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
      requires std::is_base_of_v<coordinates_base, Derived>
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
  private:
    SEQUOIA_NO_UNIQUE_ADDRESS validator_type m_Validator;
    std::array<value_type, D> m_Values{};

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
        self.m_Values = self.m_Validator(tmp);
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
        self.m_Values = self.m_Validator(tmp);
      }
    }
  };

  template<convex_space ConvexSpace, basis_for<vector_space_type_of<ConvexSpace>> Basis, class Origin, validator_for<ConvexSpace> Validator>
  class coordinates : public coordinates_base<ConvexSpace, Basis, Origin, Validator>
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
