////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/Maths/Geometry/VectorSpace.hpp"

#include <functional>

namespace sequoia
{
  namespace physics
  {
    template<class T>
    concept quantity_unit = requires {
      typename T::validator_type;
    };
    
    template<class T, class U>
    struct reduced_validator;

    template<class T, class U>
    using reduced_validator_t = reduced_validator<T, U>::type;
  }

  namespace maths
  {
    template<physics::quantity_unit T, physics::quantity_unit U>
    struct reduction<direct_product<T, U>>
    {
      using validator_type = physics::reduced_validator_t<typename T::validator_type, typename U::validator_type>;
    };
  }
}


namespace sequoia::physics
{
  using namespace maths;

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
  struct reduced_validator<absolute_validator, absolute_validator>
  {
    using type = absolute_validator;
  };

  template<convex_space QuantitySpace, quantity_unit Unit>
  using to_displacement_basis_t
    = quantity_displacement_basis<vector_space_type<QuantitySpace>, Unit, space_field_type<QuantitySpace>>;

  template<convex_space QuantitySpace, quantity_unit Unit, class Validator>
  class quantity;
  
  template<convex_space QuantitySpace, quantity_unit Unit, class Validator>
  using to_coordinates_base_type
    = coordinates_base<
        QuantitySpace,
        to_displacement_basis_t<QuantitySpace, Unit>,
        intrinsic_origin,
        Validator,
        quantity<vector_space_type<QuantitySpace>, Unit, std::identity>>;

  template<class T, class U>
  struct quantity_product;

  template<class T, class U>
  using quantity_product_t = quantity_product<T, U>::type;
  
  template<
    convex_space LHSQuantitySpace, quantity_unit LHSUnit, class LHSValidator,
    convex_space RHSQuantitySpace, quantity_unit RHSUnit, class RHSValidator
  >
  struct quantity_product<
           quantity<LHSQuantitySpace, LHSUnit, LHSValidator>,
           quantity<RHSQuantitySpace, RHSUnit, RHSValidator>
         >
  {
    using type = quantity<
                   reduction<direct_product<LHSQuantitySpace, RHSQuantitySpace>>,
                   reduction<direct_product<LHSUnit, RHSUnit>>,
                   reduced_validator_t<LHSValidator, RHSValidator>>;
  };
  
  template<convex_space QuantitySpace, quantity_unit Unit, class Validator=typename Unit::validator_type>
  class quantity : public to_coordinates_base_type<QuantitySpace, Unit, Validator>
  {
  public:
    using coordinates_type           = to_coordinates_base_type<QuantitySpace, Unit, Validator>;
    using quantity_space_type        = QuantitySpace;
    using units_type                 = Unit;
    using displacement_space_type    = vector_space_type<QuantitySpace>;
    using intrinsic_validator_type   = Unit::validator_type;
    using validator_type             = Validator;
    using field_type                 = space_field_type<QuantitySpace>;
    using value_type                 = field_type;
    using displacement_quantity_type = coordinates_type::displacement_coordinates_type;

    constexpr static std::size_t dimension{displacement_space_type::dimension};
    constexpr static std::size_t D{dimension};

    constexpr static bool is_intrinsically_absolute{(D == 1) && defines_absolute_scale_v<intrinsic_validator_type>};
    constexpr static bool is_unsafe{!std::is_same_v<Validator, intrinsic_validator_type>};
    constexpr static bool is_effectively_absolute{is_intrinsically_absolute && !is_unsafe};
    constexpr static bool has_identity_validator{coordinates_type::has_identity_validator};

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

    // Start with the special case of the same units and the numerator of dim 1 (the latter is always true of the denom)
    template<convex_space DenominatorQuantitySpace, class DenominatorValidator>
      requires     ( D == 1)
                && (quantity<DenominatorQuantitySpace, Unit, DenominatorValidator>::D == 1)
                && (is_intrinsically_absolute || vector_space<QuantitySpace>)
                && (   quantity<DenominatorQuantitySpace, Unit, DenominatorValidator>::is_intrinsically_absolute
                    || vector_space<DenominatorQuantitySpace>)
    [[nodiscard]]
    friend constexpr std::common_type_t<value_type, typename quantity<DenominatorQuantitySpace, Unit, DenominatorValidator>::value_type>
      operator/(const quantity& lhs, const quantity<DenominatorQuantitySpace, Unit, DenominatorValidator>& rhs)
    {
      return lhs.value() / rhs.value();
    }   

    template<convex_space RHSQuantitySpace, quantity_unit RHSUnit, class RHSValidator>
      requires (D == 1) || (quantity<RHSQuantitySpace, Unit, RHSValidator>::D == 1)
    [[nodiscard]]
    friend constexpr quantity_product_t<quantity, quantity<RHSQuantitySpace, RHSUnit, RHSValidator>>
      operator*(const quantity& lhs, const quantity<RHSQuantitySpace, RHSUnit, RHSValidator>& rhs)
    {
      return quantity_product_t<quantity, quantity<RHSQuantitySpace, RHSUnit, RHSValidator>>{lhs.value() * rhs.value(), reduction<direct_product<Unit, RHSUnit>>{}};
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

  template<class QuantitySet, std::floating_point T>
  struct displacement_space
  {
    using set_type          = classical_quantity_sets::differences<QuantitySet>;
    using field_type        = T;
    using vector_space_type = displacement_space;
    constexpr static std::size_t dimension{1};
  };

  template<class QuantitySet, std::floating_point T>
  struct quantity_space
  {
    using set_type          = QuantitySet;
    using vector_space_type = displacement_space<QuantitySet, T>;
  };

  template<std::floating_point T>
  struct mass_space : quantity_space<classical_quantity_sets::masses, T> {};

  template<std::floating_point T>
  struct length_space : quantity_space<classical_quantity_sets::lengths, T> {};

  template<std::floating_point T>
  struct time_space : quantity_space<classical_quantity_sets::times, T> {};

  template<std::floating_point T>
  struct temperature_space : quantity_space<classical_quantity_sets::temperatures, T> {};

  template<std::floating_point T>
  struct electrical_charge_space : quantity_space<classical_quantity_sets::electrical_charges, T> {};

  namespace units
  {
    struct kilogram_t
    {
      using validator_type = absolute_validator;
    };

    struct metre_t
    {
      using validator_type = absolute_validator;
    };

    struct second_t
    {
      using validator_type = std::identity;
    };

    struct kelvin_t
    {
      using validator_type = absolute_validator;
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
