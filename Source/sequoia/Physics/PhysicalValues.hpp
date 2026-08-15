////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/Physics/PhysicalValuesDetails.hpp"

#include <numbers>

namespace sequoia::physics
{
  template<free_module M, physical_unit U>
  struct unit_defined_right_handed_basis
  {
    using is_basis         = std::true_type;
    using free_module_type = M;
    using units_type       = U;
    using isomorphism_type = units_type;
  };

  struct no_unit_t : identity_isomorphism
  {
    using is_unit = std::true_type; // TO DO: naming makes this peverse!
  };
  
  inline constexpr no_unit_t no_unit{};

  /// @class Primary class template for the reduction of direct products to a lower dimensional space
  template<class T>
  struct reduction;

  template<class T>
  using reduction_t = reduction<T>::type;
}

namespace sequoia::maths
{
  template<physics::physical_unit T>
  struct dual<T>
  {
    using is_unit = std::true_type;
  };

  template<free_module M, physics::physical_unit U>
  struct dual_of<physics::unit_defined_right_handed_basis<M, U>>
  {
    using type = physics::unit_defined_right_handed_basis<dual_of_t<M>, dual_of_t<U>>;
  };

  template<free_module M, physics::physical_unit U>
  struct dual_of<physics::unit_defined_right_handed_basis<dual<M>, dual<U>>>
  {
    using type = physics::unit_defined_right_handed_basis<M, U>;
  };

  template<>
  struct dual_of<physics::no_unit_t>
  {
    using type = physics::no_unit_t;
  };

  template<free_module M1, physics::physical_unit U1, free_module M2, physics::physical_unit U2>
  struct consistent_bases<physics::unit_defined_right_handed_basis<M1, U1>, physics::unit_defined_right_handed_basis<M2, U2>> : std::true_type
  {
    template<free_module M, physics::physical_unit U>
    using rebind_type = physics::unit_defined_right_handed_basis<M, U>;
  };

  template<convex_space... Ts>
  struct to_base_space<physics::composite_space<Ts...>>
  {
    using sorted_tensor_product_t = meta::stable_sort_t<tensor_product<to_base_space_t<Ts>...>,  meta::type_comparator>;
    using type = physics::impl::to_composite_space_t<physics::reduction_t<physics::impl::reduce_t<physics::impl::count_and_combine_t<sorted_tensor_product_t>>>>;
  };
}

namespace sequoia::physics
{
  using namespace maths;

  template<physical_unit... Ts>
  struct composite_unit
  {
    using is_unit = std::true_type;
  };
  
  template<class... Ts>
  struct composite_space;

  template<convex_space... Ts>
    requires (free_module<Ts> ||  ...)
  struct composite_space<Ts...>
  {    
    using tensor_product_t      = tensor_product<Ts...>;
    using set_type              = reduction<typename tensor_product_t::set_type>;
    using commutative_ring_type = commutative_ring_type_of_t<tensor_product_t>;
    using is_free_module        = std::true_type;
    using arena_type            = arena_type_of_t<tensor_product<Ts...>>;
    constexpr static std::size_t dimension{std::ranges::max({dimension_of<Ts>...})};
  };

  template<convex_space... Ts>
    requires (!affine_space<Ts> && ...)
  struct composite_space<Ts...>
  {
    using tensor_product_t     = tensor_product<Ts...>;
    using set_type             = reduction<typename tensor_product_t::set_type>;
    using free_module_type     = composite_space<free_module_type_of_t<Ts>...>;
    using is_convex_space      = std::true_type;
    using arena_type           = arena_type_of_t<tensor_product<Ts...>>;
    using distinguished_origin = std::bool_constant<(has_distinguished_origin_v<Ts> && ...)>;
    using non_negative_orthant = std::bool_constant<(is_non_negative_orthant_v<Ts> && ...)>;
  };

  // Units & Spaces
  template<class... Us>
    requires (physical_unit<Us> && ...) || (convex_space<Us> && ...)
  struct reduction<tensor_product<Us...>>
  {
    using type = impl::simplify_t<tensor_product<Us...>>;
  };
 
  template<class... Ts, class U, template<class...> class TT>
    requires (std::same_as<TT<Ts...>, composite_unit <Ts...>> && physical_unit<U>)
          || (std::same_as<TT<Ts...>, composite_space<Ts...>> &&  convex_space<U>)
  struct reduction<tensor_product<TT<Ts...>, U>>
  {
    using type = impl::simplify_t<tensor_product<Ts...>, tensor_product<U>>;
  };

  template<class T, class... Us, template<class...> class TT>
    requires (std::same_as<TT<Us...>, composite_unit <Us...>> && physical_unit<T>)
          || (std::same_as<TT<Us...>, composite_space<Us...>> &&  convex_space<T>)
  struct reduction<tensor_product<T, TT<Us...>>>
  {
    using type = impl::simplify_t<tensor_product<T>, tensor_product<Us...>>;
  };

  template<class... Ts, class... Us, template<class...> class TT>
    requires (std::same_as<TT<Ts...>, composite_unit <Ts...>> && std::same_as<TT<Us...>, composite_unit <Us...>>)
          || (std::same_as<TT<Ts...>, composite_space<Ts...>> && std::same_as<TT<Us...>, composite_space<Us...>>)
  struct reduction<tensor_product<TT<Ts...>, TT<Us...>>>
  {
    using type = impl::simplify_t<tensor_product<Ts...>, tensor_product<Us...>>;
  };

  // Bounds TO DO: this is actually Reps
  template<weak_commutative_ring LHRingRep, auto LHBounds, weak_commutative_ring RHRingRep, auto RHBounds>
    requires bounds_value<LHBounds> && bounds_value<RHBounds>
  struct reduction<tensor_product<canonical_representation<LHRingRep, LHBounds>, canonical_representation<RHRingRep, RHBounds>>>
  {
    using value_type = std::common_type_t<LHRingRep, RHRingRep>; // TO DO: rethink this
    using type = canonical_representation<value_type, LHBounds * RHBounds>;
  };

  // Representations - TO DO looks superflous
  template<representation R, representation S>
  struct reduction<tensor_product<R, S>>
  {
    using value_type = std::common_type_t<typename R::value_type, typename S::value_type>; // TO DO: rethink this
    using type = canonical_representation<value_type, R::bounds_v * S::bounds_v>;
  };

  template<convex_space ValueSpace>
  inline constexpr bool permissible_value_space_v{
    (!is_dual_v<ValueSpace>) || (has_distinguished_origin_v<ValueSpace> && (dimension_of<ValueSpace> == 1))
  };

  template<
    convex_space ValueSpace,
    physical_unit Unit,
    basis_for<free_module_type_of_t<ValueSpace>> Basis,    
    representation_for<ValueSpace> Representation,
    class Origin,
    validator_for<ValueSpace, Representation> Validator
  >
    requires permissible_value_space_v<ValueSpace>
  class physical_value;
  
  struct unit_defined_origin{};

  struct implicit_affine_origin {};

  struct distinguished_origin {};

  template<convex_space ValueSpace>
  struct to_origin_type;
  
  template<convex_space ValueSpace>
  using to_origin_type_t = to_origin_type<ValueSpace>::type;

  template<convex_space ValueSpace>
    requires (!has_distinguished_origin_v<ValueSpace>) && (!affine_space<ValueSpace>)
  struct to_origin_type<ValueSpace>
  {
    using type = unit_defined_origin;
  };

  template<convex_space ValueSpace>
    requires has_distinguished_origin_v<ValueSpace>
  struct to_origin_type<ValueSpace>
  {
    using type = distinguished_origin;
  };

