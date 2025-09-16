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
#include <ratio>

namespace sequoia::physics
{
  template<class T>
  struct reciprocal_validator;

  template<class T>
  using reciprocal_validator_t = reciprocal_validator<T>::type;

  template<>
  struct reciprocal_validator<std::identity>
  {
    using type = std::identity;
  };

  template<>
  struct reciprocal_validator<maths::half_line_validator>
  {
    using type = maths::half_line_validator;
  };

  template<class T>
  inline constexpr bool has_reciprocal_validator_v{
    requires { typename reciprocal_validator_t<T>; }
  };
}

namespace sequoia::maths
{
  template<physics::physical_unit T>
    requires physics::has_reciprocal_validator_v<typename T::validator_type>
  struct dual<T>
  {
    using validator_type = physics::reciprocal_validator_t<typename T::validator_type>;
  };

  /** @brief Specialization for units, such as degrees Celsius, for which
             the corresponding quantity cannot be inverted.

             Note, though, that inverse units of this type can appear in
             displacements, where the validator is overridden, assumed to
             be std::identity
   */
  template<physics::physical_unit T>
    requires (!physics::has_reciprocal_validator_v<typename T::validator_type>)
  struct dual<T>
  {
    using validator_type = void;
  };
}

namespace sequoia::physics
{
  using namespace maths;

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

  struct no_unit_t
  {
    using validator_type = maths::half_line_validator;
  };

  inline constexpr no_unit_t no_unit{};

  template<physical_unit... Ts>
  struct composite_unit
  {
    using validator_type = reduced_validator_t<typename Ts::validator_type...>;
  };

  template<class T>
  inline constexpr bool has_arena_type_v{
    requires { typename T::arena_type;}
  };

  template<class T>
  struct arena_type_of;

  template<class T>
  using arena_type_of_t = arena_type_of<T>::type;

  template<class T>
    requires has_arena_type_v<T>
  struct arena_type_of<T>
  {
    using type = T::arena_type;
  };

  template<convex_space T>
    requires (!has_arena_type_v<dual<T>>)
  struct arena_type_of<dual<T>>
  {
    using type = arena_type_of_t<T>;
  };

  template<convex_space... Ts>
    requires (!has_arena_type_v<direct_product<Ts...>>)
  struct arena_type_of<direct_product<Ts...>>
  {
    using type = std::common_type_t<arena_type_of_t<Ts>...>;
  }; 

  /// @class Primary class template for the reduction of direct products to a lower dimensional space
  template<class T>
  struct reduction;

  template<class T>
  using reduction_t = reduction<T>::type;
  
  template<class... Ts>
  struct composite_space;

  template<convex_space... Ts>
    requires (free_module<Ts> ||  ...)
  struct composite_space<Ts...>
  {    
    using direct_product_t      = direct_product<Ts...>;
    using set_type              = reduction<typename direct_product_t::set_type>;
    using commutative_ring_type = commutative_ring_type_of_t<direct_product_t>;
    using is_free_module        = std::true_type;
    using arena_type            = arena_type_of<direct_product<Ts...>>;
    constexpr static std::size_t dimension{std::ranges::max({dimension_of<Ts>...})};
  };

  template<convex_space... Ts>
    requires (!affine_space<Ts> && ...)
  struct composite_space<Ts...>
  {
    using direct_product_t     = direct_product<Ts...>;
    using set_type             = reduction<typename direct_product_t::set_type>;
    using free_module_type     = composite_space<free_module_type_of_t<Ts>...>;
    using is_convex_space      = std::true_type;
    using arena_type           = arena_type_of<direct_product<Ts...>>;
    using distinguished_origin = std::bool_constant<(has_distinguished_origin_v<Ts> && ...)>;
  };

  template<physical_unit T, physical_unit U>
  struct reduction<direct_product<T, U>>
  {
    using type = impl::simplify_t<direct_product<T>, direct_product<U>>;
  };

  template<physical_unit... Ts, physical_unit U>
  struct reduction<direct_product<composite_unit<Ts...>, U>>
  {
    using type = impl::simplify_t<direct_product<Ts...>, direct_product<U>>;
  };

  template<physical_unit T, physical_unit... Us>
  struct reduction<direct_product<T, composite_unit<Us...>>>
  {
    using type = impl::simplify_t<direct_product<T>, direct_product<Us...>>;
  };

  template<physical_unit... Ts, physical_unit... Us>
  struct reduction<direct_product<composite_unit<Ts...>, composite_unit<Us...>>>
  {
    using type = impl::simplify_t<direct_product<Ts...>, direct_product<Us...>>;
  };

  template<convex_space T, convex_space U>
  struct reduction<direct_product<T, U>>
  {    
    using type = impl::simplify_t<direct_product<T>, direct_product<U>>;
  };

  template<convex_space... Ts, convex_space U>
  struct reduction<direct_product<composite_space<Ts...>, U>>
  {
    using type = impl::simplify_t<direct_product<Ts...>, direct_product<U>>;
  };

  template<convex_space T, convex_space... Us>
  struct reduction<direct_product<T, composite_space<Us...>>>
  {
    using type = impl::simplify_t<direct_product<Us...>, direct_product<T>>;
  };  

  template<convex_space... Ts, convex_space... Us>
  struct reduction<direct_product<composite_space<Ts...>, composite_space<Us...>>>
  {
    using type = impl::simplify_t<direct_product<Ts...>, direct_product<Us...>>;
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
  struct reduced_validator<half_line_validator, half_line_validator>
  {
    using type = half_line_validator;
  };

  template<convex_space ValueSpace, validator_for<ValueSpace> Validator>
  inline constexpr bool has_consistent_validator{
    !affine_space<ValueSpace> || is_identity_validator_v<Validator>
  };

  template<convex_space ValueSpace>
  inline constexpr bool has_consistent_space{
    (!is_dual_v<ValueSpace>) || vector_space<ValueSpace> || (!affine_space<ValueSpace>)
  };
  
  template<
    convex_space ValueSpace,
    physical_unit Unit,
    basis_for<free_module_type_of_t<ValueSpace>> Basis,
    class Origin,
    validator_for<ValueSpace> Validator
  >
    requires    has_consistent_space<ValueSpace>
             && has_consistent_validator<ValueSpace, Validator>
  class physical_value;

  template<physical_unit Unit>
  struct unit_defined_origin{};

  template<affine_space T>
  struct implicit_affine_origin {};

  template<convex_space T>
    requires has_distinguished_origin_v<T>
  struct distinguished_origin {};

