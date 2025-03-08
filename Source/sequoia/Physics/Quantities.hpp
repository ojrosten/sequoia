////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/Core/Meta/TypeAlgorithms.hpp"
#include "sequoia/Maths/Geometry/Spaces.hpp"

#include <functional>

namespace sequoia
{
  namespace physics
  {
    template<class Space>
    struct displacement_space;
  };
  
  namespace meta
  {
    template<class T, class U>
    struct type_comparator<maths::dual<T>, U> : std::bool_constant<type_name<T>() < type_name<U>()>
    {};

    template<class T, class U>
    struct type_comparator<T, maths::dual<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
    {};

    template<class T, class U>
    struct type_comparator<maths::dual<T>, maths::dual<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
    {};

    template<class T>
    struct type_comparator<maths::dual<T>, T> : std::false_type
    {};

    template<class T>
    struct type_comparator<T, maths::dual<T>> : std::true_type
    {};

    template<class T, class U>    
      requires (!maths::is_dual_v<U>)
    struct type_comparator<physics::displacement_space<T>, U> : std::bool_constant<type_name<T>() < type_name<U>()>
    {};

    template<class T, class U>
      requires (!maths::is_dual_v<T>)
    struct type_comparator<T, physics::displacement_space<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
    {};

    template<class T, class U>
    struct type_comparator<physics::displacement_space<T>, maths::dual<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
    {};

    template<class T, class U>
    struct type_comparator<maths::dual<T>, physics::displacement_space<U>> : std::bool_constant<type_name<T>() < type_name<U>()>
    {};

    template<class T, class U>
    struct type_comparator<maths::dual<physics::displacement_space<T>>, U>
      : std::bool_constant<type_name<T>() < type_name<U>()>
    {};

    template<class T, class U>
    struct type_comparator<T, maths::dual<physics::displacement_space<U>>>
      : std::bool_constant<type_name<T>() < type_name<U>()>
    {};

    template<class T, class U>
    struct type_comparator<physics::displacement_space<T>, physics::displacement_space<U>>
      : std::bool_constant<type_name<T>() < type_name<U>()>
    {};

    template<class T, class U>
    struct type_comparator<maths::dual<physics::displacement_space<T>>, maths::dual<physics::displacement_space<U>>>
      : std::bool_constant<type_name<T>() < type_name<U>()>
    {};

    template<class T>
    struct type_comparator<physics::displacement_space<T>, T> : std::false_type
    {};

    template<class T>
    struct type_comparator<maths::dual<physics::displacement_space<T>>, T> : std::false_type
    {};

    template<class T>
    struct type_comparator<T, physics::displacement_space<T>> : std::true_type
    {};
    
    template<class T>
    struct type_comparator<T, maths::dual<physics::displacement_space<T>>> : std::true_type
    {};

    template<class T>
    struct type_comparator<physics::displacement_space<T>, maths::dual<physics::displacement_space<T>>>
      : std::true_type
    {};
  }
  
  namespace physics
  {
    template<class T>
    concept quantity_unit = requires {
      typename T::validator_type;
    };
    
    template<class... Ts>
    struct reduced_validator;

    template<class... Ts>
    using reduced_validator_t = reduced_validator<Ts...>::type;

    template<class T>
    struct reduced_validator<T>
    {
      using type = T;
    };

    template<class T, class... Us>
    struct reduced_validator<T, Us...>
    {
      using type = reduced_validator_t<T, reduced_validator_t<Us...>>;
    };

    template<class T>
    struct composite_unit;

    struct no_unit_t
    {
      using validator_type = maths::half_space_validator;
    };

    inline constexpr no_unit_t no_unit{};

    // Ts are assumed to be ordered
    template<physics::quantity_unit... Ts>
    struct composite_unit<std::tuple<Ts...>>
    {
      using validator_type = reduced_validator_t<typename Ts::validator_type...>;
    };
  }

  namespace maths
  {
    template<physics::quantity_unit T>
    struct dual<T>
    {
      // TO DO: this doesn't hold for all validators!
      using validator_type = T::validator_type;
    };    
  }
}