  template<convex_space ValueSpace>
    requires (!has_distinguished_origin_v<ValueSpace> && affine_space<ValueSpace>)
  struct to_origin_type<ValueSpace>
  {
    using type = implicit_affine_origin;
  };
  
  template<
    convex_space ValueSpace,
    physical_unit Unit,
    basis_for<free_module_type_of_t<ValueSpace>> Basis,
    representation_for<ValueSpace> Representation,
    class Validator
  >
  using to_coordinates_base_type
    = coordinates_base<
        ValueSpace,
        Basis,    
        Representation,
        Validator,
        physical_value<
          free_module_type_of_t<ValueSpace>,
          Unit,
          Basis,
          typename Representation::free_module_representation,
          distinguished_origin,
          Validator
        >
      >;

  template<convex_space T, convex_space U>
  struct to_displacement_space;

  template<convex_space T, convex_space U>
  using to_displacement_space_t = to_displacement_space<T, U>::type;

  template<convex_space T>
  struct to_displacement_space<T, T>
  {
    using type = free_module_type_of_t<T>;
  };

  template<convex_space T, convex_space U>
    requires (!std::is_same_v<T, U>) && have_compatible_base_spaces_v<T, U>
  struct to_displacement_space<T, U>
  {
    using type = free_module_type_of_t<std::common_type_t<typename T::base_space, typename U::base_space>>;
  };

  template<class T, class U>
  struct physical_value_product;

  template<class T, class U>
  using physical_value_product_t = physical_value_product<T, U>::type;

  template<physical_unit LHS, physical_unit RHS>
  constexpr auto operator*(LHS, RHS) noexcept
  {
    return impl::to_composite_space_t<reduction_t<tensor_product<LHS, RHS>>>{};
  }

  template<physical_unit LHS>
  constexpr auto operator*(LHS lhs, no_unit_t) noexcept
  {
    return lhs;
  };

  template<physical_unit RHS>
    requires (!std::same_as<RHS, no_unit_t>)
  constexpr auto operator*(no_unit_t, RHS rhs) noexcept
  {
    return rhs;
  };

  template<physical_unit LHS, physical_unit RHS>
  constexpr auto operator/(LHS, RHS) noexcept
  {
    return impl::to_composite_space_t<reduction_t<tensor_product<LHS, dual_of_t<RHS>>>>{};
  }

  template<physical_unit LHS>
  constexpr auto operator/(LHS lhs, no_unit_t) noexcept
  {
    return lhs;
  };

  template<physical_unit RHS>
    requires (!std::same_as<RHS, no_unit_t>)
  constexpr auto operator/(no_unit_t, RHS) noexcept
  {
    return dual_of_t<RHS>{};
  };
  
  template<
    convex_space LHSValueSpace, physical_unit LHSUnit, basis_for<free_module_type_of_t<LHSValueSpace>> LHSBasis, representation_for<LHSValueSpace> LHSRepresentation, class Validator,
    convex_space RHSValueSpace, physical_unit RHSUnit, basis_for<free_module_type_of_t<RHSValueSpace>> RHSBasis, representation_for<RHSValueSpace> RHSRepresentation
  >
    requires consistent_bases_v<LHSBasis, RHSBasis>
          && has_distinguished_origin_v<LHSValueSpace>
          && has_distinguished_origin_v<RHSValueSpace>
          && validator_for<Validator, LHSValueSpace,  LHSRepresentation>
          && validator_for<Validator, RHSValueSpace,  RHSRepresentation>
  struct physical_value_product<physical_value<LHSValueSpace, LHSUnit, LHSBasis, LHSRepresentation, distinguished_origin, Validator>,
                                physical_value<RHSValueSpace, RHSUnit, RHSBasis, RHSRepresentation, distinguished_origin, Validator>>
  {
    using value_space_type    = impl::to_composite_space_t<reduction_t<tensor_product<LHSValueSpace, RHSValueSpace>>>;
    using units_type          = impl::to_composite_space_t<reduction_t<tensor_product<LHSUnit, RHSUnit>>>;
    using representation_type = reduction_t<tensor_product<LHSRepresentation, RHSRepresentation>>;
    using type
      = physical_value<
          value_space_type,
          units_type,
          typename consistent_bases<LHSBasis, RHSBasis>::template rebind_type<free_module_type_of_t<value_space_type>, units_type>,      
          representation_type,
          distinguished_origin,
          Validator
        >;
  };

  template<class From, class To>
  inline constexpr bool has_quantity_conversion_v{
    has_coordinate_transformation_v<From, To>
  };

  template<convex_space C, physical_unit FromUnit, physical_unit ToUnit>
  struct conversion_space
  {
    using type = C;
  };

  template<convex_space C, physical_unit FromUnit, physical_unit ToUnit>
  using conversion_space_t = conversion_space<C, FromUnit, ToUnit>::type;

  namespace impl
  {

    template<class Rep, class...>
    struct is_valid_physical_value_pack : std::false_type {};

    template<class Rep, class... Args, std::size_t... Is>
      requires (sizeof...(Args) == sizeof...(Is) + 1)
            && physical_unit<std::tuple_element_t<sizeof...(Is), std::tuple<Args...>>>
            && (std::convertible_to<std::tuple_element_t<Is, std::tuple<Args...>>, Rep> && ...)
    struct is_valid_physical_value_pack<Rep, std::tuple<Args...>, std::index_sequence<Is...>> : std::true_type
    {
    };
  }
  
  template<class Rep, class... Args>
    requires (sizeof...(Args) > 1)
  struct is_valid_physical_value_pack
    : impl::is_valid_physical_value_pack<Rep, std::tuple<Args...>, std::make_index_sequence<sizeof...(Args) - 1>>
  {};

  template<class Rep, class... Args>
  inline constexpr bool is_valid_physical_value_pack_v{is_valid_physical_value_pack<Rep, Args...>::value};

  template<physical_unit U>
  struct root_transform;

  template<physical_unit U>
  using root_transform_t = root_transform<U>::transform_type;

  template<class...>
  struct coordinate_transform;

  template<auto Displacement>
    requires arithmetic<std::remove_const_t<decltype(Displacement)>>
  struct translation
  {
    using displacement_type = std::remove_const_t<decltype(Displacement)>;
    constexpr static auto displacement{Displacement};
  };

  template<arithmetic T, physical_unit Unit, class Ratio, auto Displacement>
  [[nodiscard]]
  constexpr coordinate_bounds<T> transform_bounds(coordinate_bounds<T> b, const coordinate_transform<Unit, dilatation<Ratio>, translation<Displacement>>&)
  {
    auto transform{
      [](T val) -> T {
        const auto transformedVal{saturating_add(saturating_mul(val / Ratio::den, Ratio::num), Displacement)};
        using transformed_t = decltype(transformedVal);
        if constexpr(std::is_signed_v<transformed_t> && std::is_unsigned_v<T>)
        {
          if(transformedVal < transformed_t{})
            throw std::runtime_error{std::format("Illegal coordinate transform: try to set an unsigned integral type to {}", transformedVal)};
        }

        return static_cast<T>(transformedVal);
      }
    };

    return {transform(b.lower), transform(b.upper)};
  }

  template<convex_space ValueSpace, weak_commutative_ring ValueType, physical_unit Unit>
  struct default_representation;

  template<convex_space ValueSpace, weak_commutative_ring ValueType, physical_unit Unit>
  using default_representation_t = default_representation<ValueSpace, ValueType, Unit>::type;

  template<convex_space ValueSpace, weak_commutative_ring ValueType, physical_unit Unit>
    requires free_module<ValueSpace> || affine_space<ValueSpace>
  struct default_representation<ValueSpace, ValueType, Unit>
  {
    using type = canonical_representation<ValueType, no_bounds<to_bounds_value_type_t<ValueType>>>;
  };
  