  template<convex_space ValueSpace, physical_unit Unit>
  struct to_origin_type;

  template<convex_space ValueSpace, physical_unit Unit>
    requires (!has_distinguished_origin_v<ValueSpace>) && (!affine_space<ValueSpace>)
  struct to_origin_type<ValueSpace, Unit>
  {
    using type = unit_defined_origin<Unit>;
  };

  template<convex_space ValueSpace, physical_unit Unit>
  using to_origin_type_t = to_origin_type<ValueSpace, Unit>::type;

  template<convex_space ValueSpace, physical_unit Unit>
    requires has_distinguished_origin_v<ValueSpace>
  struct to_origin_type<ValueSpace, Unit>
  {
    using type = distinguished_origin<ValueSpace>;
  };

  template<convex_space ValueSpace, physical_unit Unit>
    requires (!has_distinguished_origin_v<ValueSpace> && affine_space<ValueSpace>)
  struct to_origin_type<ValueSpace, Unit>
  {
    using type = implicit_affine_origin<ValueSpace>;
  };
  
  template<convex_space ValueSpace, physical_unit Unit, basis_for<free_module_type_of_t<ValueSpace>> Basis, class Validator>
  using to_coordinates_base_type
    = coordinates_base<
        ValueSpace,
        Basis,
        Validator,
        physical_value<free_module_type_of_t<ValueSpace>, Unit, Basis, distinguished_origin<free_module_type_of_t<ValueSpace>>, std::identity>>;

  template<class T>
  inline constexpr bool has_base_space_v{
    requires { typename T::base_space; }
  };

  template<convex_space T>
  struct to_base_space
  {
    using type = T;
  };

  template<convex_space T>
  using to_base_space_t = to_base_space<T>::type;

  template<convex_space T>
    requires has_base_space_v<T>
  struct to_base_space<T>
  {
    using type = T::base_space;
  };

  template<convex_space T>
  struct to_base_space<dual<T>>
  {
    using type = dual<T>;
  };

  template<convex_space T>
    requires has_base_space_v<T>
  struct to_base_space<dual<T>>
  {
    using type = dual<typename T::base_space>;
  };

  template<convex_space... Ts>
  struct to_base_space<composite_space<Ts...>>
  {
    using sorted_direct_product_t = meta::stable_sort_t<direct_product<to_base_space_t<Ts>...>,  meta::type_comparator>;
    using type = impl::to_composite_space_t<reduction<impl::reduce_t<impl::count_and_combine_t<sorted_direct_product_t>>>>;
  };

  template<convex_space T, convex_space U>
  inline constexpr bool have_compatible_base_spaces_v{
    has_base_space_v<T> && has_base_space_v<U> && (free_module_type_of_t<T>::dimension == free_module_type_of_t<U>::dimension) && requires {
      typename std::common_type<typename T::base_space, typename U::base_space>::type;
    }
  };

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

  template<class Basis1, class Basis2>
  struct consistent_bases : std::false_type {};

  template<class Basis1, class Basis2>
  inline constexpr bool consistent_bases_v{consistent_bases<Basis1, Basis2>::value};

  template<free_module M1, free_module M2>
  struct consistent_bases<canonical_right_handed_basis<M1>, canonical_right_handed_basis<M2>> : std::true_type
  {
    template<free_module M>
    using rebind_type = canonical_right_handed_basis<M>;
  };

  template<class T, class U>
  struct physical_value_product;

  template<class T, class U>
  using physical_value_product_t = physical_value_product<T, U>::type;

  template<convex_space C>
  inline constexpr bool is_1d_euclidean_v{
       std::is_same_v<euclidean_vector_space<commutative_ring_type_of_t<C>, 1, arena_type_of_t<C>>, C>
    || std::is_same_v<euclidean_half_space<commutative_ring_type_of_t<C>, arena_type_of_t<C>>, C>
  };
  
  template<
    convex_space LHSValueSpace, physical_unit LHSUnit, basis_for<free_module_type_of_t<LHSValueSpace>> LHSBasis, class LHSValidator,
    convex_space RHSValueSpace, physical_unit RHSUnit, basis_for<free_module_type_of_t<RHSValueSpace>> RHSBasis, class RHSValidator
  >
    requires consistent_bases_v<LHSBasis, RHSBasis> && (!is_1d_euclidean_v<LHSValueSpace> && !is_1d_euclidean_v<RHSValueSpace>)
  struct physical_value_product<physical_value<LHSValueSpace, LHSUnit, LHSBasis, distinguished_origin<LHSValueSpace>, LHSValidator>,
                                physical_value<RHSValueSpace, RHSUnit, RHSBasis, distinguished_origin<RHSValueSpace>, RHSValidator>>
  {
    using value_space_type = impl::to_composite_space_t<reduction_t<direct_product<LHSValueSpace, RHSValueSpace>>>;
    using type
      = physical_value<
          value_space_type,
          impl::to_composite_space_t<reduction_t<direct_product<LHSUnit, RHSUnit>>>,
          typename consistent_bases<LHSBasis, RHSBasis>::template rebind_type<free_module_type_of_t<value_space_type>>,
          distinguished_origin<value_space_type>,
          reduced_validator_t<LHSValidator, RHSValidator>
        >;
  };

  template<
    convex_space LHSValueSpace, physical_unit LHSUnit, basis_for<free_module_type_of_t<LHSValueSpace>> LHSBasis, class LHSValidator,
    convex_space RHSValueSpace, physical_unit RHSUnit, basis_for<free_module_type_of_t<RHSValueSpace>> RHSBasis, class RHSValidator
  >
    requires  consistent_bases_v<LHSBasis, RHSBasis> && is_1d_euclidean_v<LHSValueSpace>
  struct physical_value_product<physical_value<LHSValueSpace, LHSUnit, LHSBasis, distinguished_origin<LHSValueSpace>, LHSValidator>,
                                physical_value<RHSValueSpace, RHSUnit, RHSBasis, distinguished_origin<RHSValueSpace>, RHSValidator>>
  {
    using type
      = physical_value<
          RHSValueSpace,
          RHSUnit,
          RHSBasis,
          distinguished_origin<RHSValueSpace>,
          reduced_validator_t<LHSValidator, RHSValidator>
        >;
  };