namespace sequoia::physics
{
  using namespace maths;
  
  template<class T>
  struct reduction;

  namespace impl
  {
    template<class T, int I>
    struct type_counter {};
    
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

    /// Promote all T to displacement_space<T>
    template<class T, int I, class... Ts, int... Is>
    struct counter<displacement_space<T>, std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>>
    {
      using type = std::tuple<type_counter<displacement_space<T>, I+1>, type_counter<Ts, Is>...>;
    };

    /// Promote all dual<T> to dual<displacement_space<T>>
    template<class T, int I, class... Ts, int... Is>
    struct counter<dual<displacement_space<T>>, std::tuple<type_counter<dual<T>, I>, type_counter<Ts, Is>...>>
    {
      using type = std::tuple<type_counter<dual<displacement_space<T>>, I+1>, type_counter<Ts, Is>...>;
    };

    /// Promote all T to displacement_space<T>
    template<class T, int I, class... Ts, int... Is>
    struct counter<dual<displacement_space<T>>, std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>>
    {
      using type = std::tuple<type_counter<displacement_space<T>, I-1>, type_counter<Ts, Is>...>;
    };

    /// Promote all dual<T> to dual<displacement_space<T>>
    template<class T, int I, class... Ts, int... Is>
    struct counter<displacement_space<T>, std::tuple<type_counter<dual<T>, I>, type_counter<Ts, Is>...>>
    {
      using type = std::tuple<type_counter<dual<displacement_space<T>>, I-1>, type_counter<Ts, Is>...>;
    };

    template<class T, int I, class... Ts, int... Is>
    struct counter<dual<T>, std::tuple<type_counter<displacement_space<T>, I>, type_counter<Ts, Is>...>>
    {
      using type = std::tuple<type_counter<displacement_space<T>, I-1>, type_counter<Ts, Is>...>;
    };

    template<class>
    struct to_dual;

    template<class T>
    using to_dual_t = to_dual<T>::type;

    template<class C>
    struct to_dual {
      using type = dual<C>;
    };

    template<class C>
    struct to_dual<dual<C>> {
      using type = C;
    };

    template<convex_space... Ts>
    struct to_dual<reduction<direct_product<std::tuple<Ts...>>>>
    {
      using type = reduction<direct_product<std::tuple<to_dual_t<Ts>...>>>;
    };

    template<quantity_unit... Ts>
    struct to_dual<composite_unit<std::tuple<Ts...>>>
    {
      using type = composite_unit<std::tuple<to_dual_t<Ts>...>>;
    };

    //========= reduction of direct products to a lower dimensional space ========= //

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

    template<class T, int I>
      requires (I < 0)
    struct unpack<type_counter<T, I>>
    {
      using type = unpack_t<type_counter<T, I + 1>, std::tuple<dual<T>>>;
    };

    template<class T, int I, class... Ts>
      requires (I > 0)
    struct unpack<type_counter<T, I>, std::tuple<Ts...>>
    {
      using type = unpack_t<type_counter<T, I - 1>, std::tuple<T, Ts...>>;
    };

    template<class T, int I, class... Ts>
      requires (I < 0)
    struct unpack<type_counter<T, I>, std::tuple<Ts...>>
    {
      using type = unpack_t<type_counter<T, I + 1>, std::tuple<dual<T>, Ts...>>;
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

    template<convex_space T>
      requires (!affine_space<T> && !vector_space<T>)
    struct reduce<std::tuple<type_counter<T, 0>>>
    {
      using type = std::tuple<euclidean_half_space<1, typename T::vector_space_type::field_type>>;
    };

    template<class T, class... Ts, int... Is>
    struct reduce<std::tuple<type_counter<T, 0>, type_counter<Ts, Is>...>>
    {
      using type = reduce_t<std::tuple<type_counter<Ts, Is>...>>;
    };

    template<class T, int I>
      requires (I != 0)
    struct reduce<std::tuple<type_counter<T, I>>>
    {
      using type = unpack_t<type_counter<T, I>>;
    };
 