  template<convex_space ValueSpace, weak_commutative_ring ValueType, physical_unit Unit>
    requires (!free_module<ValueSpace> && !affine_space<ValueSpace>)
  struct default_representation<ValueSpace, ValueType, Unit>
  {
    using transform_t = root_transform_t<Unit>;
    constexpr static auto bounds_v{transform_bounds(half_line_bounds<to_bounds_value_type_t<ValueType>>, transform_t{})};
    using type = canonical_representation<ValueType, bounds_v>;
  };

  template<
    convex_space ValueSpace,
    physical_unit Unit,
    basis_for<free_module_type_of_t<ValueSpace>> Basis,// = unit_defined_right_handed_basis<free_module_type_of_t<ValueSpace>, Unit>,    
    representation_for<ValueSpace> Representation,//       = default_representation_t<ValueSpace, ValueType, Unit>,
    class Origin                                        = to_origin_type_t<ValueSpace>,
    validator_for<ValueSpace, Representation> Validator = throwing_validator
  >
    requires permissible_value_space_v<ValueSpace>
  class physical_value
    : public to_coordinates_base_type<ValueSpace, Unit, Basis, Representation, Validator>
  {
  public:
    using coordinates_type         = to_coordinates_base_type<ValueSpace, Unit, Basis, Representation, Validator>;
    using space_type               = ValueSpace;
    using units_type               = Unit;
    using basis_type               = Basis;
    using origin_type              = Origin;
    using displacement_space_type  = free_module_type_of_t<ValueSpace>;
    using representation_type      = Representation;
    using validator_type           = Validator;    
    using value_type               = coordinates_type::value_type;
    using displacement_value_type  = coordinates_type::displacement_value_type;
    using displacement_type        = coordinates_type::displacement_coordinates_type;

    constexpr static std::size_t dimension{displacement_space_type::dimension};
    constexpr static std::size_t D{dimension};

    constexpr static bool has_identity_validator{coordinates_type::has_identity_validator};

    template<convex_space RHSValueSpace, class RHSBasis>
    constexpr static bool is_composable_with{
         consistent_bases_v<basis_type, RHSBasis>
      && (is_non_negative_orthant_v<space_type>    || vector_space<space_type>)
      && (is_non_negative_orthant_v<RHSValueSpace> || vector_space<RHSValueSpace>)
    };

    template<convex_space RHSValueSpace, class RHSBasis>
    constexpr static bool is_multipicable_with{
         is_composable_with<RHSValueSpace, RHSBasis>
      && ((D == 1) || (free_module_type_of_t<RHSValueSpace>::dimension == 1))
    };

    template<convex_space RHSValueSpace, class RHSRepresentation, class RHSBasis>
    constexpr static bool is_divisible_with{
         weak_field<displacement_value_type>
      && weak_field<typename RHSRepresentation::value_type>
      && is_composable_with<RHSValueSpace, RHSBasis>
      && (free_module_type_of_t<RHSValueSpace>::dimension == 1)
    };

    using coordinates_type::coordinates_type;

    template<convex_space OtherValueSpace, basis_for<free_module_type_of_t<OtherValueSpace>> OtherBasis, class OtherOrigin>
      requires (!std::same_as<space_type, OtherValueSpace>)
           && has_distinguished_origin_v<space_type>
           && have_compatible_base_spaces_v<space_type, OtherValueSpace>
           && consistent_bases_v<basis_type, OtherBasis>
    [[nodiscard]]
    // TO DO: refine this
    friend constexpr auto operator+(const physical_value& lhs, const physical_value<OtherValueSpace, Unit, OtherBasis, representation_type, OtherOrigin, validator_type>& rhs)
    {
      using value_space_t    = to_base_space_t<space_type>;
      using basis_t          = consistent_bases<basis_type, OtherBasis>::template rebind_type<free_module_type_of_t<value_space_t>, Unit>;
      using physical_value_t = physical_value<value_space_t, Unit, basis_t, representation_type, to_origin_type_t<value_space_t>, validator_type>;

      return [&] <std::size_t... Is>(std::index_sequence<Is...>) {
        return physical_value_t{std::array{(lhs.values()[Is] + rhs.values()[Is])...}, units_type{}};
      }(std::make_index_sequence<D>{});
    }

    template<class OtherValueSpace, basis_for<free_module_type_of_t<OtherValueSpace>> OtherBasis, class OtherOrigin>
      requires (!std::same_as<space_type, OtherValueSpace>)
            && (!std::same_as<displacement_space_type, OtherValueSpace>)
            && have_compatible_base_spaces_v<space_type, OtherValueSpace>
            && consistent_bases_v<basis_type, OtherBasis>
    [[nodiscard]]
    // TO DO: refine this
    friend constexpr auto operator-(const physical_value& lhs, const physical_value<OtherValueSpace, Unit, OtherBasis, representation_type, OtherOrigin, validator_type>& rhs)
      noexcept(has_identity_validator)
    {
      using disp_space_t = to_displacement_space_t<ValueSpace, OtherValueSpace>;
      using basis_t      = consistent_bases<basis_type, OtherBasis>::template rebind_type<free_module_type_of_t<disp_space_t>, Unit>;
      using disp_t       = to_coordinates_base_type<disp_space_t, Unit, basis_t, representation_type, Validator>::displacement_coordinates_type;
      return[&] <std::size_t... Is>(std::index_sequence<Is...>) {
        return disp_t{std::array{(lhs.values()[Is] - rhs.values()[Is])...}, units_type{}};
      }(std::make_index_sequence<D>{});
    }

    template<
      convex_space RHSValueSpace,
      physical_unit RHSUnit,
      basis_for<free_module_type_of_t<RHSValueSpace>> RHSBasis,      
      representation_for<RHSValueSpace> RHSRepresentation,
      class RHSOrigin
    >
      requires is_multipicable_with<RHSValueSpace, RHSBasis> // TO DO: include repr, origin
    [[nodiscard]]
    // TO DO: move to derived class
    friend constexpr auto operator*(const physical_value& lhs,
                                    const physical_value<RHSValueSpace, RHSUnit, RHSBasis, RHSRepresentation, RHSOrigin, validator_type>& rhs)
    {
      using physical_value_t
        = physical_value_product_t<
            physical_value,
            physical_value<RHSValueSpace,RHSUnit, RHSBasis, RHSRepresentation, RHSOrigin, validator_type>
          >;

      using derived_units_type = physical_value_t::units_type;
      return physical_value_t{lhs.value() * rhs.value(), derived_units_type{}};
    }

    template<
      convex_space RHSValueSpace,
      physical_unit RHSUnit,
      basis_for<free_module_type_of_t<RHSValueSpace>> RHSBasis,
      class RHSOrigin,
      representation_for<RHSValueSpace> RHSRepresentation
    >
    requires is_divisible_with<RHSValueSpace, RHSRepresentation, RHSBasis> // TO DO: include origin
    [[nodiscard]]
    friend constexpr auto operator/(const physical_value& lhs,
                                    const physical_value<RHSValueSpace, RHSUnit, RHSBasis, RHSRepresentation, RHSOrigin, validator_type>& rhs)
    {
      using dual_rep_t = dual_of_t<RHSRepresentation>;
      using physical_value_t
        = physical_value_product_t<
            physical_value,
            physical_value<dual_of_t<RHSValueSpace>, dual_of_t<RHSUnit>, dual_of_t<RHSBasis>,  dual_rep_t, distinguished_origin, validator_type>
          >;
      using derived_units_type = physical_value_t::units_type;

      return[&] <std::size_t... Is>(std::index_sequence<Is...>) {
        return physical_value_t{std::array{(lhs.values()[Is] / rhs.value())...}, derived_units_type{}};
      }(std::make_index_sequence<D>{});
    }

    [[nodiscard]] friend constexpr auto operator/(value_type value, const physical_value& rhs)
      requires ((D == 1) && (is_non_negative_orthant_v<space_type> || vector_space<ValueSpace>))
    {
      using dual_rep_t = dual_of_t<representation_type>;
      using physical_value_t = physical_value<dual_of_t<ValueSpace>, dual_of_t<Unit>, dual_of_t<basis_type>, dual_rep_t, distinguished_origin, validator_type>;
      using derived_units_type = physical_value_t::units_type;
      return physical_value_t{value / rhs.value(), derived_units_type{}};
    }
 
    template<class Self, class LoweredValueSpace, basis_for<free_module_type_of_t<LoweredValueSpace>> OtherBasis>    
      requires std::same_as<to_base_space_t<space_type>, LoweredValueSpace> && consistent_bases_v<basis_type, OtherBasis>
    [[nodiscard]]
    constexpr operator physical_value<LoweredValueSpace, Unit, OtherBasis, representation_type, Origin, validator_type>(this const Self& self) noexcept
    {
      using physical_value_t = physical_value<LoweredValueSpace, Unit, OtherBasis, representation_type, Origin, validator_type>;
      
      return [&self] <std::size_t... Is>(std::index_sequence<Is...>) {
        return physical_value_t{std::array{self.values()[Is]...}, Unit{}};
      }(std::make_index_sequence<D>{});
    }

    template<
      physical_unit OtherUnit,
      convex_space OtherSpace                                       = conversion_space_t<ValueSpace, Unit, OtherUnit>,
      basis_for<free_module_type_of_t<OtherSpace>> OtherBasis       = unit_defined_right_handed_basis<free_module_type_of_t<OtherSpace>, OtherUnit>,      
      representation_for<OtherSpace> OtherRepresentation            = default_representation_t<OtherSpace, value_type, OtherUnit>,
      class OtherOrigin                                             = to_origin_type_t<OtherSpace>,
      validator_for<OtherSpace, OtherRepresentation> OtherValidator = throwing_validator
    >
      requires has_quantity_conversion_v<physical_value, physical_value<OtherSpace, OtherUnit, OtherBasis, OtherRepresentation, OtherOrigin, OtherValidator>>
    [[nodiscard]]
    constexpr physical_value<OtherSpace, OtherUnit, OtherBasis, OtherRepresentation, OtherOrigin, OtherValidator> convert_to(OtherUnit) const
     noexcept(has_noexcept_coordinate_transformation_v<physical_value, physical_value<OtherSpace, OtherUnit, OtherBasis, OtherRepresentation, OtherOrigin,  OtherValidator>>)
    {
      return coordinate_transformation<physical_value, physical_value<OtherSpace, OtherUnit, OtherBasis, OtherRepresentation, OtherOrigin, OtherValidator>>{}(*this);
    }

    [[nodiscard]]
    constexpr physical_value convert_to(units_type) const noexcept { return *this; }
  };
    