  template<
    convex_space LHSValueSpace, physical_unit LHSUnit, basis_for<free_module_type_of_t<LHSValueSpace>> LHSBasis, class LHSValidator,
    convex_space RHSValueSpace, physical_unit RHSUnit, basis_for<free_module_type_of_t<RHSValueSpace>> RHSBasis, class RHSValidator
  >
    requires consistent_bases_v<LHSBasis, RHSBasis> && is_1d_euclidean_v<RHSValueSpace>
  struct physical_value_product<physical_value<LHSValueSpace, LHSUnit, LHSBasis, distinguished_origin<LHSValueSpace>, LHSValidator>,
                                physical_value<RHSValueSpace, RHSUnit, RHSBasis, distinguished_origin<RHSValueSpace>, RHSValidator>>
  {
    using type
      = physical_value<
          LHSValueSpace,
          LHSUnit,
          LHSBasis,
          distinguished_origin<LHSValueSpace>,
          reduced_validator_t<LHSValidator, RHSValidator>
        >;
  };

  template<class From, class To>
  inline constexpr bool has_quantity_conversion_v{
    has_coordinate_transformation_v<From, To> && std::constructible_from<coordinate_transformation<From, To>>
  };

  template<convex_space C, physical_unit FromUnit, physical_unit ToUnit>
  struct conversion_space
  {
    using type = C;
  };

  template<convex_space C, physical_unit FromUnit, physical_unit ToUnit>
  using conversion_space_t = conversion_space<C, FromUnit, ToUnit>::type;

  template<convex_space ValueSpace, physical_unit Unit>
  using default_validator_t = std::conditional_t<affine_space<ValueSpace>, std::identity, typename Unit::validator_type>;

  template<
    convex_space ValueSpace,
    physical_unit Unit,
    basis_for<free_module_type_of_t<ValueSpace>> Basis = canonical_right_handed_basis<free_module_type_of_t<ValueSpace>>,
    class Origin                                       = to_origin_type_t<ValueSpace, Unit>,
    validator_for<ValueSpace> Validator                = default_validator_t<ValueSpace, Unit>
  >
    requires    has_consistent_space<ValueSpace>
             && has_consistent_validator<ValueSpace, Validator>
  class physical_value final : public to_coordinates_base_type<ValueSpace, Unit, Basis, Validator>
  {
  public:
    using coordinates_type         = to_coordinates_base_type<ValueSpace, Unit, Basis, Validator>;
    using space_type               = ValueSpace;
    using units_type               = Unit;
    using basis_type               = Basis;
    using origin_type              = Origin;
    using displacement_space_type  = free_module_type_of_t<ValueSpace>;
    using intrinsic_validator_type = Unit::validator_type;
    using validator_type           = Validator;
    using ring_type                = commutative_ring_type_of_t<ValueSpace>;
    using value_type               = ring_type;
    using displacement_type        = coordinates_type::displacement_coordinates_type;

    constexpr static std::size_t dimension{displacement_space_type::dimension};
    constexpr static std::size_t D{dimension};

    constexpr static bool is_intrinsically_absolute{
      (D == 1) && !affine_space<space_type> && defines_half_line_v<intrinsic_validator_type>
    };

    constexpr static bool is_effectively_absolute{is_intrinsically_absolute && std::is_same_v<Validator, intrinsic_validator_type>};
    constexpr static bool has_identity_validator{coordinates_type::has_identity_validator};

    template<convex_space RHSValueSpace, class RHSUnit, class RHSBasis, class RHSOrigin, class RHSValidator>
    constexpr static bool is_composable_with{
         consistent_bases_v<basis_type, RHSBasis>
      && (is_intrinsically_absolute || vector_space<space_type>)
      && (physical_value<RHSValueSpace, RHSUnit, RHSBasis, RHSOrigin, RHSValidator>::is_intrinsically_absolute || vector_space<RHSValueSpace>)
    };

    template<convex_space RHSValueSpace, class RHSUnit, class RHSBasis, class RHSOrigin, class RHSValidator>
    constexpr static bool is_multipicable_with{
         is_composable_with<RHSValueSpace, RHSUnit, RHSBasis, RHSOrigin, RHSValidator>
      && ((D == 1) || (physical_value<RHSValueSpace, RHSUnit, RHSBasis, RHSOrigin, RHSValidator>::D == 1))
    };

    template<convex_space RHSValueSpace, class RHSUnit, class RHSBasis, class RHSOrigin, class RHSValidator>
    constexpr static bool is_divisible_with{
         weak_field<ring_type>
      && weak_field<commutative_ring_type_of_t<RHSValueSpace>>
      && is_composable_with<RHSValueSpace, RHSUnit, RHSBasis, RHSOrigin, RHSValidator>
      && (physical_value<RHSValueSpace, RHSUnit, RHSBasis, RHSOrigin, RHSValidator>::D == 1)
    };

    constexpr physical_value() = default;

    constexpr physical_value(value_type val, units_type) requires (D == 1)
      : coordinates_type{val}
    {}

    constexpr physical_value(std::span<const value_type, D> val, units_type)
      : coordinates_type{val}
    {}

    [[nodiscard]]
    constexpr physical_value operator-() const noexcept(has_identity_validator)
      requires (coordinates_type::has_distinguished_origin) && (!std::is_unsigned_v<ring_type>) && (!is_effectively_absolute)
    {
      return physical_value{utilities::to_array(this->values(), [](value_type t) { return -t; }), units_type{}};
    }

    using coordinates_type::operator+=;
    
    template<convex_space OtherValueSpace, basis_for<free_module_type_of_t<OtherValueSpace>> OtherBasis, class OtherOrigin>
      requires is_intrinsically_absolute && (std::is_base_of_v<ValueSpace, OtherValueSpace>) && consistent_bases_v<basis_type, OtherBasis>
    constexpr physical_value& operator+=(const physical_value<OtherValueSpace, Unit, OtherBasis, OtherOrigin, Validator>& other) noexcept(has_identity_validator)
    {
      return *this = (*this + other);
    }

    template<convex_space OtherValueSpace, basis_for<free_module_type_of_t<OtherValueSpace>> OtherBasis, class OtherOrigin>
      requires is_intrinsically_absolute && (!std::is_same_v<ValueSpace, OtherValueSpace>) && have_compatible_base_spaces_v<ValueSpace, OtherValueSpace> && consistent_bases_v<basis_type, OtherBasis>
    [[nodiscard]]
    friend constexpr auto operator+(const physical_value& lhs, const physical_value<OtherValueSpace, Unit, OtherBasis, OtherOrigin, Validator>& rhs)
    {
      using value_space_t = std::common_type_t<typename ValueSpace::base_space, typename OtherValueSpace::base_space>;
      using physical_value_t
        = physical_value<value_space_t, Unit, canonical_right_handed_basis<free_module_type_of_t<value_space_t>>, to_origin_type_t<value_space_t, Unit>, Validator>;

      return [&] <std::size_t... Is>(std::index_sequence<Is...>) {
        return physical_value_t{std::array{(lhs.values()[Is] + rhs.values()[Is])...}, units_type{}};
      }(std::make_index_sequence<D>{});
    }

