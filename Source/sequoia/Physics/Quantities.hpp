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

namespace sequoia::physics
{
  using namespace maths;

  struct coordinate_basis_type{};

  template<class VectorSpace, class Unit, std::floating_point T>
  struct quantity_displacement_basis
  {
    using vector_space_type      = VectorSpace;
    using unit_type              = Unit;
    using basis_orientation_type = coordinate_basis_type;
  };

  template<class T>
  concept quantity_unit = requires {
    typename T::validator_type;
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

    constexpr static bool is_intrinsically_absolute{(dimension == 1) && defines_absolute_scale_v<intrinsic_validator_type>};
    constexpr static bool is_unsafe{!std::is_same_v<Validator, intrinsic_validator_type>};
    constexpr static bool is_effectively_absolute{is_intrinsically_absolute && !is_unsafe};

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
    constexpr quantity operator-() const noexcept(coordinates_type::has_identity_validator)
      requires (!is_effectively_absolute)
    {
      return quantity{to_array(this->values(), [](value_type t) { return -t; }), units_type{}};
    }

    [[nodiscard]]
    friend constexpr displacement_quantity_type operator-(const quantity& lhs, const quantity& rhs) noexcept(coordinates_type::has_identity_validator)
    {
      return[&] <std::size_t... Is>(std::index_sequence<Is...>) {
        return displacement_quantity_type{(lhs.values()[Is] - rhs.values()[Is])..., units_type{}};
      }(std::make_index_sequence<D>{});
    }
  };


  namespace quantity_sets
  {
    struct masses
    {
      using topological_space_type = std::true_type;
    };

    template<class QuantitySet>
    struct differences
    {
      using quantity_set_type = QuantitySet;
    };
  }

  template<std::floating_point T>
  struct mass_displacement_space
  {
    using set_type          = quantity_sets::differences<quantity_sets::masses>;
    using field_type        = T;
    using vector_space_type = mass_displacement_space;
    constexpr static std::size_t dimension{1};
  };

  template<std::floating_point T>
  struct mass_space
  {
    using set_type          = quantity_sets::masses;
    using vector_space_type = mass_displacement_space<T>;
  };

  namespace units
  {
    struct kilogram_t
    {
      using validator_type = absolute_validator;
    };

    inline constexpr kilogram_t kilogram{};
  }
}