  // TO DO: rethink this. We probably don't need Rep.
  // But this does beg the question about e.g. integral masses

  template<physical_unit Unit, class Rep>
  struct default_space {};

  template<physical_unit Unit, class Rep>
  using default_space_t = default_space<Unit, Rep>::type;

  template<physical_unit Unit, class Rep>
  inline constexpr bool has_default_space_v{
    requires {
      typename default_space_t<Unit, Rep>;
    }
  };

  template<physical_unit Unit, class Rep>
    requires has_default_space_v<Unit, Rep>
  struct default_space<dual<Unit>, Rep>
  {
    using type = dual_of_t<default_space_t<Unit, Rep>>;
  };

  template<physical_unit... Ts, class Rep>
    requires (has_default_space_v<Ts, Rep> && ...)
  struct default_space<composite_unit<Ts...>, Rep>
  {
    using type = impl::to_composite_space_t<reduction_t<tensor_product<default_space_t<Ts, Rep>...>>>;
  };

  // TO DO: a default valiator?
  template<class T, physical_unit Unit>
    requires has_default_space_v<Unit, T>
  physical_value(T, Unit)
    -> physical_value<
         default_space_t<Unit, T>,
         Unit,
         unit_defined_right_handed_basis<free_module_type_of_t<default_space_t<Unit, T>>, Unit>,
         default_representation_t<default_space_t<Unit, T>, T, Unit>,
         to_origin_type_t<default_space_t<Unit, T>>,
         throwing_validator
       >;

  namespace sets::classical
  {
    template<class Arena>
    struct masses
    {
      using arena_type = Arena;
    };

    template<class Arena>
    struct temperatures
    {
      using arena_type = Arena;
    };

    template<class Arena>
    struct electrical_currents
    {
      using arena_type = Arena;
    };

    template<class Arena>
    struct times
    {
      using arena_type = Arena;
    };

    template<class Arena>
    struct time_intervals
    {
      using arena_type = Arena;
    };

    template<std::size_t D, class Arena>
    struct positions
    {
      using arena_type = Arena;
    };

    template<class Arena>
    struct lengths
    {
      using arena_type = Arena;
    };

    template<class Arena>
    struct angles
    {
      using arena_type = Arena;
    };

    template<class PhysicalValueSet>
    struct differences
    {
      using physical_value_set_type = PhysicalValueSet;
      using algebraic_structure     = identifies_as_field_t; // TO DO: generalize this
    };
  }
  
  template<class Space>
  struct associated_displacement_space
  {
    constexpr static std::size_t dimension{Space::dimension};
    using set_type              = sets::classical::differences<typename Space::set_type>;
    using commutative_ring_type = sets::classical::differences<typename Space::set_type>;
    using is_free_module        = std::true_type;
    using arena_type            = Space::arena_type;
  };

  template<class Space>
      requires has_base_space_v<Space>
  struct associated_displacement_space<Space>
  {
    constexpr static std::size_t dimension{Space::dimension};
    using set_type              = sets::classical::differences<typename Space::set_type>;
    using commutative_ring_type = sets::classical::differences<typename Space::set_type>;
    using is_free_module        = std::true_type;
    using base_space            = associated_displacement_space<typename Space::base_space>;
    using arena_type            = Space::arena_type;
  };

  template<class PhysicalValueSet, std::size_t D, class Derived>
  struct physical_value_convex_space
  {
    constexpr static std::size_t dimension{D};
    using set_type            = PhysicalValueSet;
    using free_module_type    = associated_displacement_space<Derived>;
    using is_convex_space     = std::true_type;
    using arena_type          = PhysicalValueSet::arena_type;
  };

  template<class PhysicalValueSet, std::size_t D, class Derived>
  struct physical_value_affine_space
  {
    constexpr static std::size_t dimension{D};
    using set_type            = PhysicalValueSet;
    using free_module_type    = associated_displacement_space<Derived>;
    using is_affine_space     = std::true_type;
    using arena_type          = PhysicalValueSet::arena_type;
  };

  // TO DO: probably want this inheriting from an elementary space? Maybe as an isomorphism...?
  template<class PhysicalValueSet, class Field, std::size_t D, class Derived>
  struct physical_value_vector_space
  {
    constexpr static std::size_t dimension{D};
    using set_type            = PhysicalValueSet;
    using field_type          = Field;
    using is_vector_space     = std::true_type;
  };

  template<class Arena>
  struct mass_space
    : physical_value_convex_space<sets::classical::masses<Arena>, 1, mass_space<Arena>>
  {
    using arena_type           = Arena;
    using base_space           = mass_space;
    using distinguished_origin = std::true_type;
    using non_negative_orthant = std::true_type;
  };

