////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/Physics/QuantitiesDetails.hpp"

namespace sequoia::maths
{
  template<physics::quantity_unit T>
  struct dual<T>
  {
    // TO DO: this doesn't hold for all validators!
    using validator_type = T::validator_type;
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

  // Ts are assumed to be ordered
  template<physics::quantity_unit... Ts>
  struct composite_unit<std::tuple<Ts...>>
  {
    using validator_type = reduced_validator_t<typename Ts::validator_type...>;
  };

  /// \class Primary class template for the reduction of direct products to a lower dimensional space
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
  struct reduced_validator<half_line_validator, half_line_validator>
  {
    using type = half_line_validator;
  };

  template<convex_space QuantitySpace, quantity_unit Unit>
  using to_displacement_basis_t
    = quantity_displacement_basis<vector_space_type_of<QuantitySpace>, Unit, space_field_type<QuantitySpace>>;

  template<convex_space QuantitySpace, quantity_unit Unit>
  inline constexpr bool has_intrinsic_origin{
       vector_space<QuantitySpace>
    || (!affine_space<QuantitySpace> && defines_half_line_v<typename Unit::validator_type>)
  };


  template<convex_space QuantitySpace, quantity_unit Unit, class Origin>
  inline constexpr bool has_consistent_origin{
       ( has_intrinsic_origin<QuantitySpace, Unit> &&  std::is_same_v<Origin, intrinsic_origin>)
    || (!has_intrinsic_origin<QuantitySpace, Unit> && !std::is_same_v<Origin, intrinsic_origin>)
  };

  template<convex_space QuantitySpace, validator_for<QuantitySpace> Validator>
  inline constexpr bool has_consistent_validator{
    !affine_space<QuantitySpace> || std::is_same_v<Validator, std::identity>
  };

  template<convex_space QuantitySpace, quantity_unit Unit, class Origin, validator_for<QuantitySpace> Validator>
    requires has_consistent_validator<QuantitySpace, Validator> && has_consistent_origin<QuantitySpace, Unit, Origin>
  class quantity;

  template<quantity_unit Unit>
  struct unit_defined_origin{};

  template<affine_space T>
  struct implicit_affine_origin {};

  template<convex_space QuantitySpace, quantity_unit Unit>
  struct to_origin_type;

  template<convex_space QuantitySpace, quantity_unit Unit>
    requires (!has_intrinsic_origin<QuantitySpace, Unit>) && (!affine_space<QuantitySpace>)
  struct to_origin_type<QuantitySpace, Unit>
  {
    using type = unit_defined_origin<Unit>;
  };

  template<convex_space QuantitySpace, quantity_unit Unit>
  using to_origin_type_t = to_origin_type<QuantitySpace, Unit>::type;

  template<convex_space QuantitySpace, quantity_unit Unit>
    requires has_intrinsic_origin<QuantitySpace, Unit>
  struct to_origin_type<QuantitySpace, Unit>
  {
    using type = intrinsic_origin;
  };

  template<affine_space QuantitySpace, quantity_unit Unit>
    requires (!has_intrinsic_origin<QuantitySpace, Unit>)
  struct to_origin_type<QuantitySpace, Unit>
  {
    using type = implicit_affine_origin<QuantitySpace>;
  };
  
  template<convex_space QuantitySpace, quantity_unit Unit, class Origin, class Validator>
  using to_coordinates_base_type
    = coordinates_base<
        QuantitySpace,
        to_displacement_basis_t<QuantitySpace, Unit>,
        Origin,
        Validator,
        quantity<vector_space_type_of<QuantitySpace>, Unit, intrinsic_origin, std::identity>>;

  template<class T, class U>
  struct quantity_product;

  template<class T, class U>
  using quantity_product_t = quantity_product<T, U>::type;
  
  template<
    convex_space LHSQuantitySpace, quantity_unit LHSUnit, class LHSValidator,
    convex_space RHSQuantitySpace, quantity_unit RHSUnit, class RHSValidator
  >
  struct quantity_product<quantity<LHSQuantitySpace, LHSUnit, intrinsic_origin, LHSValidator>,
                          quantity<RHSQuantitySpace, RHSUnit, intrinsic_origin, RHSValidator>>
  {
    using type = quantity<
      reduction_t<direct_product<LHSQuantitySpace, RHSQuantitySpace>>,
      reduction_t<std::tuple<LHSUnit, RHSUnit>>,
      intrinsic_origin,
      reduced_validator_t<LHSValidator, RHSValidator>>;
  };