    template<class T, int I, class... Ts, int... Is>
      requires (I != 0)
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
      requires (I != 0)
    struct reduce<std::tuple<type_counter<T, I>, type_counter<Ts, Is>...>, std::tuple<Us...>>
    {
      using type = reduce_t<std::tuple<type_counter<Ts, Is>...>, unpack_t<type_counter<T, I>, std::tuple<Us...>>>;
    };

    template<class... Us>
    struct reduce<std::tuple<>, std::tuple<Us...>>
    {
      using type = std::tuple<Us...>;
    };

    template<physics::quantity_unit T>
    struct reduce<std::tuple<type_counter<T, 0>>>
    {
      using type = std::tuple<physics::no_unit_t>;
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
     
    template<physics::quantity_unit... Ts>
    struct simplify<physics::composite_unit<std::tuple<Ts...>>>
    {
      using reduced_tuple_type = simplify_t<std::tuple<Ts...>>;
      using type = std::conditional_t<std::tuple_size_v<reduced_tuple_type> == 1,
                                      std::tuple_element_t<0, reduced_tuple_type>,
                                      physics::composite_unit<reduced_tuple_type>>;
    };
  }

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
    
  template<physics::quantity_unit T, physics::quantity_unit U>
  struct reduction<std::tuple<T, U>>
  {
    using type = impl::simplify_t<physics::composite_unit<meta::merge_t<std::tuple<T>, std::tuple<U>, meta::type_comparator>>>;
  };

  template<physics::quantity_unit... Ts, physics::quantity_unit U>
  struct reduction<std::tuple<physics::composite_unit<std::tuple<Ts...>>, U>>
  {
    using type = impl::simplify_t<physics::composite_unit<meta::merge_t<std::tuple<Ts...>, std::tuple<U>, meta::type_comparator>>>;
  };

  template<physics::quantity_unit T, physics::quantity_unit... Us>
  struct reduction<std::tuple<T, physics::composite_unit<std::tuple<Us...>>>>
  {
    using type = impl::simplify_t<physics::composite_unit<meta::merge_t<std::tuple<Us...>, std::tuple<T>, meta::type_comparator>>>;
  };

  template<physics::quantity_unit... Ts, physics::quantity_unit... Us>
  struct reduction<std::tuple<physics::composite_unit<std::tuple<Ts...>>, physics::composite_unit<std::tuple<Us...>>>>
  {
    using type = impl::simplify_t<physics::composite_unit<meta::merge_t<std::tuple<Ts...>, std::tuple<Us...>, meta::type_comparator>>>;
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

  struct coordinate_basis_type{};

  template<vector_space VectorSpace, class Unit, std::floating_point T>
  struct quantity_displacement_basis
  {
    using vector_space_type      = VectorSpace;
    using unit_type              = Unit;
    using basis_orientation_type = coordinate_basis_type;
  };

  template<class T>
  struct reduced_validator<T, std::identity>
  {
    using type = std::identity;
  };

  template<class T>
    requires (!std::is_same_v<T, std::identity>)
  struct reduced_validator<std::identity, T>
  {
    using type = std::identity;
  };

  template<>
  struct reduced_validator<half_space_validator, half_space_validator>
  {
    using type = half_space_validator;
  };

  template<convex_space QuantitySpace, quantity_unit Unit>
  using to_displacement_basis_t
    = quantity_displacement_basis<vector_space_type_of<QuantitySpace>, Unit, space_field_type<QuantitySpace>>;

  template<convex_space QuantitySpace, quantity_unit Unit, class Validator>
  class quantity;

  template<quantity_unit Unit>
  struct unit_defined_origin{};
  
  template<convex_space QuantitySpace, quantity_unit Unit, class Validator>
  using to_coordinates_base_type
    = coordinates_base<
        QuantitySpace,
        to_displacement_basis_t<QuantitySpace, Unit>,
        // TO DO: figure out if there's a better way of expressing this
        std::conditional_t<vector_space<QuantitySpace> || defines_half_space_v<typename Unit::validator_type>,
                           intrinsic_origin,
                           unit_defined_origin<Unit>>,
        Validator,
        quantity<vector_space_type_of<QuantitySpace>, Unit, std::identity>>;
  