  template<class Arena>
  struct absolute_temperature_space
    : physical_value_convex_space<sets::classical::temperatures<Arena>, 1, absolute_temperature_space<Arena>>
  {
    using arena_type           = Arena;
    using base_space           = absolute_temperature_space;
    using distinguished_origin = std::true_type;
    using non_negative_orthant = std::true_type;
  };

  template<convex_space C>
    requires has_distinguished_origin_v<C>
  struct relaxed_space : C
  {
    using base_space           = relaxed_space<typename C::base_space>;
    using free_module_type     = associated_displacement_space<relaxed_space>;
    using distinguished_origin = std::false_type;
    using non_negative_orthant = std::false_type;
  };

  template<class Arena>
  using temperature_space = relaxed_space<absolute_temperature_space<Arena>>;
  
  template<class Arena>
  struct electrical_current_space
    : physical_value_vector_space<sets::classical::electrical_currents<Arena>, maths::commutative_rings::reals<1>, 1, electrical_current_space<Arena>>
  {
    using arena_type = Arena;
    using base_space = electrical_current_space;
  };

  template<class Arena>
  struct angular_space : physical_value_vector_space<sets::classical::angles<Arena>, maths::commutative_rings::reals<1>, 1, angular_space<Arena>>
  {
    using arena_type = Arena;
    using base_space = angular_space;
  };

  template<class Arena>
  struct length_space
    : physical_value_convex_space<sets::classical::lengths<Arena>, 1, length_space<Arena>>
  {
    using arena_type           = Arena;
    using base_space           = length_space;
    using distinguished_origin = std::true_type;
    using non_negative_orthant = std::true_type;
  };

  template<class Arena>
  struct width_space : length_space<Arena>
  {
    struct free_module_type : associated_displacement_space<width_space<Arena>> {};
  };

  template<class Arena>
  struct height_space : length_space<Arena>
  {
    struct free_module_type : associated_displacement_space<height_space<Arena>> {};
  };

  template<class Arena>
  struct radius_space : length_space<Arena>
  {
    struct free_module_type : associated_displacement_space<radius_space<Arena>> {};
  };

  template<class Arena>
  struct time_interval_space
    : physical_value_convex_space<sets::classical::time_intervals<Arena>, 1, time_interval_space<Arena>>
  {
    using arena_type           = Arena;
    using distinguished_origin = std::true_type;
    using non_negative_orthant = std::true_type;
  };
  
  template<class Arena>
  struct time_space : physical_value_affine_space<sets::classical::times<Arena>, 1, time_space<Arena>>
  {
    using arena_type = Arena;
  };

  template<std::size_t D, class Arena>
  struct position_space : physical_value_affine_space<sets::classical::positions<D, Arena>, D, position_space<D, Arena>>
  {
    using arena_type = Arena;
  };
  
  struct implicit_common_arena {};

  template<physical_unit U>
  inline constexpr bool has_symbol_v{
    requires {
      { U::symbol } -> std::convertible_to<std::string_view>; }
  };

  template<auto Bounds>
    requires bounds_value<Bounds>
  struct scale_invariant_bounds : std::false_type {};

  template<auto Bounds>
    requires bounds_value<Bounds>
  using scale_invariant_bounds_t = scale_invariant_bounds<Bounds>::type;

  template<auto Bounds>
    requires bounds_value<Bounds>
  inline constexpr bool scale_invariant_bounds_v{scale_invariant_bounds<Bounds>::value};

  template<auto Bounds>
    requires bounds_value<Bounds>
          && ((Bounds.lower == Bounds.least_lower_bound)    || (Bounds.lower == 0))
          && ((Bounds.upper == Bounds.greatest_upper_bound) || (Bounds.upper == 0))
  struct scale_invariant_bounds<Bounds>
    : std::true_type
  {};

  template<auto Bounds>
    requires bounds_value<Bounds>
  struct translation_invariant_bounds : std::false_type {};

  template<auto Bounds>
    requires bounds_value<Bounds>
  using translation_invariant_bounds_t = translation_invariant_bounds<Bounds>::type;

  template<auto Bounds>
    requires bounds_value<Bounds>
  inline constexpr bool translation_invariant_bounds_v{translation_invariant_bounds<Bounds>::value};

  template<auto Bounds>
    requires bounds_value<Bounds>
          && (Bounds.lower == Bounds.least_lower_bound)
          && (Bounds.upper == Bounds.greatest_upper_bound)
  struct translation_invariant_bounds<Bounds>
    : std::true_type
  {};

  template<class...>
  struct product;

  template<class... Ts>
  using product_t = product<Ts...>::type;

  template<class T, class U, class... Vs>
  struct product<T, U, Vs...>
  {
    using type = product_t<product_t<T, U>, Vs...>;
  }; 

  template<class>
  struct inverse;

  template<class T>
  using inverse_t = inverse<T>::type;

  template<auto Num, auto Den>
  struct inverse<dilatation<ratio<Num, Den>>>
  {
    using type = dilatation<ratio<Den, Num>>;
  };

  template<std::intmax_t Num, std::intmax_t Den>
  struct inverse<dilatation<std::ratio<Num, Den>>>
  {
    using type = dilatation<std::ratio<Den, Num>>;
  };

  template<auto Displacement>
  struct inverse<translation<Displacement>>
  {
    using type = translation<-Displacement>;
  };

  template<physical_unit U, class Ratio, auto Displacement>
  struct coordinate_transform<U, dilatation<Ratio>, translation<Displacement>>
  {
    using is_unit              = std::true_type;
    using transform_type       = coordinate_transform<U, dilatation<Ratio>, translation<Displacement>>;
    using with_respect_to_type = U;
    using dilatation_type      = dilatation<Ratio>;
    using translation_type     = translation<Displacement>;
  };

  template<physical_unit U, class Ratio, auto Displacement>
  struct inverse<coordinate_transform<U, dilatation<Ratio>, translation<Displacement>>>
  {
    using inverse_dil_type   = inverse_t<dilatation<Ratio>>;
    using inverse_ratio_type = inverse_dil_type::ratio_type;
    using type               = coordinate_transform<U, inverse_dil_type, inverse_t<translation<Displacement * inverse_ratio_type::num / inverse_ratio_type::den>>>;
  };

  template<
    physical_unit LHSUnit, class LHSRatio, auto LHSDisplacement,
    physical_unit RHSUnit, class RHSRatio, auto RHSDisplacement
  >
  struct product<coordinate_transform<LHSUnit, dilatation<LHSRatio>, translation<LHSDisplacement>>,
                 coordinate_transform<RHSUnit, dilatation<RHSRatio>, translation<RHSDisplacement>>>
  {
    using dilatation_type  = dilatation<ratio_multiply<LHSRatio, RHSRatio, allow_ratio_fp_conversion::yes>>;
    using translation_type = translation<LHSDisplacement + RHSDisplacement * LHSRatio::num / LHSRatio::den>;
    using type             = coordinate_transform<RHSUnit, dilatation_type, translation_type>;
  };

  template<class T>
  struct is_coordinate_transform : std::false_type {};

  template<class T>
  using is_coordinate_transform_t = is_coordinate_transform<T>::type;

  template<class T>
  inline constexpr bool is_coordinate_transform_v{is_coordinate_transform<T>::value};

  template<physical_unit U, class Ratio, auto Displacement>
  struct is_coordinate_transform<coordinate_transform<U, dilatation<Ratio>, translation<Displacement>>> : std::true_type {};

  template<physical_unit U>
  inline constexpr bool has_coordinate_transform_v{
    requires {
      typename U::transform_type;
      requires is_coordinate_transform_v<typename U::transform_type>;
    }
  };
  