    template<class OtherValueSpace, basis_for<free_module_type_of_t<OtherValueSpace>> OtherBasis, class OtherOrigin>
    requires   (!std::is_same_v<OtherValueSpace, displacement_space_type>)
            && (std::is_same_v<ValueSpace, OtherValueSpace> || have_compatible_base_spaces_v<ValueSpace, OtherValueSpace>) && consistent_bases_v<basis_type, OtherBasis>
    [[nodiscard]]
    friend constexpr auto operator-(const physical_value& lhs, const physical_value<OtherValueSpace, Unit, OtherBasis, OtherOrigin, Validator>& rhs)
      noexcept(has_identity_validator)
    {
      using disp_space_t = to_displacement_space_t<ValueSpace, OtherValueSpace>;
      using basis_t = canonical_right_handed_basis<free_module_type_of_t<disp_space_t>>;
      using disp_t = to_coordinates_base_type<disp_space_t, Unit, basis_t, Validator>::displacement_coordinates_type;
      return[&] <std::size_t... Is>(std::index_sequence<Is...>) {
        return disp_t{std::array{(lhs.values()[Is] - rhs.values()[Is])...}, units_type{}};
      }(std::make_index_sequence<D>{});
    }

    template<convex_space RHSValueSpace, physical_unit RHSUnit, basis_for<free_module_type_of_t<RHSValueSpace>> RHSBasis, class RHSOrigin, class RHSValidator>
      requires is_multipicable_with<RHSValueSpace, RHSUnit, RHSBasis, RHSOrigin, RHSValidator>
    [[nodiscard]]
    friend constexpr auto operator*(const physical_value& lhs,
                                    const physical_value<RHSValueSpace, RHSUnit, RHSBasis, RHSOrigin, RHSValidator>& rhs)
    {
      using physical_value_t
        = physical_value_product_t<
            physical_value,
            physical_value<RHSValueSpace,RHSUnit, RHSBasis, RHSOrigin, RHSValidator>
          >;

      using derived_units_type = physical_value_t::units_type;
      return physical_value_t{lhs.value() * rhs.value(), derived_units_type{}};
    }

    template<convex_space RHSValueSpace, class RHSUnit,  basis_for<free_module_type_of_t<RHSValueSpace>> RHSBasis, class RHSOrigin, class RHSValidator>
      requires is_divisible_with<RHSValueSpace, RHSUnit, RHSBasis, RHSOrigin, RHSValidator>
    [[nodiscard]]
    friend constexpr auto operator/(const physical_value& lhs,
                                    const physical_value<RHSValueSpace, RHSUnit, RHSBasis, RHSOrigin, RHSValidator>& rhs)
    {
      using physical_value_t
        = physical_value_product_t<
            physical_value,
            physical_value<dual_of_t<RHSValueSpace>, dual_of_t<RHSUnit>, dual_of_t<RHSBasis>, distinguished_origin<dual_of_t<RHSValueSpace>>, RHSValidator>
          >;
      using derived_units_type = physical_value_t::units_type;
      if constexpr(dimension == 1)
      {
        return physical_value_t{lhs.value() / rhs.value(), derived_units_type{}};
      }
      else
      {
        return[&] <std::size_t... Is>(std::index_sequence<Is...>) {
          return physical_value_t{std::array{(lhs.values()[Is] / rhs.value())...}, derived_units_type{}};
        }(std::make_index_sequence<D>{});
      }
    }

    [[nodiscard]] friend constexpr auto operator/(value_type value, const physical_value& rhs)
      requires ((D == 1) && (is_intrinsically_absolute || vector_space<ValueSpace>))
    {
      using physical_value_t = physical_value<dual_of_t<ValueSpace>, dual_of_t<Unit>, dual_of_t<basis_type>, distinguished_origin<dual_of_t<ValueSpace>>, validator_type>;
      using derived_units_type = physical_value_t::units_type;
      return physical_value_t{value / rhs.value(), derived_units_type{}};
    }

    // Reduce *everything* to its base space on both sides of the equation; if these are the same, allow the conversion 
    template<class LoweredValueSpace, class OtherUnit, basis_for<free_module_type_of_t<LoweredValueSpace>> OtherBasis, class OtherOrigin>    
      requires std::same_as<to_base_space_t<space_type>, to_base_space_t<LoweredValueSpace>> && consistent_bases_v<basis_type, OtherBasis>
    [[nodiscard]]
    constexpr operator physical_value<LoweredValueSpace, OtherUnit, OtherBasis, OtherOrigin, validator_type>() const noexcept
    {
      using value_space_t    = to_base_space_t<LoweredValueSpace>;
      using basis_t          = canonical_right_handed_basis<free_module_type_of_t<value_space_t>>;
      using physical_value_t = physical_value<to_base_space_t<LoweredValueSpace>, Unit, basis_t, to_origin_type_t<value_space_t, Unit>, validator_type>;
      if constexpr(dimension == 1)
      {
        return physical_value_t{this->value(), OtherUnit{}};
      }
      else
      {
        return [this] <std::size_t... Is>(std::index_sequence<Is...>) {
          return physical_value_t{std::array{this->values()[Is]...}, OtherUnit{}};
        }(std::make_index_sequence<D>{});
      }
    }

    template<
      physical_unit OtherUnit,
      convex_space OtherSpace=conversion_space_t<ValueSpace, Unit, OtherUnit>,
      basis_for<free_module_type_of_t<OtherSpace>> OtherBasis = canonical_right_handed_basis<free_module_type_of_t<OtherSpace>>,
      class OtherOrigin                                       = to_origin_type_t<OtherSpace, OtherUnit>,
      validator_for<OtherSpace> OtherValidator                = default_validator_t<OtherSpace, OtherUnit>
    >
      requires has_quantity_conversion_v<physical_value, physical_value<OtherSpace, OtherUnit, OtherBasis, OtherOrigin, OtherValidator>>
    [[nodiscard]]
    constexpr physical_value<OtherSpace, OtherUnit, OtherBasis, OtherOrigin, OtherValidator> convert_to(OtherUnit) const
      noexcept(has_noexcept_coordinate_transformation_v<physical_value, physical_value<OtherSpace, OtherUnit, OtherBasis, OtherOrigin, OtherValidator>>)
    {
      return coordinate_transformation<physical_value, physical_value<OtherSpace, OtherUnit, OtherBasis, OtherOrigin, OtherValidator>>{}(*this);
    }