  template<class T, class U>
  struct quantity_product;

  template<class T, class U>
  using quantity_product_t = quantity_product<T, U>::type;
  
  template<
    convex_space LHSQuantitySpace, quantity_unit LHSUnit, class LHSValidator,
    convex_space RHSQuantitySpace, quantity_unit RHSUnit, class RHSValidator
  >
  struct quantity_product<quantity<LHSQuantitySpace, LHSUnit, LHSValidator>, quantity<RHSQuantitySpace, RHSUnit, RHSValidator>>
  {
    using type = quantity<
      reduction_t<direct_product<LHSQuantitySpace, RHSQuantitySpace>>,
      reduction_t<std::tuple<LHSUnit, RHSUnit>>,
      reduced_validator_t<LHSValidator, RHSValidator>>;
  };

  template<
    convex_space LHSQuantitySpace, quantity_unit LHSUnit, class LHSValidator,
    convex_space RHSQuantitySpace, quantity_unit RHSUnit, class RHSValidator
  >
    requires    std::is_same_v<euclidean_vector_space<1, space_field_type<LHSQuantitySpace>>, LHSQuantitySpace>
             || std::is_same_v<euclidean_half_space<1, space_field_type<LHSQuantitySpace>>, LHSQuantitySpace>
  struct quantity_product<quantity<LHSQuantitySpace, LHSUnit, LHSValidator>, quantity<RHSQuantitySpace, RHSUnit, RHSValidator>>
  {
    using type = quantity<RHSQuantitySpace, RHSUnit, reduced_validator_t<LHSValidator, RHSValidator>>;
  };

  template<
    convex_space LHSQuantitySpace, quantity_unit LHSUnit, class LHSValidator,
    convex_space RHSQuantitySpace, quantity_unit RHSUnit, class RHSValidator
  >
    requires    std::is_same_v<euclidean_vector_space<1, space_field_type<RHSQuantitySpace>>, RHSQuantitySpace>
             || std::is_same_v<euclidean_half_space<1, space_field_type<RHSQuantitySpace>>, RHSQuantitySpace>
  struct quantity_product<quantity<LHSQuantitySpace, LHSUnit, LHSValidator>, quantity<RHSQuantitySpace, RHSUnit, RHSValidator>>
  {
    using type = quantity<LHSQuantitySpace, LHSUnit, reduced_validator_t<LHSValidator, RHSValidator>>;
  };
  
  template<convex_space QuantitySpace, quantity_unit Unit, class Validator=typename Unit::validator_type>
  class quantity : public to_coordinates_base_type<QuantitySpace, Unit, Validator>
  {
  public:
    using coordinates_type           = to_coordinates_base_type<QuantitySpace, Unit, Validator>;
    using quantity_space_type        = QuantitySpace;
    using units_type                 = Unit;
    using displacement_space_type    = vector_space_type_of<QuantitySpace>;
    using intrinsic_validator_type   = Unit::validator_type;
    using validator_type             = Validator;
    using field_type                 = space_field_type<QuantitySpace>;
    using value_type                 = field_type;
    using displacement_quantity_type = coordinates_type::displacement_coordinates_type;

    constexpr static std::size_t dimension{displacement_space_type::dimension};
    constexpr static std::size_t D{dimension};

    constexpr static bool is_intrinsically_absolute{(D == 1) && defines_half_space_v<intrinsic_validator_type>};
    constexpr static bool is_effectively_absolute{is_intrinsically_absolute && std::is_same_v<Validator, intrinsic_validator_type>};
    constexpr static bool has_identity_validator{coordinates_type::has_identity_validator};

    template<convex_space RHSQuantitySpace, class RHSUnit, class RHSValidator>
    constexpr static bool is_composable_with{
         (is_intrinsically_absolute || vector_space<QuantitySpace>)
      && (quantity<RHSQuantitySpace, RHSUnit, RHSValidator>::is_intrinsically_absolute || vector_space<RHSQuantitySpace>)
    };