  template<physical_unit U>
  inline constexpr bool derives_from_another_unit_v{
    requires {
      typename U::with_respect_to_type;
      requires physical_unit<typename U::with_respect_to_type>;
    }
  };

  template<physical_unit U>
  struct root_transform
  {
    using transform_type = coordinate_transform<U, dilatation<std::ratio<1, 1>>, translation<0>>;
    using units_type     = U;
  };

  template<physical_unit U>
  using root_transform_unit_t = root_transform<U>::units_type;

  template<physical_unit U>
    requires derives_from_another_unit_v<U>
         && (!derives_from_another_unit_v<typename U::with_respect_to_type>)
  struct root_transform<U> : root_transform<typename U::with_respect_to_type>
  {
    using transform_type = U::transform_type;
  };

  template<physical_unit U>
    requires derives_from_another_unit_v<U>
          && derives_from_another_unit_v<typename U::with_respect_to_type>
  struct root_transform<U> : root_transform<typename U::with_respect_to_type>
  {
    using wrt_type = typename U::with_respect_to_type;
    using nested_transform_type = root_transform_t<wrt_type>;
    using transform_type = product_t<typename U::transform_type, nested_transform_type>;
  };

  template<physical_unit... Us>
  struct root_transform<composite_unit<Us...>>
  {
    using units_type = decltype((root_transform_unit_t<Us>{} * ...));
    using transform_type = product_t<root_transform_t<Us>...>;
  };

  template<class T>
  struct has_identity_dilatation : std::false_type {};

  template<class T>
  using has_identity_dilatation_t = has_identity_dilatation<T>::type;

  template<class T>
  inline constexpr bool has_identity_dilatation_v{has_identity_dilatation<T>::value};

  template<physical_unit U, class Ratio, auto Displacement>
  struct has_identity_dilatation<coordinate_transform<U, dilatation<Ratio>, translation<Displacement>>>
    : std::bool_constant<Ratio::num == Ratio::den>
  {};

  template<class T>
  struct has_identity_translation : std::false_type {};

  template<class T>
  using has_identity_translation_t = has_identity_translation<T>::type;

  template<class T>
  inline constexpr bool has_identity_translation_v{has_identity_translation<T>::value};

  template<physical_unit U, class Ratio, auto Displacement>
  struct has_identity_translation<coordinate_transform<U, dilatation<Ratio>, translation<Displacement>>>
    : std::bool_constant<Displacement == 0>
  {}; 
  
  template<convex_space C, physical_unit FromUnit, physical_unit ToUnit>
    requires (!has_distinguished_origin_v<C>)
          || (!has_identity_translation_v<root_transform_t<FromUnit>> && !has_identity_translation_v<root_transform_t<ToUnit>>)
  struct conversion_space<C, FromUnit, ToUnit>
  {
    using type = C;
  };

  template<convex_space C, physical_unit FromUnit, physical_unit ToUnit>
    requires has_distinguished_origin_v<C>
  && (has_identity_translation_v<root_transform_t<FromUnit>> && !has_identity_translation_v<root_transform_t<ToUnit>>)
  struct conversion_space<C, FromUnit, ToUnit>
  {
    using type = relaxed_space<C>;
  };

  template<convex_space C, physical_unit FromUnit, physical_unit ToUnit>
    requires has_distinguished_origin_v<C> && has_identity_translation_v<root_transform_t<ToUnit>>
  struct conversion_space<relaxed_space<C>, FromUnit, ToUnit>
  {
    using type = C;
  };
  
  template<physical_unit Unit>
  struct micro : coordinate_transform<Unit, dilatation<std::mega>, translation<0>>
  {
    using transform_type = coordinate_transform<Unit, dilatation<std::mega>, translation<0>>;
  };
  
  template<physical_unit Unit>
  struct milli : coordinate_transform<Unit, dilatation<std::kilo>, translation<0>>
  {
    using transform_type = coordinate_transform<Unit, dilatation<std::kilo>, translation<0>>;
  };

  template<physical_unit Unit>
  struct kilo : coordinate_transform<Unit, dilatation<std::milli>, translation<0>>
  {
    using transform_type = coordinate_transform<Unit, dilatation<std::milli>, translation<0>>;
  };

  template<physical_unit Unit>
  struct mega : coordinate_transform<Unit, dilatation<std::micro>, translation<0>>
  {
    using transform_type = coordinate_transform<Unit, dilatation<std::micro>, translation<0>>;
  };
  
  // TO DO: these namespace have been made inline to workaround an MSVC bug
  // https://developercommunity.visualstudio.com/t/Overload-resolution-failing-with-a-class/10977207
  // It may be worth making them inline, regardless, and possibly abandoning
  // the inner namespace. I need to think about this.
  inline namespace si
  {
    inline namespace units
    {
      struct ampere_t
      {
        using is_unit        = std::true_type;
        constexpr static std::string_view symbol{"A"};        
      };
    
      struct kilogram_t
      {
        using is_unit        = std::true_type;
        constexpr static std::string_view symbol{"kg"};
      };

      struct metre_t
      {
        using is_unit        = std::true_type;
        constexpr static std::string_view symbol{"m"};
      };

      struct second_t
      {
        using is_unit        = std::true_type;
        constexpr static std::string_view symbol{"s"};
      };

      struct kelvin_t
      {
        using is_unit        = std::true_type;
        constexpr static std::string_view symbol{"K"};
      };

      struct coulomb_t
      {
        using is_unit        = std::true_type;
        constexpr static std::string_view symbol{"C"};
      };

      struct radian_t
      {
        using is_unit        = std::true_type;
        constexpr static std::string_view symbol{"rad"};
      };

      struct celsius_t : coordinate_transform<kelvin_t, dilatation<std::ratio<1, 1>>, translation<-273.15L>>
      {
        constexpr static std::string_view symbol{"degC"};
      };

      inline constexpr ampere_t   ampere{};
      inline constexpr kilogram_t kilogram{};
      inline constexpr metre_t    metre{};
      inline constexpr second_t   second{};
      inline constexpr kelvin_t   kelvin{};
      inline constexpr coulomb_t  coulomb{};
      inline constexpr radian_t   radian{};

      inline constexpr celsius_t celsius{};

      using milligram_t = micro<si::units::kilogram_t>;
      using gram_t      = milli<si::units::kilogram_t>;
      using tonne_t     = kilo<si::units::kilogram_t>;
      using kilotonne_t = mega<si::units::kilogram_t>;

      inline constexpr milligram_t milligram{};
      inline constexpr gram_t      gram{};
      inline constexpr tonne_t     tonne{};
      inline constexpr kilotonne_t kilotonne{};
    }

    template<convex_space Space, physical_unit Unit>
    struct default_basis
    {
      using type = unit_defined_right_handed_basis<free_module_type_of_t<Space>, Unit>;
    };

    template<convex_space Space, physical_unit Unit>
    using default_basis_t = default_basis<Space, Unit>::type;

    template<convex_space Space, physical_unit Unit, weak_commutative_ring Rep, class Validator>
    using basic_quantity
      = physical_value<
          Space,
          Unit,
          default_basis_t<Space, Unit>,
          default_representation_t<Space, Rep, Unit>,
          to_origin_type_t<Space>,
          Validator
        >;
    
    template<std::floating_point T, class Validator=throwing_validator, class Arena=implicit_common_arena>
    using mass = basic_quantity<mass_space<Arena>, units::kilogram_t, T, Validator>;

    template<std::floating_point T, class Validator=throwing_validator, class Arena=implicit_common_arena>
    using length = basic_quantity<length_space<Arena>, units::metre_t, T, Validator>;

    template<std::floating_point T, class Validator=throwing_validator, class Arena=implicit_common_arena>
    using time_interval = basic_quantity<time_interval_space<Arena>, units::second_t, T, Validator>;

    template<std::floating_point T, class Validator=throwing_validator, class Arena=implicit_common_arena>
    using temperature = basic_quantity<absolute_temperature_space<Arena>, units::kelvin_t, T, Validator>;

    template<std::floating_point T, class Validator=throwing_validator, class Arena=implicit_common_arena>
    using temperature_celsius = basic_quantity<temperature_space<Arena>, units::celsius_t, T, Validator>;

    template<std::floating_point T, class Validator=throwing_validator, class Arena=implicit_common_arena>
    using electrical_current = basic_quantity<electrical_current_space<Arena>, units::ampere_t, T, Validator>;

    template<std::floating_point T, class Validator=throwing_validator, class Arena=implicit_common_arena>
    using angle = basic_quantity<angular_space<Arena>, units::radian_t, T, Validator>;

    template<std::floating_point T, class Validator=throwing_validator, class Arena=implicit_common_arena>
    using width = basic_quantity<width_space<Arena>, units::metre_t, T, Validator>;

    template<std::floating_point T, class Validator=throwing_validator, class Arena=implicit_common_arena>
    using height = basic_quantity<height_space<Arena>, units::metre_t, T, Validator>;

    template<
      std::floating_point T,
      class Arena=implicit_common_arena,
      class Origin=implicit_affine_origin,
      class Validator=throwing_validator
    >
    using time
      = physical_value<
          time_space<Arena>,
          units::second_t,
          unit_defined_right_handed_basis<free_module_type_of_t<time_space<Arena>>, units::second_t>,      
          canonical_representation<T, no_bounds<T>>,
          Origin,
          Validator
        >;

    template<      
      std::floating_point T,
      std::size_t D,
      class Arena  = implicit_common_arena,      
      basis_for<free_module_type_of_t<position_space<D, Arena>>> Basis = unit_defined_right_handed_basis<free_module_type_of_t<position_space<D, Arena>>, units::metre_t>,
      class Origin = implicit_affine_origin,      
      class Validator=throwing_validator
    >
    using position = physical_value<position_space<D, Arena>, units::metre_t, Basis, canonical_representation<T, no_bounds<T>>, Origin, Validator>;
  }