  template<
    convex_space LHSQuantitySpace, quantity_unit LHSUnit, class LHSValidator,
    convex_space RHSQuantitySpace, quantity_unit RHSUnit, class RHSValidator
  >
    requires    std::is_same_v<euclidean_vector_space<1, space_field_type<LHSQuantitySpace>>, LHSQuantitySpace>
             || std::is_same_v<euclidean_half_space<1, space_field_type<LHSQuantitySpace>>, LHSQuantitySpace>
  struct quantity_product<quantity<LHSQuantitySpace, LHSUnit, intrinsic_origin, LHSValidator>,
                          quantity<RHSQuantitySpace, RHSUnit, intrinsic_origin, RHSValidator>>
  {
    using type = quantity<RHSQuantitySpace, RHSUnit, intrinsic_origin, reduced_validator_t<LHSValidator, RHSValidator>>;
  };

  template<
    convex_space LHSQuantitySpace, quantity_unit LHSUnit, class LHSValidator,
    convex_space RHSQuantitySpace, quantity_unit RHSUnit, class RHSValidator
  >
    requires    std::is_same_v<euclidean_vector_space<1, space_field_type<RHSQuantitySpace>>, RHSQuantitySpace>
             || std::is_same_v<euclidean_half_space<1, space_field_type<RHSQuantitySpace>>, RHSQuantitySpace>
  struct quantity_product<quantity<LHSQuantitySpace, LHSUnit, intrinsic_origin, LHSValidator>,
                          quantity<RHSQuantitySpace, RHSUnit, intrinsic_origin, RHSValidator>>
  {
    using type = quantity<LHSQuantitySpace, LHSUnit, intrinsic_origin, reduced_validator_t<LHSValidator, RHSValidator>>;
  };

  template<
    convex_space QuantitySpace,
    quantity_unit Unit,
    class Origin=to_origin_type_t<QuantitySpace, Unit>,
    validator_for<QuantitySpace> Validator=typename Unit::validator_type
  >
    requires has_consistent_validator<QuantitySpace, Validator> && has_consistent_origin<QuantitySpace, Unit, Origin>
  class quantity : public to_coordinates_base_type<QuantitySpace, Unit, Origin, Validator>
  {
  public:
    using coordinates_type           = to_coordinates_base_type<QuantitySpace, Unit, Origin, Validator>;
    using quantity_space_type        = QuantitySpace;
    using units_type                 = Unit;
    using origin_type                = Origin;
    using displacement_space_type    = vector_space_type_of<QuantitySpace>;
    using intrinsic_validator_type   = Unit::validator_type;
    using validator_type             = Validator;
    using field_type                 = space_field_type<QuantitySpace>;
    using value_type                 = field_type;
    using displacement_quantity_type = coordinates_type::displacement_coordinates_type;

    constexpr static std::size_t dimension{displacement_space_type::dimension};
    constexpr static std::size_t D{dimension};

    constexpr static bool is_intrinsically_absolute{
      (D == 1) && !affine_space<quantity_space_type> && defines_half_line_v<intrinsic_validator_type>
    };

    constexpr static bool is_effectively_absolute{is_intrinsically_absolute && std::is_same_v<Validator, intrinsic_validator_type>};
    constexpr static bool has_identity_validator{coordinates_type::has_identity_validator};

    template<convex_space RHSQuantitySpace, class RHSUnit, class RHSOrigin, class RHSValidator>
    constexpr static bool is_composable_with{
         (is_intrinsically_absolute || vector_space<QuantitySpace>)
      && (quantity<RHSQuantitySpace, RHSUnit, RHSOrigin, RHSValidator>::is_intrinsically_absolute || vector_space<RHSQuantitySpace>)
    };

    template<convex_space RHSQuantitySpace, class RHSUnit, class RHSOrigin, class RHSValidator>
    constexpr static bool is_multipicable_with{
         is_composable_with<RHSQuantitySpace, RHSUnit, RHSOrigin, RHSValidator>
      && ((D == 1) || (quantity<RHSQuantitySpace, RHSUnit, RHSOrigin, RHSValidator>::D == 1))
    };