    template<convex_space RHSQuantitySpace, class RHSUnit, class RHSValidator>
    constexpr static bool is_multipicable_with{
            is_composable_with<RHSQuantitySpace, RHSUnit, RHSValidator>
         && ((D == 1) || (quantity<RHSQuantitySpace, RHSUnit, RHSValidator>::D == 1))
    };

    template<convex_space RHSQuantitySpace, class RHSUnit, class RHSValidator>
    constexpr static bool is_divisible_with{
      is_composable_with<RHSQuantitySpace, RHSUnit, RHSValidator> && (quantity<RHSQuantitySpace, RHSUnit, RHSValidator>::D == 1)
    };

    constexpr quantity() = default;

    constexpr quantity(value_type val, units_type) requires (D == 1)
      : coordinates_type{val}
    {}

    constexpr quantity(std::span<const value_type, D> val, units_type)
      : coordinates_type{val}
    {}

    constexpr quantity operator-() const requires is_effectively_absolute = delete;

    [[nodiscard]]
    constexpr quantity operator+() const
    {
      return quantity{this->values(), units_type{}};
    }

    [[nodiscard]]
    constexpr quantity operator-() const noexcept(has_identity_validator)
      requires (!is_effectively_absolute)
    {
      return quantity{to_array(this->values(), [](value_type t) { return -t; }), units_type{}};
    }

    [[nodiscard]]
    friend constexpr displacement_quantity_type operator-(const quantity& lhs, const quantity& rhs) noexcept(has_identity_validator)
    {
      return[&] <std::size_t... Is>(std::index_sequence<Is...>) {
        return displacement_quantity_type{(lhs.values()[Is] - rhs.values()[Is])..., units_type{}};
      }(std::make_index_sequence<D>{});
    }

    template<convex_space RHSQuantitySpace, quantity_unit RHSUnit, class RHSValidator>
      requires is_multipicable_with<RHSQuantitySpace, RHSUnit, RHSValidator>
    [[nodiscard]]
    friend constexpr auto operator*(const quantity& lhs, const quantity<RHSQuantitySpace, RHSUnit, RHSValidator>& rhs)
    {
      using quantity_t = quantity_product_t<quantity, quantity<RHSQuantitySpace, RHSUnit, RHSValidator>>;
      using derived_units_type = quantity_t::units_type;
      return quantity_t{lhs.value() * rhs.value(), derived_units_type{}};
    }
    
    // TO DO: ensure e.g. T / Delta T explicitly reduces to a 1D Euclidean vector space
    template<convex_space RHSQuantitySpace, class RHSUnit, class RHSValidator>
       requires is_divisible_with<RHSQuantitySpace, RHSUnit, RHSValidator>
    [[nodiscard]]
    friend constexpr auto operator/(const quantity& lhs, const quantity<RHSQuantitySpace, RHSUnit, RHSValidator>& rhs)
    {
      using impl::to_dual_t;
      using quantity_t = quantity_product_t<quantity, quantity<to_dual_t<RHSQuantitySpace>, to_dual_t<RHSUnit>, RHSValidator>>;
      using derived_units_type = quantity_t::units_type;
      return quantity_t{lhs.value() / rhs.value(), derived_units_type{}};
    }

    [[nodiscard]] friend constexpr auto operator/(value_type value, const quantity& rhs)
      requires ((D == 1) && (is_intrinsically_absolute || vector_space<QuantitySpace>))
    {
      using impl::to_dual_t;
      using quantity_t = quantity<to_dual_t<QuantitySpace>, to_dual_t<Unit>, std::identity>;
      using derived_units_type = quantity_t::units_type;
      return quantity_t{value / rhs.value(), derived_units_type{}};
    }    
  };

  namespace classical_quantity_sets
  {
    struct masses
    {
      using topological_space_type = std::true_type;
    };

    struct lengths
    {
      using topological_space_type = std::true_type;
    };

