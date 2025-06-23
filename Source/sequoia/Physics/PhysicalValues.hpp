////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/Physics/PhysicalValuesDetails.hpp"

namespace sequoia::physics
{
  template<physical_unit T>
  struct is_invertible_unit : std::false_type {};

  template<physical_unit T>
    requires   std::is_same_v<typename T::validator_type, std::identity>
            || std::is_same_v<typename T::validator_type, maths::half_line_validator>
  struct is_invertible_unit<T> : std::true_type
  {};

  template<physical_unit T>
  using is_invertible_unit_t = is_invertible_unit<T>::type;

  template<physical_unit T>
  inline constexpr bool is_invertible_unit_v{is_invertible_unit<T>::value};
}

namespace sequoia::maths
{
  template<physics::physical_unit T>
    requires physics::is_invertible_unit_v<T>
  struct dual<T>
  {
    using validator_type = T::validator_type;
  };

  /** @brief Specialization for units, such as degrees Celsius, for which
             the corresponding quantity cannot be inverted.

             Note, though, that inverse units of this type can appear in
             displacements, where the validator is overridden, assumed to
             be std::identity
   */
  template<physics::physical_unit T>
    requires (!physics::is_invertible_unit_v<T>)
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
    constexpr static std::size_t dimension{std::ranges::max({dimension_of<Ts>...})};
    using is_free_module = std::true_type;
  };

  template<convex_space... Ts>
    requires (!affine_space<Ts> && ...)
  struct composite_space<Ts...>
  {
    using direct_product_t = direct_product<Ts...>;
    using set_type         = reduction<typename direct_product_t::set_type>;
    using free_module_type = composite_space<free_module_type_of_t<Ts>...>;
    using is_convex_space  = std::true_type;
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
    using type = impl::simplify_t<direct_product<Us...>, direct_product<T>>;
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
  
  template<std::size_t D>
  struct canonical_convention;

  template<>
  struct canonical_convention<1>
  {
    constexpr static std::size_t dimension{1};
  };

  template<std::size_t D>
    requires (D > 1)
  struct canonical_convention<D> : canonical_convention<1>
  {
    constexpr static std::size_t dimension{D};
  };

  struct y_down_convention : canonical_convention<1>
  {
    constexpr static std::size_t dimension{2};
  };

  struct left_handed_convention : canonical_convention<1>
  {
    constexpr static std::size_t dimension{3};
  };
  

  template<free_module FreeModule, class Unit, class Convention>
  struct physical_value_displacement_basis
  {
    using free_module_type = FreeModule;
    using unit_type        = Unit;
    using convention_type  = Convention;
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

  template<convex_space ValueSpace, physical_unit Unit, class Convention>
  using to_displacement_basis_t
    = physical_value_displacement_basis<free_module_type_of_t<ValueSpace>, Unit, Convention>;

  template<convex_space ValueSpace, physical_unit Unit>
  inline constexpr bool has_distinguished_origin{
       free_module<ValueSpace>
    || (!affine_space<ValueSpace> && defines_half_line_v<typename Unit::validator_type>)
  };

  template<convex_space ValueSpace, physical_unit Unit, class Origin>
  inline constexpr bool has_consistent_origin{
       ( has_distinguished_origin<ValueSpace, Unit> &&  std::is_same_v<Origin, distinguished_origin>)
    || (!has_distinguished_origin<ValueSpace, Unit> && !std::is_same_v<Origin, distinguished_origin>)
  };

  template<convex_space ValueSpace, validator_for<ValueSpace> Validator>
  inline constexpr bool has_consistent_validator{
    !affine_space<ValueSpace> || std::is_same_v<Validator, std::identity>
  };

  template<convex_space ValueSpace, physical_unit Unit>
  inline constexpr bool has_consistent_unit{
    !is_dual_v<Unit> || vector_space<ValueSpace> || (!affine_space<ValueSpace> && is_invertible_unit_v<dual_of_t<Unit>>)
  };
  
  template<convex_space ValueSpace, physical_unit Unit, class Convention, class Origin, validator_for<ValueSpace> Validator>
    requires    has_consistent_unit<ValueSpace, Unit>
             && has_consistent_validator<ValueSpace, Validator>
             && has_consistent_origin<ValueSpace, Unit, Origin>
  class physical_value;

  template<physical_unit Unit>
  struct unit_defined_origin{};

  template<affine_space T>
  struct implicit_affine_origin {};

  template<convex_space ValueSpace, physical_unit Unit>
  struct to_origin_type;

  template<convex_space ValueSpace, physical_unit Unit>
    requires (!has_distinguished_origin<ValueSpace, Unit>) && (!affine_space<ValueSpace>)
  struct to_origin_type<ValueSpace, Unit>
  {
    using type = unit_defined_origin<Unit>;
  };

  template<convex_space ValueSpace, physical_unit Unit>
  using to_origin_type_t = to_origin_type<ValueSpace, Unit>::type;

  template<convex_space ValueSpace, physical_unit Unit>
    requires has_distinguished_origin<ValueSpace, Unit>
  struct to_origin_type<ValueSpace, Unit>
  {
    using type = distinguished_origin;
  };

  template<convex_space ValueSpace, physical_unit Unit>
    requires (!has_distinguished_origin<ValueSpace, Unit> && affine_space<ValueSpace>)
  struct to_origin_type<ValueSpace, Unit>
  {
    using type = implicit_affine_origin<ValueSpace>;
  };
  
  template<convex_space ValueSpace, physical_unit Unit, class Convention, class Origin, class Validator>
  using to_coordinates_base_type
    = coordinates_base<
        ValueSpace,
        to_displacement_basis_t<ValueSpace, Unit, Convention>,
        Origin,
        Validator,
        physical_value<free_module_type_of_t<ValueSpace>, Unit, Convention, distinguished_origin, std::identity>>;

  template<convex_space T>
  inline constexpr bool has_base_space_v{
    requires { typename T::base_space; }
  };

  template<convex_space T>
  struct to_base_space
  {
    using type = T;
  };

  template<convex_space T>
    requires has_base_space_v<T>
  struct to_base_space<T>
  {
    using type = T::base_space;
  };

  template<convex_space T>
  using to_base_space_t = to_base_space<T>::type;

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
  
  template<class T, class U>
  struct physical_value_product;

  template<class T, class U>
  using physical_value_product_t = physical_value_product<T, U>::type;
  
  template<
    convex_space LHSValueSpace, physical_unit LHSUnit, class LHSConvention, class LHSValidator,
    convex_space RHSValueSpace, physical_unit RHSUnit, class RHSConvention, class RHSValidator
  >
    requires std::common_with<LHSConvention, RHSConvention>
  struct physical_value_product<physical_value<LHSValueSpace, LHSUnit, LHSConvention, distinguished_origin, LHSValidator>,
                                physical_value<RHSValueSpace, RHSUnit, RHSConvention, distinguished_origin, RHSValidator>>
  {
    using type
      = physical_value<
          impl::to_composite_space_t<reduction_t<direct_product<to_base_space_t<LHSValueSpace>, to_base_space_t<RHSValueSpace>>>>,
          impl::to_composite_space_t<reduction_t<direct_product<LHSUnit, RHSUnit>>>,
          std::common_type_t<LHSConvention, RHSConvention>,
          distinguished_origin,
          reduced_validator_t<LHSValidator, RHSValidator>
        >;
  };

  template<
    convex_space LHSValueSpace, physical_unit LHSUnit, class LHSConvention, class LHSValidator,
    convex_space RHSValueSpace, physical_unit RHSUnit, class RHSConvention, class RHSValidator
  >
    requires     std::common_with<LHSConvention, RHSConvention>
             && (   std::is_same_v<euclidean_vector_space<1, commutative_ring_type_of_t<LHSValueSpace>>, LHSValueSpace>
                 || std::is_same_v<euclidean_half_space<commutative_ring_type_of_t<LHSValueSpace>>, LHSValueSpace>)
  struct physical_value_product<physical_value<LHSValueSpace, LHSUnit, LHSConvention, distinguished_origin, LHSValidator>,
                                physical_value<RHSValueSpace, RHSUnit, RHSConvention, distinguished_origin, RHSValidator>>
  {
    using type
      = physical_value<
          to_base_space_t<RHSValueSpace>,
          RHSUnit,
          std::common_type_t<LHSConvention, RHSConvention>,
          distinguished_origin,
          reduced_validator_t<LHSValidator, RHSValidator>
        >;
  };

  template<
    convex_space LHSValueSpace, physical_unit LHSUnit, class LHSConvention, class LHSValidator,
    convex_space RHSValueSpace, physical_unit RHSUnit, class RHSConvention, class RHSValidator
  >
    requires     std::common_with<LHSConvention, RHSConvention>
             && (   std::is_same_v<euclidean_vector_space<1, commutative_ring_type_of_t<RHSValueSpace>>, RHSValueSpace>
                 || std::is_same_v<euclidean_half_space<commutative_ring_type_of_t<RHSValueSpace>>, RHSValueSpace>)
  struct physical_value_product<physical_value<LHSValueSpace, LHSUnit, LHSConvention, distinguished_origin, LHSValidator>,
                                physical_value<RHSValueSpace, RHSUnit, RHSConvention, distinguished_origin, RHSValidator>>
  {
    using type
      = physical_value<
          to_base_space_t<LHSValueSpace>,
          LHSUnit,
          std::common_type_t<LHSConvention, RHSConvention>,
          distinguished_origin,
          reduced_validator_t<LHSValidator, RHSValidator>
        >;
  };
  
  template<
    convex_space ValueSpace,
    physical_unit Unit,
    class Convention                    = canonical_convention<free_module_type_of_t<ValueSpace>::dimension>,
    class Origin                        = to_origin_type_t<ValueSpace, Unit>,
    validator_for<ValueSpace> Validator = typename Unit::validator_type
  >
    requires    has_consistent_unit<ValueSpace, Unit>
             && has_consistent_validator<ValueSpace, Validator>
             && has_consistent_origin<ValueSpace, Unit, Origin>
  class physical_value final : public to_coordinates_base_type<ValueSpace, Unit, Convention, Origin, Validator>
  {
  public:
    using coordinates_type         = to_coordinates_base_type<ValueSpace, Unit, Convention, Origin, Validator>;
    using space_type               = ValueSpace;
    using units_type               = Unit;
    using convention_type          = Convention;
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

    template<convex_space RHSValueSpace, class RHSUnit, class RHSConvention, class RHSOrigin, class RHSValidator>
    constexpr static bool is_composable_with{
          std::common_with<convention_type, RHSConvention>
      && (is_intrinsically_absolute || vector_space<space_type>)
      && (physical_value<RHSValueSpace, RHSUnit, RHSConvention, RHSOrigin, RHSValidator>::is_intrinsically_absolute || vector_space<RHSValueSpace>)
    };

    template<convex_space RHSValueSpace, class RHSUnit, class RHSConvention, class RHSOrigin, class RHSValidator>
    constexpr static bool is_multipicable_with{
         std::common_with<convention_type, RHSConvention>
      && is_composable_with<RHSValueSpace, RHSUnit, RHSConvention, RHSOrigin, RHSValidator>
      && ((D == 1) || (physical_value<RHSValueSpace, RHSUnit, RHSConvention, RHSOrigin, RHSValidator>::D == 1))
    };

    template<convex_space RHSValueSpace, class RHSUnit, class RHSConvention, class RHSOrigin, class RHSValidator>
    constexpr static bool is_divisible_with{
         weak_field<ring_type>
      && weak_field<commutative_ring_type_of_t<RHSValueSpace>>
      && std::common_with<convention_type, RHSConvention>
      && is_composable_with<RHSValueSpace, RHSUnit, RHSConvention, RHSOrigin, RHSValidator>
      && (physical_value<RHSValueSpace, RHSUnit, RHSConvention, RHSOrigin, RHSValidator>::D == 1)
    };

    constexpr physical_value() = default;

    constexpr physical_value(value_type val, units_type) requires (D == 1)
      : coordinates_type{val}
    {}

    constexpr physical_value(std::span<const value_type, D> val, units_type)
      : coordinates_type{val}
    {}

    constexpr physical_value operator-() const requires is_effectively_absolute = delete;

    [[nodiscard]]
    constexpr physical_value operator+() const
    {
      return physical_value{this->values(), units_type{}};
    }

    using coordinates_type::operator+=;
    
    template<convex_space OtherValueSpace>
      requires is_intrinsically_absolute && (std::is_base_of_v<ValueSpace, OtherValueSpace>)
    constexpr physical_value& operator+=(const physical_value<OtherValueSpace, Unit, Convention, Origin, Validator>& other) noexcept(has_identity_validator)
    {
      this->apply_to_each_element(other.values(), [](value_type& lhs, value_type rhs){ lhs += rhs; });
      return *this;
    }

    template<convex_space OtherValueSpace>
      requires (!std::is_same_v<ValueSpace, OtherValueSpace>) && have_compatible_base_spaces_v<ValueSpace, OtherValueSpace>
    [[nodiscard]]
    friend constexpr auto operator+(const physical_value& lhs, const physical_value<OtherValueSpace, Unit, Convention, Origin, Validator>& rhs)
    {
      using physical_value_t
        = physical_value<std::common_type_t<typename ValueSpace::base_space, typename OtherValueSpace::base_space>, Unit, Convention, Origin, Validator>;
      return physical_value_t{lhs.values(), units_type{}} += rhs;
    }
    
    [[nodiscard]]
    constexpr physical_value operator-() const noexcept(has_identity_validator)
      requires (!is_effectively_absolute)
    {
      return physical_value{utilities::to_array(this->values(), [](value_type t) { return -t; }), units_type{}};
    }

    template<class OtherValueSpace>
    requires   (!std::is_same_v<OtherValueSpace, displacement_space_type>)
            && (std::is_same_v<ValueSpace, OtherValueSpace> || have_compatible_base_spaces_v<ValueSpace, OtherValueSpace>)
    [[nodiscard]]
    friend constexpr auto operator-(const physical_value& lhs, const physical_value<OtherValueSpace, Unit, Convention, Origin, Validator>& rhs)
      noexcept(has_identity_validator)
    {
      using disp_space_t = to_displacement_space_t<ValueSpace, OtherValueSpace>;
      using disp_t = to_coordinates_base_type<disp_space_t, Unit, convention_type, Origin, Validator>::displacement_coordinates_type;
      return[&] <std::size_t... Is>(std::index_sequence<Is...>) {
        return disp_t{std::array{(lhs.values()[Is] - rhs.values()[Is])...}, units_type{}};
      }(std::make_index_sequence<D>{});
    }

    template<convex_space RHSValueSpace, physical_unit RHSUnit, class RHSConvention, class RHSOrigin, class RHSValidator>
      requires is_multipicable_with<RHSValueSpace, RHSUnit, RHSConvention, RHSOrigin, RHSValidator>
    [[nodiscard]]
    friend constexpr auto operator*(const physical_value& lhs,
                                    const physical_value<RHSValueSpace, RHSUnit, RHSConvention, RHSOrigin, RHSValidator>& rhs)
    {
      using physical_value_t
        = physical_value_product_t<
            physical_value,
            physical_value<RHSValueSpace,RHSUnit, RHSConvention, RHSOrigin, RHSValidator>
          >;

      using derived_units_type = physical_value_t::units_type;
      return physical_value_t{lhs.value() * rhs.value(), derived_units_type{}};
    }

    template<convex_space RHSValueSpace, class RHSUnit, class RHSConvention, class RHSOrigin, class RHSValidator>
      requires is_divisible_with<RHSValueSpace, RHSUnit, RHSConvention, RHSOrigin, RHSValidator>
    [[nodiscard]]
    friend constexpr auto operator/(const physical_value& lhs,
                                    const physical_value<RHSValueSpace, RHSUnit, RHSConvention, RHSOrigin, RHSValidator>& rhs)
    {
      using physical_value_t
        = physical_value_product_t<
            physical_value,
            physical_value<dual_of_t<to_base_space_t<RHSValueSpace>>, dual_of_t<RHSUnit>, RHSConvention, RHSOrigin, RHSValidator>
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
      using physical_value_t = physical_value<dual_of_t<ValueSpace>, dual_of_t<Unit>, convention_type, origin_type, validator_type>;
      using derived_units_type = physical_value_t::units_type;
      return physical_value_t{value / rhs.value(), derived_units_type{}};
    }    
  };

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
    using is_free_module = std::true_type;
  };

  template<class PhysicalValueSet, arithmetic Rep, std::size_t D, class Derived>
  struct physical_value_convex_space
  {
    constexpr static std::size_t dimension{D};
    using set_type            = PhysicalValueSet;
    using representation_type = Rep;
    using free_module_type    = associated_displacement_space<Derived>;
    using is_convex_space = std::true_type;
  };

  template<class PhysicalValueSet, arithmetic Rep, std::size_t D, class Derived>
  struct physical_value_affine_space
  {
    constexpr static std::size_t dimension{D};
    using set_type            = PhysicalValueSet;
    using representation_type = Rep;
    using free_module_type    = associated_displacement_space<Derived>;
    using is_affine_space = std::true_type;
  };

  template<class PhysicalValueSet, arithmetic Rep, std::size_t D, class Derived>
  struct physical_value_vector_space
  {
    constexpr static std::size_t dimension{D};
    using set_type            = PhysicalValueSet;
    using representation_type = Rep;
    using field_type          = Rep;
    using is_vector_space = std::true_type;
  };

  template<std::floating_point Rep, class Arena>
  struct mass_space
    : physical_value_convex_space<sets::classical::masses<Arena>, Rep, 1, mass_space<Rep, Arena>>
  {
    using base_space = mass_space;
  };

  template<std::floating_point Rep, class Arena>
  struct temperature_space
    : physical_value_convex_space<sets::classical::temperatures<Arena>, Rep, 1, temperature_space<Rep, Arena>>
  {
    using base_space = temperature_space;
  };

  template<std::floating_point Rep, class Arena>
  struct electrical_current_space
    : physical_value_vector_space<sets::classical::electrical_currents<Arena>, Rep, 1, electrical_current_space<Rep, Arena>>
  {
    using base_space = electrical_current_space;
  };

  template<std::floating_point Rep, class Arena>
  struct angular_space : physical_value_vector_space<sets::classical::angles<Arena>, Rep, 1, angular_space<Rep, Arena>>
  {
    using base_space = angular_space;
  };

  template<arithmetic Rep, class Arena>
  struct length_space
    : physical_value_convex_space<sets::classical::lengths<Arena>, Rep, 1, length_space<Rep, Arena>>
  {
    using base_space = length_space;
  };

  template<arithmetic Rep, class Arena>
  struct width_space : length_space<Rep, Arena>
  {
    using convex_space_type = width_space;
  };

  template<arithmetic Rep, class Arena>
  struct height_space : length_space<Rep, Arena>
  {
    using convex_space_type = height_space;
  };

  template<arithmetic Rep, class Arena>
  struct time_interval_space
    : physical_value_convex_space<sets::classical::time_intervals<Arena>, Rep, 1, time_interval_space<Rep, Arena>>
  {};
  
  template<arithmetic Rep, class Arena>
  struct time_space : physical_value_affine_space<sets::classical::times<Arena>, Rep, 1, time_space<Rep, Arena>>
  {};

  template<std::size_t D, arithmetic Rep, class Arena>
  struct position_space : physical_value_affine_space<sets::classical::positions<Arena>, Rep, D, position_space<D, Rep, Arena>>
  {};
  
  struct implicit_common_arena {};

  template<physical_unit U>
  inline constexpr bool has_symbol_v{
    requires {
      { U::symbol } -> std::convertible_to<std::string_view>; }
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
        };

        using validator_type = validator;
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
    }

    template<std::floating_point T, class Arena=implicit_common_arena>
    using mass = physical_value<mass_space<T, Arena>, units::kilogram_t>;

    template<std::floating_point T, class Arena=implicit_common_arena>
    using length = physical_value<length_space<T, Arena>, units::metre_t>;

    template<std::floating_point T, class Arena=implicit_common_arena>
    using time_interval = physical_value<time_interval_space<T, Arena>, units::second_t>;

    template<std::floating_point T, class Arena=implicit_common_arena>
    using temperature = physical_value<temperature_space<T, Arena>, units::kelvin_t>;

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
    using time = physical_value<time_space<T, Arena>, units::second_t, canonical_convention<1>, Origin, std::identity>;

    template<
      std::size_t D,
      std::floating_point T,
      class Arena      = implicit_common_arena,      
      class Convention = canonical_convention<D>,
      class Origin     = implicit_affine_origin<position_space<D, T, Arena>>
    >
    using position = physical_value<position_space<D, T, Arena>, units::metre_t, Convention, Origin, std::identity>;
  }

  template<vector_space ValueSpace, physical_unit Unit, class Convention, class Origin, validator_for<ValueSpace> Validator>
    requires (dimension_of<ValueSpace> == 1)
  [[nodiscard]]
  constexpr physical_value<ValueSpace, Unit, Convention, Origin, Validator> abs(physical_value<ValueSpace, Unit, Convention, Origin, Validator> q)
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
  using quantity = physical_value<ValueSpace, Unit, canonical_convention<1>, to_origin_type_t<ValueSpace, Unit>, Validator>;
}

template<
  sequoia::maths::convex_space ValueSpace,
  sequoia::physics::physical_unit Unit,
  sequoia::maths::validator_for<ValueSpace> Validator
>
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