    template<convex_space RHSQuantitySpace, class RHSUnit, class RHSOrigin, class RHSValidator>
    constexpr static bool is_divisible_with{
         is_composable_with<RHSQuantitySpace, RHSUnit, RHSOrigin, RHSValidator>
      && (quantity<RHSQuantitySpace, RHSUnit, RHSOrigin, RHSValidator>::D == 1)
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

    template<convex_space RHSQuantitySpace, quantity_unit RHSUnit, class RHSOrigin, class RHSValidator>
      requires is_multipicable_with<RHSQuantitySpace, RHSUnit, RHSOrigin, RHSValidator>
    [[nodiscard]]
    friend constexpr auto operator*(const quantity& lhs, const quantity<RHSQuantitySpace, RHSUnit, RHSOrigin, RHSValidator>& rhs)
    {
      using quantity_t = quantity_product_t<quantity, quantity<RHSQuantitySpace, RHSUnit, RHSOrigin, RHSValidator>>;
      using derived_units_type = quantity_t::units_type;
      return quantity_t{lhs.value() * rhs.value(), derived_units_type{}};
    }

    template<convex_space RHSQuantitySpace, class RHSUnit, class RHSOrigin, class RHSValidator>
      requires is_divisible_with<RHSQuantitySpace, RHSUnit, RHSOrigin, RHSValidator>
    [[nodiscard]]
    friend constexpr auto operator/(const quantity& lhs, const quantity<RHSQuantitySpace, RHSUnit, RHSOrigin, RHSValidator>& rhs)
    {
      using quantity_t = quantity_product_t<quantity, quantity<dual_of_t<RHSQuantitySpace>, dual_of_t<RHSUnit>, RHSOrigin, RHSValidator>>;
      using derived_units_type = quantity_t::units_type;
      return quantity_t{lhs.value() / rhs.value(), derived_units_type{}};
    }

    [[nodiscard]] friend constexpr auto operator/(value_type value, const quantity& rhs)
      requires ((D == 1) && (is_intrinsically_absolute || vector_space<QuantitySpace>))
    {
      using quantity_t = quantity<dual_of_t<QuantitySpace>, dual_of_t<Unit>, origin_type, validator_type>;
      using derived_units_type = quantity_t::units_type;
      return quantity_t{value / rhs.value(), derived_units_type{}};
    }    
  };

  namespace classical_quantity_sets
  {
    template<class HostSystem>
    struct masses
    {
      using host_system_type = HostSystem;
    };

    template<class HostSystem>
    struct temperatures
    {
      using host_system_type = HostSystem;
    };

    template<class HostSystem>
    struct electrical_charges
    {
      using host_system_type = HostSystem;
    };

    template<class HostSystem>
    struct times
    {
      using host_system_type = HostSystem;
    };

    template<class HostSystem>
    struct time_intervals
    {
      using host_system_type = HostSystem;
    };

    template<class HostSystem>
    struct positions
    {
      using host_system_type = HostSystem;
    };

    template<class HostSystem>
    struct lengths
    {
      using host_system_type = HostSystem;
    };

    template<class HostSystem>
    struct angles
    {
      using host_system_type = HostSystem;
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

  template<std::floating_point Rep, class HostSystem>
  struct mass_space
    : quantity_convex_space<classical_quantity_sets::masses<HostSystem>, Rep, mass_space<Rep, HostSystem>>
  {};

  template<std::floating_point Rep, class HostSystem>
  struct temperature_space
    : quantity_convex_space<classical_quantity_sets::temperatures<HostSystem>, Rep, temperature_space<Rep, HostSystem>>
  {};

  template<std::floating_point Rep, class HostSystem>
  struct electrical_charge_space
    : quantity_vector_space<classical_quantity_sets::electrical_charges<HostSystem>, Rep, electrical_charge_space<Rep, HostSystem>>
  {};

  template<std::floating_point Rep, class HostSystem>
  struct angular_space : quantity_vector_space<classical_quantity_sets::angles<HostSystem>, Rep, angular_space<Rep, HostSystem>>
  {};

  template<std::floating_point Rep, class HostSystem>
  struct length_space
    : quantity_convex_space<classical_quantity_sets::lengths<HostSystem>, Rep, length_space<Rep, HostSystem>>
  {};

  template<std::floating_point Rep, class HostSystem>
  struct time_interval_space
    : quantity_convex_space<classical_quantity_sets::time_intervals<HostSystem>, Rep, time_interval_space<Rep, HostSystem>>
  {};
  
  template<std::floating_point Rep, class HostSystem>
  struct time_space : quantity_affine_space<classical_quantity_sets::times<HostSystem>, Rep, time_space<Rep, HostSystem>>
  {};

  template<std::size_t D, std::floating_point Rep, class HostSystem>
  struct position_space : quantity_affine_space<classical_quantity_sets::positions<HostSystem>, Rep, position_space<D, Rep, HostSystem>>
  {};