    struct times
    {
      using topological_space_type = std::true_type;
    };

    struct temperatures
    {
      using topological_space_type = std::true_type;
    };

    struct electrical_charges
    {
      using topological_space_type = std::true_type;
    };

    template<class QuantitySet>
    struct differences
    {
      using quantity_set_type = QuantitySet;
    };
  }

  template<class Space>
  struct displacement_space
  {
    using set_type          = classical_quantity_sets::differences<typename Space::set_type>;
    using field_type        = Space::representation_type;
    using vector_space_type = displacement_space;
    constexpr static std::size_t dimension{1};
  };

  template<class QuantitySet, std::floating_point Rep, class Derived>
  struct quantity_convex_space
  {
    using set_type            = QuantitySet;
    using representation_type = Rep;
    using vector_space_type   = displacement_space<Derived>;
    using convex_space_type   = Derived;
  };

  template<class QuantitySet, std::floating_point Rep, class Derived>
  struct quantity_affine_space
  {
    using set_type            = QuantitySet;
    using representation_type = Rep;
    using vector_space_type   = displacement_space<Derived>;
    using affine_space_type   = Derived;
  };

  template<class QuantitySet, std::floating_point Rep, class Derived>
  struct quantity_vector_space
  {
    using set_type            = QuantitySet;
    using representation_type = Rep;
    using field_type          = Rep;
    using vector_space_type   = Derived;

    constexpr static std::size_t dimension{1};
  };

  template<std::floating_point Rep>
  struct mass_space : quantity_convex_space<classical_quantity_sets::masses, Rep, mass_space<Rep>> {};

  template<std::floating_point Rep>
  struct length_space : quantity_convex_space<classical_quantity_sets::lengths, Rep, length_space<Rep>> {};

  template<std::floating_point Rep>
  struct time_space : quantity_affine_space<classical_quantity_sets::times, Rep, time_space<Rep>> {};

  template<std::floating_point Rep>
  struct temperature_space : quantity_convex_space<classical_quantity_sets::temperatures, Rep, temperature_space<Rep>> {};

  template<std::floating_point Rep>
  struct electrical_charge_space : quantity_vector_space<classical_quantity_sets::electrical_charges, Rep, electrical_charge_space<Rep>> {};

  namespace units
  {
    struct kilogram_t
    {
      using validator_type = half_space_validator;
    };

    struct metre_t
    {
      using validator_type = half_space_validator;
    };

    struct second_t
    {
      using validator_type = std::identity;
    };

    struct kelvin_t
    {
      using validator_type = half_space_validator;
    };

    struct coulomb_t
    {
      using validator_type = std::identity;
    };

    inline constexpr kilogram_t kilogram{};
    inline constexpr metre_t    metre{};
    inline constexpr second_t   second{};
    inline constexpr kelvin_t   kelvin{};
    inline constexpr coulomb_t  coulomb{};

    struct celsius_t
    {
      struct validator
      {
        template<std::floating_point T>
        [[nodiscard]]
        constexpr T operator()(const T val) const
        {
          if(val < T(-273.15)) throw std::domain_error{std::format("Value {} less than -273.15", val)};

          return val;
        }

        // TO DO: remove the need for this
        template<std::floating_point T>
        [[nodiscard]]
        constexpr std::array<T, 1> operator()(std::array<T, 1> val) const
        {
          return {operator()(val.front())};
        }
      };

      using validator_type = validator;
    };

    inline constexpr celsius_t celsius{};
  }

  namespace si
  {
    template<std::floating_point T>
    using mass = quantity<mass_space<T>, units::kilogram_t>;

    template<std::floating_point T>
    using length = quantity<length_space<T>, units::metre_t>;

    template<std::floating_point T>
    using time = quantity<time_space<T>, units::second_t>;

    template<std::floating_point T>
    using temperature = quantity<temperature_space<T>, units::kelvin_t>;

    template<std::floating_point T>
    using electrical_charge = quantity<electrical_charge_space<T>, units::coulomb_t>;
  }
}