  // TO DO: see comment above si namespace
  inline namespace non_si
  {
    inline namespace units
    {
      struct degree_t : coordinate_transform<si::units::radian_t, dilatation<ratio<std::intmax_t{180}, std::numbers::pi_v<long double>>>, translation<0>>
      {
        using is_unit        = std::true_type;
        constexpr static std::string_view symbol{"deg"};
      };

      struct gradian_t : coordinate_transform<si::units::radian_t, dilatation<ratio<std::intmax_t{200}, std::numbers::pi_v<long double>>>, translation<0>>
      {
        using is_unit        = std::true_type;
        constexpr static std::string_view symbol{"gon"};
      };

      inline constexpr degree_t degree{};
      inline constexpr gradian_t gradian{};

      struct farenheight_t : coordinate_transform<si::units::celsius_t, dilatation<std::ratio<9, 5>>, translation<32.0L>>
      {
        using is_unit = std::true_type;
        constexpr static std::string_view symbol{"degF"};
      };

      inline constexpr farenheight_t farenheight{};

      struct foot_t : coordinate_transform<si::units::metre_t, dilatation<std::ratio<10000, 3048>>, translation<0>>
      {
        using is_unit = std::true_type;
        constexpr static std::string_view symbol{"ft"};
      };

      inline constexpr foot_t foot{};
    }

    template<std::floating_point T, class Validator=throwing_validator, class Arena=implicit_common_arena>
    using temperature_farenheight = basic_quantity<temperature_space<Arena>, units::farenheight_t, T, Validator>;
  }

  template<convex_space C, physical_unit FromUnit, physical_unit ToUnit>
  struct conversion_space<associated_displacement_space<C>, FromUnit, ToUnit>
  {
    using type = associated_displacement_space<conversion_space_t<C, FromUnit, ToUnit>>;
  };

  template<physical_unit Unit, class Rep, class Ratio, auto Trans>
    requires has_default_space_v<Unit, Rep>
  struct default_space<coordinate_transform<Unit, dilatation<Ratio>, translation<Trans>>, Rep>
    : default_space<Unit, Rep>
  {};

  template<physical_unit Unit, class Rep>
    requires has_coordinate_transform_v<Unit> && (!is_coordinate_transform_v<Unit>) // Last condition only necessary for MSVC
  struct default_space<Unit, Rep> : default_space<root_transform_t<Unit>, Rep> {};

  // TO DO: Probably want to get rid of T
  template<std::floating_point T>
  struct default_space<si::units::metre_t, T>
  {
    using type = length_space<implicit_common_arena>;
  };

  template<std::floating_point T>
  struct default_space<si::units::second_t, T>
  {
    using type = time_interval_space<implicit_common_arena>;
  };

  template<std::floating_point T>
  struct default_space<si::units::kilogram_t, T>
  {
    using type = mass_space<implicit_common_arena>;
  };

  template<std::floating_point T>
  struct default_space<si::units::radian_t, T>
  {
    using type = angular_space<implicit_common_arena>;
  };

  template<std::floating_point T>
  struct default_space<si::units::kelvin_t, T>
  {
    using type = absolute_temperature_space<implicit_common_arena>;
  };

  template<std::floating_point T>
  struct default_space<si::units::celsius_t, T>
  {
    using type = temperature_space<implicit_common_arena>;
  };

  template<std::floating_point T>
  struct default_space<si::units::ampere_t, T>
  {
    using type = electrical_current_space<implicit_common_arena>;
  };
  
  template<vector_space ValueSpace, physical_unit Unit, basis_for<free_module_type_of_t<ValueSpace>> Basis, class Origin, representation_for<ValueSpace> Representation, validator_for<ValueSpace, Representation> Validator>
    requires (dimension_of<ValueSpace> == 1)
  [[nodiscard]]
  constexpr physical_value<ValueSpace, Unit, Basis, Representation, Origin, Validator> abs(physical_value<ValueSpace, Unit, Basis, Representation, Origin, Validator> q)
  {
    return {std::abs(q.value()), Unit{}};
  }

  // TO DO: constrain the unit to be a coordinate transformation of radians
  // TO DO: tidy template params, make bounds a NTTP
  template<physical_unit Unit, std::floating_point T, class Validator=throwing_validator, class Arena=implicit_common_arena>
  [[nodiscard]]
  constexpr T sin(physical_value<angular_space<Arena>, Unit, default_basis_t<angular_space<Arena>, Unit>, canonical_representation<T, no_bounds<T>>, distinguished_origin, Validator> theta)
  {
    return std::sin(theta.convert_to(si::units::radian_t{}).value());
  }

  template<physical_unit Unit, std::floating_point T, class Validator=throwing_validator, class Arena=implicit_common_arena>
  [[nodiscard]]
  constexpr T cos(physical_value<angular_space<Arena>, Unit, default_basis_t<angular_space<Arena>, Unit>, canonical_representation<T, no_bounds<T>>, distinguished_origin, Validator> theta)
  {
    return std::cos(theta.convert_to(si::units::radian_t{}).value());
  }

  template<physical_unit Unit, std::floating_point T, class Validator=throwing_validator, class Arena=implicit_common_arena>
  [[nodiscard]]
  constexpr T tan(physical_value<angular_space<Arena>, Unit, default_basis_t<angular_space<Arena>, Unit>, canonical_representation<T, no_bounds<T>>, distinguished_origin, Validator> theta)
  {
    return std::tan(theta.convert_to(si::units::radian_t{}).value());
  }