    [[nodiscard]]
    constexpr physical_value convert_to(Unit) const noexcept { return *this; } 
  };

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

  template<class T, physical_unit U>
    requires has_default_space_v<U, T>
  physical_value(T, U) -> physical_value<default_space_t<U, T>, U>;

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

    template<class Arena>
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
    };
  }
  
  template<class Space>
  struct associated_displacement_space
  {
    constexpr static std::size_t dimension{Space::dimension};
    using set_type              = sets::classical::differences<typename Space::set_type>;
    using commutative_ring_type = Space::representation_type;
    using is_free_module        = std::true_type;
    using arena_type            = Space::arena_type;
  };

  template<class Space>
      requires has_base_space_v<Space>
  struct associated_displacement_space<Space>
  {
    constexpr static std::size_t dimension{Space::dimension};
    using set_type              = sets::classical::differences<typename Space::set_type>;
    using commutative_ring_type = Space::representation_type;
    using is_free_module        = std::true_type;
    using base_space            = associated_displacement_space<typename Space::base_space>;
    using arena_type            = Space::arena_type;
  };

  template<class PhysicalValueSet, arithmetic Rep, std::size_t D, class Derived>
  struct physical_value_convex_space
  {
    constexpr static std::size_t dimension{D};
    using set_type            = PhysicalValueSet;
    using representation_type = Rep;
    using free_module_type    = associated_displacement_space<Derived>;
    using is_convex_space     = std::true_type;
    using arena_type          = PhysicalValueSet::arena_type;
  };

  template<class PhysicalValueSet, arithmetic Rep, std::size_t D, class Derived>
  struct physical_value_affine_space
  {
    constexpr static std::size_t dimension{D};
    using set_type            = PhysicalValueSet;
    using representation_type = Rep;
    using free_module_type    = associated_displacement_space<Derived>;
    using is_affine_space     = std::true_type;
    using arena_type          = PhysicalValueSet::arena_type;
  };

  template<class PhysicalValueSet, arithmetic Rep, std::size_t D, class Derived>
  struct physical_value_vector_space
  {
    constexpr static std::size_t dimension{D};
    using set_type            = PhysicalValueSet;
    using representation_type = Rep;
    using field_type          = Rep;
    using is_vector_space     = std::true_type;
  };

  template<std::floating_point Rep, class Arena>
  struct mass_space
    : physical_value_convex_space<sets::classical::masses<Arena>, Rep, 1, mass_space<Rep, Arena>>
  {
    using arena_type           = Arena;
    using base_space           = mass_space;
    using distinguished_origin = std::true_type;
  };

  template<std::floating_point Rep, class Arena>
  struct absolute_temperature_space
    : physical_value_convex_space<sets::classical::temperatures<Arena>, Rep, 1, absolute_temperature_space<Rep, Arena>>
  {
    using arena_type           = Arena;
    using base_space           = absolute_temperature_space;
    using distinguished_origin = std::true_type;
  };

  template<convex_space C>
    requires has_distinguished_origin_v<C>
  struct relaxed_space : C
  {
    using base_space          = relaxed_space<typename C::base_space>;
    using free_module_type    = associated_displacement_space<relaxed_space>;
    using distinguished_origin = std::false_type;
  };

  template<std::floating_point Rep, class Arena>
  using temperature_space = relaxed_space<absolute_temperature_space<Rep, Arena>>;
  
  template<std::floating_point Rep, class Arena>
  struct electrical_current_space
    : physical_value_vector_space<sets::classical::electrical_currents<Arena>, Rep, 1, electrical_current_space<Rep, Arena>>
  {
    using arena_type = Arena;
    using base_space = electrical_current_space;
  };

  template<std::floating_point Rep, class Arena>
  struct angular_space : physical_value_vector_space<sets::classical::angles<Arena>, Rep, 1, angular_space<Rep, Arena>>
  {
    using arena_type = Arena;
    using base_space = angular_space;
  };

  template<arithmetic Rep, class Arena>
  struct length_space
    : physical_value_convex_space<sets::classical::lengths<Arena>, Rep, 1, length_space<Rep, Arena>>
  {
    using arena_type           = Arena;
    using base_space           = length_space;
    using distinguished_origin = std::true_type;
  };

  template<arithmetic Rep, class Arena>
  struct width_space : length_space<Rep, Arena>
  {
    struct free_module_type : associated_displacement_space<width_space<Rep, Arena>> {};
  };

  template<arithmetic Rep, class Arena>
  struct height_space : length_space<Rep, Arena>
  {
    struct free_module_type : associated_displacement_space<height_space<Rep, Arena>> {};
  };

  template<arithmetic Rep, class Arena>
  struct time_interval_space
    : physical_value_convex_space<sets::classical::time_intervals<Arena>, Rep, 1, time_interval_space<Rep, Arena>>
  {
    using arena_type = Arena;
    using distinguished_origin = std::true_type;
  };
  
  template<arithmetic Rep, class Arena>
  struct time_space : physical_value_affine_space<sets::classical::times<Arena>, Rep, 1, time_space<Rep, Arena>>
  {
    using arena_type = Arena;
  };

  template<arithmetic Rep, std::size_t D, class Arena>
  struct position_space : physical_value_affine_space<sets::classical::positions<Arena>, Rep, D, position_space<Rep, D, Arena>>
  {
    using arena_type = Arena;
  };
  
  struct implicit_common_arena {};

  template<physical_unit U>
  inline constexpr bool has_symbol_v{
    requires {
      { U::symbol } -> std::convertible_to<std::string_view>; }
  };

  template<class Validator>
  struct scale_invariant_validator : std::false_type {};

  template<>
  struct scale_invariant_validator<std::identity> : std::true_type {};

  template<>
  struct scale_invariant_validator<half_line_validator> : std::true_type {};

  template<class Validator>
  using scale_invariant_validator_t = scale_invariant_validator<Validator>::type;

  template<class Validator>
  inline constexpr bool scale_invariant_validator_v{scale_invariant_validator<Validator>::value};

  template<class Validator>
  struct translation_invariant_validator : std::false_type {};

  template<>
  struct translation_invariant_validator<std::identity> : std::true_type {};

  template<class Validator>
  using translation_invariant_validator_t = translation_invariant_validator<Validator>::type;

  template<class Validator>
  inline constexpr bool translation_invariant_validator_v{translation_invariant_validator<Validator>::value};

  //====== Temporary home for some (hacky) ratio stuff ======//
  template<auto Num, auto Den>
  struct ratio;

  template<long double Num, long double Den>
  struct ratio<Num, Den>
  {
    constexpr static auto num{Num};
    constexpr static auto den{Den};
  };

  template<long double Num, std::intmax_t Den>
  struct ratio<Num, Den>
  {
    constexpr static auto num{Num};
    constexpr static auto den{Den};
  };

  template<std::intmax_t Num, long double Den>
  struct ratio<Num, Den>
  {
    constexpr static auto num{Num};
    constexpr static auto den{Den};
  };

  template<std::intmax_t Num, intmax_t Den>
  struct ratio<Num, Den> : std::ratio<Num, Den>
  {
  };

  template<class T>
  struct is_ratio : std::false_type {};

  template<class T>
  using is_ratio_t = is_ratio<T>::type;

  template<class T>
  inline constexpr bool is_ratio_v{is_ratio<T>::value};

  template<std::intmax_t Num, std::intmax_t Den>
  struct is_ratio<std::ratio<Num, Den>> : std::true_type {};

  template<auto Num, auto Den>
  struct is_ratio<ratio<Num, Den>> : std::true_type {};

  template<class>
  struct inverse;

  template<class T>
  using inverse_t = inverse<T>::type;

  template<auto Num, auto Den>
  struct inverse<ratio<Num, Den>>
  {
    using type = ratio<Den, Num>;
  };

  template<auto Num, auto Den>
  struct inverse<std::ratio<Num, Den>>
  {
    using type = std::ratio<Den, Num>;
  };
  

  template<class...>
  struct product;

  template<class T, class U>
  using product_t = product<T, U>::type;

  template<auto Num1, auto Den1>
  struct product<ratio<Num1, Den1>>
  {
    using type = ratio<Num1, Den1>;
  };

  template<std::intmax_t Num1, std::intmax_t Den1>
  struct product<std::ratio<Num1, Den1>>
  {
    using type = std::ratio<Num1, Den1>;
  };

  template<auto Num1, auto Den1, auto Num2, auto Den2>
  struct product<ratio<Num1, Den1>, ratio<Num2, Den2>>
  {
    using type = ratio<Num1 * Num2, Den1 * Den2>;
  };

  template<auto Num1, auto Den1, std::intmax_t Num2, std::intmax_t Den2>
    requires(std::integral<decltype(Den1)>)
  struct product<ratio<Num1, Den1>, std::ratio<Num2, Den2>>
  {
    using reduced_ratio_type = std::ratio<Num2, Den1 * Den2>;
    using type = ratio<Num1 * reduced_ratio_type::num, reduced_ratio_type::den>;
  };

  template<auto Num1, auto Den1, std::intmax_t Num2, std::intmax_t Den2>
    requires(std::integral<decltype(Num1)>)
  struct product<ratio<Num1, Den1>, std::ratio<Num2, Den2>>
  {
    using reduced_ratio_type = std::ratio<Num1 * Num2, Den2>;
    using type = ratio<reduced_ratio_type::num, reduced_ratio_type::den * Den1>;
  };

  template<intmax_t Num1, intmax_t Den1, auto Num2, auto Den2>
    requires(std::integral<decltype(Den2)>)
  struct product<std::ratio<Num1, Den1>, ratio<Num2, Den2>>
  {
    using reduced_ratio_type = std::ratio<Num1, Den1 * Den2>;
    using type = ratio<Num2 * reduced_ratio_type::num, reduced_ratio_type::den>;
  };

  template<intmax_t Num1, intmax_t Den1, auto Num2, auto Den2>
    requires(std::integral<decltype(Num2)>)
  struct product<std::ratio<Num1, Den1>, ratio<Num2, Den2>>
  {
    using reduced_ratio_type = std::ratio<Num1 * Num2, Den1>;
    using type = ratio<reduced_ratio_type::num, reduced_ratio_type::den * Den2>;
  };

  template<auto Num1, auto Den1, std::intmax_t Num2, std::intmax_t Den2>
  struct product<ratio<Num1, Den1>, std::ratio<Num2, Den2>> : product<ratio<Num1, Den1>, ratio<Num2, Den2>>
  {};

  template<std::intmax_t Num1, std::intmax_t Den1, auto Num2, auto Den2>
  struct product<std::ratio<Num1, Den1>, ratio<Num2, Den2>> : product<ratio<Num1, Den1>, ratio<Num2, Den2>>
  {};

  template<std::intmax_t Num1, std::intmax_t Den1, std::intmax_t Num2, std::intmax_t Den2>
  struct product<std::ratio<Num1, Den1>, std::ratio<Num2, Den2>>
  {
    using type = std::ratio_multiply<std::ratio<Num1, Den1>, std::ratio<Num2, Den2>>;
  };

  //====== End of temporary home for some (hacky) ratio stuff ======//

  template<class T>
  struct dilatation;

  template<auto Num, auto Den>
  struct dilatation<ratio<Num, Den>>
  {
    using ratio_type = ratio<Num, Den>;
  };

  template<std::intmax_t Num, std::intmax_t Den>
  struct dilatation<std::ratio<Num, Den>>
  {
    using ratio_type = std::ratio<Num, Den>;
  };

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
    requires arithmetic<std::remove_const_t<decltype(Displacement)>>
  struct translation
  {
    using displacement_type = std::remove_const_t<decltype(Displacement)>;
    constexpr static auto displacement{Displacement};
  };

  template<auto Displacement>
  struct inverse<translation<Displacement>>
  {
    using type = translation<-Displacement>;
  };

  template<class...>
  struct coordinate_transform;

  template<physical_unit U, class Ratio, auto Displacement>
  struct coordinate_transform<U, dilatation<Ratio>, translation<Displacement>>
  {
    using validator_type = U::validator_type;// TO DO: Sort this out!
    using transform_type = coordinate_transform<U, dilatation<Ratio>, translation<Displacement>>; // And this?
    using with_respect_to_type = U;
    using dilatation_type      = dilatation<Ratio>;
    using translation_type     = translation<Displacement>;
  };

  template<physical_unit U, class Ratio, auto Displacement>
  struct inverse<coordinate_transform<U, dilatation<Ratio>, translation<Displacement>>>
  {
    using validator_type = U::validator_type;// Sort this out!
    using inverse_dil_type   = inverse_t<dilatation<Ratio>>;
    using inverse_ratio_type = inverse_dil_type::ratio_type;
    using type = coordinate_transform<U, inverse_dil_type, inverse_t<translation<Displacement * inverse_ratio_type::num / inverse_ratio_type::den>>>;
  };

  template<
    physical_unit LHSUnit, class LHSRatio, auto LHSDisplacement,
    physical_unit RHSUnit, class RHSRatio, auto RHSDisplacement
  >
  struct product<coordinate_transform<LHSUnit, dilatation<LHSRatio>, translation<LHSDisplacement>>,
                 coordinate_transform<RHSUnit, dilatation<RHSRatio>, translation<RHSDisplacement>>>
  {
    using dilatation_type  = dilatation<product_t<LHSRatio, RHSRatio>>;
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
    using unit_type  = U;
  };

  template<physical_unit U>
  using root_transform_t = root_transform<U>::transform_type;

  template<physical_unit U>
  using root_transform_unit_t = root_transform<U>::unit_type;

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
    using validator_type = Unit::validator_type;
    using transform_type = coordinate_transform<Unit, dilatation<std::mega>, translation<0>>;
  };
  
  template<physical_unit Unit>
  struct milli : coordinate_transform<Unit, dilatation<std::kilo>, translation<0>>
  {
    using validator_type = Unit::validator_type;
    using transform_type = coordinate_transform<Unit, dilatation<std::kilo>, translation<0>>;
  };

  template<physical_unit Unit>
  struct kilo : coordinate_transform<Unit, dilatation<std::milli>, translation<0>>
  {
    using validator_type = Unit::validator_type;
    using transform_type = coordinate_transform<Unit, dilatation<std::milli>, translation<0>>;
  };

  template<physical_unit Unit>
  struct mega : coordinate_transform<Unit, dilatation<std::micro>, translation<0>>
  {
    using validator_type = Unit::validator_type;
    using transform_type = coordinate_transform<Unit, dilatation<std::micro>, translation<0>>;
  };
  
  namespace si
  {
    namespace units
    {
      struct ampere_t
      {
        using validator_type = std::identity;
        constexpr static std::string_view symbol{"A"};
      };
    
      struct kilogram_t
      {
        using validator_type = half_line_validator;
        constexpr static std::string_view symbol{"kg"};
      };

      struct metre_t
      {
        using validator_type = half_line_validator;
        constexpr static std::string_view symbol{"m"};
      };

      struct second_t
      {
        using validator_type = half_line_validator;
        constexpr static std::string_view symbol{"s"};
      };

      struct kelvin_t
      {
        using validator_type = half_line_validator;
        constexpr static std::string_view symbol{"K"};
      };

      struct coulomb_t
      {
        using validator_type = std::identity;
        constexpr static std::string_view symbol{"C"};
      };

      struct radian_t
      {
        using validator_type = std::identity;
        constexpr static std::string_view symbol{"rad"};
      };

      struct celsius_t : coordinate_transform<kelvin_t, dilatation<std::ratio<1, 1>>, translation<-273.15L>>
      {
        using translation_type = translation<-273.15L>;

        constexpr static std::string_view symbol{"degC"};
        
        struct validator
        {
          template<std::floating_point T>
          [[nodiscard]]
          constexpr T operator()(const T val) const
          {
            constexpr auto absZero{static_cast<T>(translation_type::displacement)};
            if(val < absZero)
              throw std::domain_error{std::format("Value {} less than {} {}", val, absZero, symbol)};

            return val;
          }
        };

        using validator_type = validator;
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

    template<std::floating_point T, class Arena=implicit_common_arena>
    using mass = physical_value<mass_space<T, Arena>, units::kilogram_t>;

    template<std::floating_point T, class Arena=implicit_common_arena>
    using length = physical_value<length_space<T, Arena>, units::metre_t>;

    template<std::floating_point T, class Arena=implicit_common_arena>
    using time_interval = physical_value<time_interval_space<T, Arena>, units::second_t>;

    template<std::floating_point T, class Arena=implicit_common_arena>
    using temperature = physical_value<absolute_temperature_space<T, Arena>, units::kelvin_t>;

    template<std::floating_point T, class Arena=implicit_common_arena>
    using temperature_celsius = physical_value<temperature_space<T, Arena>, units::celsius_t>;

    template<std::floating_point T, class Arena=implicit_common_arena>
    using electrical_current = physical_value<electrical_current_space<T, Arena>, units::ampere_t>;

    template<std::floating_point T, class Arena=implicit_common_arena>
    using angle = physical_value<angular_space<T, Arena>, units::radian_t>;

    template<std::floating_point T, class Arena=implicit_common_arena>
    using width = physical_value<width_space<T, Arena>, units::metre_t>;

    template<std::floating_point T, class Arena=implicit_common_arena>
    using height = physical_value<height_space<T, Arena>, units::metre_t>;

    template<
      std::floating_point T,
      class Arena=implicit_common_arena,
      class Origin=implicit_affine_origin<time_space<T, Arena>>
    >
    using time = physical_value<time_space<T, Arena>, units::second_t, canonical_right_handed_basis<free_module_type_of_t<time_space<T, Arena>>>, Origin, std::identity>;

    template<      
      std::floating_point T,
      std::size_t D,
      class Arena  = implicit_common_arena,      
      basis_for<free_module_type_of_t<position_space<T, D, Arena>>> Basis = canonical_right_handed_basis<free_module_type_of_t<position_space<T, D, Arena>>>,
      class Origin = implicit_affine_origin<position_space<T, D, Arena>>
    >
    using position = physical_value<position_space<T, D, Arena>, units::metre_t, Basis, Origin, std::identity>;
  }

  namespace non_si
  {
    namespace units
    {
      struct degree_t : coordinate_transform<si::units::radian_t, dilatation<ratio<intmax_t{180}, std::numbers::pi_v<long double>>>, translation<0>>
      {
        using validator_type = std::identity;
        constexpr static std::string_view symbol{"deg"};
      };

      struct gradian_t : coordinate_transform<si::units::radian_t, dilatation<ratio<intmax_t{200}, std::numbers::pi_v<long double>>>, translation<0>>
      {
        using validator_type = std::identity;
        constexpr static std::string_view symbol{"gon"};
      };

      inline constexpr degree_t degree{};
      inline constexpr gradian_t gradian{};

      struct farenheight_t
        : coordinate_transform<si::units::celsius_t, dilatation<std::ratio<9, 5>>, translation<32.0l>>
      {        
        constexpr static std::string_view symbol{"degF"};

        struct validator
        {
          template<std::floating_point T>
          [[nodiscard]]
          constexpr T operator()(const T val) const
          {
            constexpr auto absZero{static_cast<T>(-273.15L / 5 * 9 + 32)};
            if(val < absZero)
              throw std::domain_error{std::format("Value {} less than {} {}", val, absZero, symbol)};

            return val;
          }
        };

        using validator_type = validator;
      };

      inline constexpr farenheight_t farenheight{};
    }

    template<std::floating_point T, class Arena=implicit_common_arena>
    using temperature_farenheight = physical_value<temperature_space<T, Arena>, units::farenheight_t>;
  }

  template<convex_space C, physical_unit FromUnit, physical_unit ToUnit>
  struct conversion_space<associated_displacement_space<C>, FromUnit, ToUnit>
  {
    using type = associated_displacement_space<conversion_space_t<C, FromUnit, ToUnit>>;
  };

  template<physical_unit Unit, class Rep, class Ratio>
    requires has_default_space_v<Unit, Rep>
  struct default_space<coordinate_transform<Unit, dilatation<Ratio>, translation<0>>, Rep> : default_space<Unit, Rep> {};

  template<physical_unit Unit, class Rep>
    requires has_coordinate_transform_v<Unit>
  struct default_space<Unit, Rep> : default_space<root_transform_t<Unit>, Rep> {};

  template<std::floating_point T>
  struct default_space<si::units::kilogram_t, T>
  {
    using type = mass_space<T, implicit_common_arena>;
  };

  template<std::floating_point T>
  struct default_space<si::units::radian_t, T>
  {
    using type = angular_space<T, implicit_common_arena>;
  };
  
  template<vector_space ValueSpace, physical_unit Unit, class Basis, class Origin, validator_for<ValueSpace> Validator>
    requires (dimension_of<ValueSpace> == 1)
  [[nodiscard]]
  constexpr physical_value<ValueSpace, Unit, Basis, Origin, Validator> abs(physical_value<ValueSpace, Unit, Basis, Origin, Validator> q)
  {
    return {std::abs(q.value()), Unit{}};
  }

  template<std::floating_point T, class Arena=implicit_common_arena>
  [[nodiscard]]
  constexpr T sin(physical_value<angular_space<T, Arena>, si::units::radian_t> theta)
  {
    return std::sin(theta.value());
  }

  template<std::floating_point T, class Arena=implicit_common_arena>
  [[nodiscard]]
  constexpr T cos(physical_value<angular_space<T, Arena>, si::units::radian_t> theta)
  {
    return std::cos(theta.value());
  }

  template<std::floating_point T, class Arena=implicit_common_arena>
  [[nodiscard]]
  constexpr T tan(physical_value<angular_space<T, Arena>, si::units::radian_t> theta)
  {
    return std::tan(theta.value());
  }

  template<class Arena=implicit_common_arena, std::floating_point T>
  [[nodiscard]]
  constexpr physical_value<angular_space<T, Arena>, si::units::radian_t> asin(T x)
  {
    return {std::asin(x), si::units::radian};
  }

  template<class Arena=implicit_common_arena, std::floating_point T>
  [[nodiscard]]
  constexpr physical_value<angular_space<T, Arena>, si::units::radian_t> acos(T x)
  {
    return {std::acos(x), si::units::radian};
  }

  template<class Arena=implicit_common_arena, std::floating_point T>
  [[nodiscard]]
  constexpr physical_value<angular_space<T, Arena>, si::units::radian_t> atan(T x)
  {
    return {std::atan(x), si::units::radian};
  }

  template<
    convex_space ValueSpace,
    physical_unit Unit,
    validator_for<ValueSpace> Validator=typename Unit::validator_type
  >
    requires has_consistent_validator<ValueSpace, Validator>
  using quantity = physical_value<ValueSpace, Unit, canonical_right_handed_basis<free_module_type_of_t<ValueSpace>>, to_origin_type_t<ValueSpace, Unit>, Validator>;

  
  template<std::floating_point Rep, class Arena=implicit_common_arena>
  using euclidean_1d_vector_quantity = quantity<euclidean_vector_space<Rep, 1, Arena>, no_unit_t, std::identity>;

  template<std::floating_point Rep, class Arena=implicit_common_arena>
  using euclidean_half_line_quantity = quantity<euclidean_half_space<Rep, Arena>, no_unit_t>;
}

namespace sequoia::maths
{
  using namespace physics;

  template<
    convex_space ValueSpaceFrom,
    physical_unit UnitFrom,
    basis_for<free_module_type_of_t<ValueSpaceFrom>> BasisFrom,
    class OriginFrom,
    validator_for<ValueSpaceFrom> ValidatorFrom,
    convex_space ValueSpaceTo,
    basis_for<free_module_type_of_t<ValueSpaceTo>> BasisTo,
    physical_unit UnitTo,
    class OriginTo,
    validator_for<ValueSpaceTo> ValidatorTo
  >
    requires std::same_as<root_transform_unit_t<UnitFrom>, root_transform_unit_t<UnitTo>>  
  struct coordinate_transformation<
    physical_value<ValueSpaceFrom, UnitFrom, BasisFrom, OriginFrom, ValidatorFrom>,
    physical_value<ValueSpaceTo,   UnitTo,   BasisTo,   OriginTo,   ValidatorTo>
  >
  {
    using value_type      = commutative_ring_type_of_t<ValueSpaceFrom>;
    using from_unit_type  = UnitFrom;
    using from_type       = physical_value<ValueSpaceFrom, from_unit_type, BasisFrom, OriginFrom, ValidatorFrom>;
    using to_unit_type    = UnitTo;
    using to_type         = physical_value<ValueSpaceTo, to_unit_type, BasisTo, OriginTo, ValidatorTo>;
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
        to_unit_type{}
      };
    }
  };
}

// TO DO: extend this
template<
  sequoia::maths::convex_space ValueSpace,
  sequoia::physics::physical_unit Unit,
  sequoia::maths::validator_for<ValueSpace> Validator
>
  requires (sequoia::maths::dimension_of<ValueSpace> == 1)
struct std::formatter<sequoia::physics::quantity<ValueSpace, Unit, Validator>>
{
  constexpr auto parse(auto& ctx)
  {
    return ctx.begin();
  }

  auto format(const sequoia::physics::quantity<ValueSpace, Unit, Validator>& q, auto& ctx) const
  {
    if constexpr(sequoia::physics::has_symbol_v<Unit>)
      return std::format_to(ctx.out(), "{} {}", q.value(), Unit::symbol);
    else
      return std::format_to(ctx.out(), "{}", q.value());
  }
};