  namespace units
  {
    struct kilogram_t
    {
      using validator_type = half_line_validator;
    };

    struct metre_t
    {
      using validator_type = half_line_validator;
    };

    struct second_t
    {
      using validator_type = half_line_validator;
    };

    struct kelvin_t
    {
      using validator_type = half_line_validator;
    };

    struct coulomb_t
    {
      using validator_type = std::identity;
    };

    struct radian_t
    {
      using validator_type = std::identity;
    };

    inline constexpr kilogram_t kilogram{};
    inline constexpr metre_t    metre{};
    inline constexpr second_t   second{};
    inline constexpr kelvin_t   kelvin{};
    inline constexpr coulomb_t  coulomb{};
    inline constexpr radian_t   radian{};

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
    };

    inline constexpr celsius_t celsius{};
  }
  
  struct implicit_common_system {};

  namespace si
  {
    template<std::floating_point T, class HostSystem=implicit_common_system>
    using mass = quantity<mass_space<T, HostSystem>, units::kilogram_t>;

    template<std::floating_point T, class HostSystem=implicit_common_system>
    using length = quantity<length_space<T, HostSystem>, units::metre_t>;

    template<std::floating_point T, class HostSystem=implicit_common_system>
    using time_interval = quantity<time_interval_space<T, HostSystem>, units::second_t>;

    template<std::floating_point T, class HostSystem=implicit_common_system>
    using temperature = quantity<temperature_space<T, HostSystem>, units::kelvin_t>;

    template<std::floating_point T, class HostSystem=implicit_common_system>
    using electrical_charge = quantity<electrical_charge_space<T, HostSystem>, units::coulomb_t>;

    template<std::floating_point T, class HostSystem=implicit_common_system>
    using angle = quantity<angular_space<T, HostSystem>, units::radian_t>;

    template<
      std::floating_point T,
      class HostSystem=implicit_common_system,
      class Origin=implicit_affine_origin<time_space<T, HostSystem>>
    >
    using time = quantity<time_space<T, HostSystem>, units::second_t, Origin, std::identity>;

    // TO DO: consider other coordinate systems
    template<
      std::size_t D,
      std::floating_point T,
      class HostSystem=implicit_common_system,
      class Origin=implicit_affine_origin<position_space<D, T, HostSystem>>
    >
    using position = quantity<position_space<D, T, HostSystem>, units::metre_t, Origin, std::identity>;
  }

  template<vector_space QuantitySpace, quantity_unit Unit, class Origin, validator_for<QuantitySpace> Validator>
    requires (dimension_of<QuantitySpace> == 1)
  [[nodiscard]]
  constexpr quantity<QuantitySpace, Unit, Origin, Validator> abs(quantity<QuantitySpace, Unit, Origin, Validator> q)
  {
    return {std::abs(q.value()), Unit{}};
  }

  template<std::floating_point T, class HostSystem=implicit_common_system>
  [[nodiscard]]
  constexpr T sin(quantity<angular_space<T, HostSystem>, units::radian_t> theta)
  {
    return std::sin(theta.value());
  }

  template<std::floating_point T, class HostSystem=implicit_common_system>
  [[nodiscard]]
  constexpr T cos(quantity<angular_space<T, HostSystem>, units::radian_t> theta)
  {
    return std::cos(theta.value());
  }

  template<std::floating_point T, class HostSystem=implicit_common_system>
  [[nodiscard]]
  constexpr T tan(quantity<angular_space<T, HostSystem>, units::radian_t> theta)
  {
    return std::tan(theta.value());
  }

  template<class HostSystem=implicit_common_system, std::floating_point T>
  [[nodiscard]]
  constexpr quantity<angular_space<T, HostSystem>, units::radian_t> asin(T x)
  {
    return quantity<angular_space<T, HostSystem>, units::radian_t>{std::asin(x), units::radian};
  }

  template<class HostSystem=implicit_common_system, std::floating_point T>
  [[nodiscard]]
  constexpr quantity<angular_space<T, HostSystem>, units::radian_t> acos(T x)
  {
    return quantity<angular_space<T, HostSystem>, units::radian_t>{std::acos(x), units::radian};
  }

  template<class HostSystem=implicit_common_system, std::floating_point T>
  [[nodiscard]]
  constexpr quantity<angular_space<T, HostSystem>, units::radian_t> atan(T x)
  {
    return quantity<angular_space<T, HostSystem>, units::radian_t>{std::atan(x), units::radian};
  }
}