  template<physical_unit Unit = si::units::radian_t, class Arena=implicit_common_arena, class Validator=throwing_validator, std::floating_point T>
  [[nodiscard]]
  constexpr basic_quantity<angular_space<Arena>, Unit, T, Validator> asin(T x)
  {
    return {std::asin(x), Unit{}};
  }

  template<physical_unit Unit = si::units::radian_t, class Arena=implicit_common_arena, class Validator=throwing_validator, std::floating_point T>
  [[nodiscard]]
  constexpr basic_quantity<angular_space<Arena>, Unit, T, Validator> acos(T x)
  {
    return {std::acos(x), Unit{}};
  }

  template<physical_unit Unit = si::units::radian_t, class Arena=implicit_common_arena, class Validator=throwing_validator, std::floating_point T>
  [[nodiscard]]
  constexpr basic_quantity<angular_space<Arena>, Unit, T, Validator> atan(T x)
  {
    return {std::atan(x), Unit{}};
  }

  template<
    physical_unit Unit,
    class Rep,
    class Representation = default_representation_t<default_space_t<Unit, Rep>, Rep, Unit>,
    class Validator      = throwing_validator
  >
  using quantity
    = physical_value<
        default_space_t<Unit, Rep>,
        Unit,
        unit_defined_right_handed_basis<free_module_type_of_t<default_space_t<Unit, Rep>>, Unit>,
        Representation,
        to_origin_type_t<default_space_t<Unit, Rep>>,
        Validator
      >;

  template<
    convex_space ValueSpace,
    physical_unit Unit,
    weak_commutative_ring ValueType,
    class Validator = throwing_validator
  >
  using dimensionless_quantity
    = physical_value<
        ValueSpace,
        Unit,
        unit_defined_right_handed_basis<free_module_type_of_t<ValueSpace>, Unit>,    
        default_representation_t<ValueSpace, ValueType, Unit>,
        to_origin_type_t<ValueSpace>,
        Validator
      >;
  
  template<
    std::floating_point Rep,
    class Validator = throwing_validator,
    class Arena     = implicit_common_arena
  >
  using euclidean_1d_vector_quantity
    = dimensionless_quantity<
        euclidean_vector_space<1, Arena>,
        no_unit_t,
        Rep,
        Validator
       >;

  template<
    std::floating_point Rep,
    class Validator = throwing_validator,
    class Arena     = implicit_common_arena
  >
  using euclidean_half_line_quantity
    = dimensionless_quantity<
        euclidean_half_line<Arena>,
        no_unit_t,
        Rep,
        Validator
      >;

  template<physical_unit Unit, arithmetic Rep>
    requires has_default_space_v<Unit, Rep>
  [[nodiscard]]
  quantity<Unit, Rep> operator*(Rep val, Unit u)
  {
    return {val, u};
  }

  template<arithmetic Rep, physical_unit Unit>
    requires has_default_space_v<Unit, Rep> && permissible_value_space_v<dual_of_t<default_space_t<Unit, Rep>>>
  [[nodiscard]]
  quantity<dual_of_t<Unit>, Rep> operator/(Rep val, Unit)
  {
    return {val, dual_of_t<Unit>{}};
  }
}

namespace sequoia::maths
{
  using namespace physics;

  template<
    convex_space ValueSpaceFrom,
    physical_unit UnitFrom,
    basis_for<free_module_type_of_t<ValueSpaceFrom>> BasisFrom,    
    representation_for<ValueSpaceFrom> RepresentationFrom,  
    class OriginFrom,
    validator_for<ValueSpaceFrom, RepresentationFrom> ValidatorFrom,
    convex_space ValueSpaceTo,
    basis_for<free_module_type_of_t<ValueSpaceTo>> BasisTo,
    physical_unit UnitTo,    
    representation_for<ValueSpaceTo> RepresentationTo,
    class OriginTo,
    validator_for<ValueSpaceTo, RepresentationTo> ValidatorTo
  >
    requires std::same_as<root_transform_unit_t<UnitFrom>, root_transform_unit_t<UnitTo>>  
  struct coordinate_transformation<
    physical_value<ValueSpaceFrom, UnitFrom, BasisFrom, RepresentationFrom, OriginFrom, ValidatorFrom>,
    physical_value<ValueSpaceTo,   UnitTo,   BasisTo,   RepresentationTo,   OriginTo,   ValidatorTo>
  >
  {
    using value_type      = typename RepresentationFrom::value_type;
    using from_units_type = UnitFrom;
    using from_type       = physical_value<ValueSpaceFrom, from_units_type, BasisFrom, RepresentationFrom, OriginFrom, ValidatorFrom>;
    using to_units_type   = UnitTo;
    using to_type         = physical_value<ValueSpaceTo, to_units_type, BasisTo,  RepresentationTo, OriginTo, ValidatorTo>;
    using transform_type  = product_t<root_transform_t<UnitTo>, inverse_t<root_transform_t<UnitFrom>>>;

    constexpr static auto to_displacement() noexcept {
      if constexpr(free_module<ValueSpaceFrom>)
        return value_type{};
      else
        return transform_type::translation_type::displacement;
    };

    [[nodiscard]]
    constexpr to_type operator()(const from_type& pv)
    {
      return {
        utilities::to_array(
          pv.values(),
          [](value_type v) -> value_type {
            using ratio_type = transform_type::dilatation_type::ratio_type;

            return static_cast<value_type>((v * ratio_type::num / ratio_type::den) + to_displacement());
          }
        ),
        to_units_type{}
      };
    }
  };
}

namespace sequoia::maths
{
  // TO DO Generalize this
  template<class S, class T>
  struct common_ring<physics::sets::classical::differences<S>, physics::sets::classical::differences<T>>
  {
    using type = commutative_rings::reals<1>;
  };

  template<class T>
  struct common_ring<commutative_rings::reals<1>, physics::sets::classical::differences<T>>
  {
    using type = commutative_rings::reals<1>;
  };

  template<class T>
  struct common_ring<physics::sets::classical::differences<T>, commutative_rings::reals<1>>
    : common_ring<commutative_rings::reals<1>, physics::sets::classical::differences<T>>
  {
  };
}


template<
  sequoia::maths::convex_space ValueSpace,
  sequoia::physics::physical_unit Unit,
  sequoia::maths::basis_for<sequoia::maths::free_module_type_of_t<ValueSpace>> Basis,  
  sequoia::maths::representation_for<ValueSpace> Representation,
  class Origin,
  sequoia::maths::validator_for<ValueSpace, Representation> Validator
>
struct std::formatter<sequoia::physics::physical_value<ValueSpace, Unit, Basis, Representation, Origin, Validator>>
{
  using physical_value_type = sequoia::physics::physical_value<ValueSpace, Unit, Basis, Representation, Origin, Validator>;
  constexpr static auto dimension{sequoia::maths::dimension_of<ValueSpace> };

  constexpr auto parse(auto& ctx)
  {
    return ctx.begin();
  }

  auto format(const physical_value_type& v, auto& ctx) const
    requires (dimension == 1)
  {
    if constexpr(sequoia::physics::has_symbol_v<Unit>)
      return std::format_to(ctx.out(), "{} {}", v.value(), Unit::symbol);
    else
      return std::format_to(ctx.out(), "{}", v.value());
  }

  // TO DO: reinstate when formatting is working for spans 
  /*auto format(const physical_value_type& v, auto& ctx) const
    requires (dimension > 1)
  {
    if constexpr(sequoia::physics::has_symbol_v<Unit>)
      return std::format_to(ctx.out(), "{} {}", v.values(), Unit::symbol);
    else
      return std::format_to(ctx.out(), "{}", v.values());
  }*/
};

