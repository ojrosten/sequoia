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

  template<convex_space QuantitySpace, quantity_unit Unit, class Validator=typename Unit::validator_type>
  class quantity : public coordinates_base<QuantitySpace, quantity_displacement_basis<typename QuantitySpace::vector_space_type, Unit, typename QuantitySpace::vector_space_type::field_type>, intrinsic_origin, Validator>
  {
  public:
    using coordinates_type = coordinates_base<QuantitySpace, quantity_displacement_basis<typename QuantitySpace::vector_space_type, Unit, typename QuantitySpace::vector_space_type::field_type>, intrinsic_origin, Validator>;

    using quantity_space_type      = QuantitySpace;
    using unit_type                = Unit;
    using displacement_space_type  = typename QuantitySpace::vector_space_type;
    using intrinsic_validator_type = typename Unit::validator_type;
    using validator_type           = Validator;
    using field_type               = typename displacement_space_type::field_type;
    using value_type               = field_type;

    constexpr static std::size_t dimension{displacement_space_type::dimension};
    constexpr static std::size_t D{dimension};

    constexpr static bool is_intrinsically_absolute{(dimension == 1) && defines_absolute_scale_v<intrinsic_validator_type>};
    constexpr static bool is_unsafe{!std::is_same_v<Validator, intrinsic_validator_type>};
    constexpr static bool is_effectively_absolute{is_intrinsically_absolute && !is_unsafe};

    constexpr quantity() = default;

    constexpr quantity(value_type val, unit_type) requires (D == 1)
      : coordinates_type{val}
    {}

    constexpr quantity(std::span<const value_type, D> val, unit_type)
      : coordinates_type{val}
    {}

    quantity operator-() const requires is_effectively_absolute = delete;

    [[nodiscard]]
    constexpr quantity operator+() const
    {
      return quantity{this->values(), unit_type{}};
    }

    [[nodiscard]]
    constexpr quantity operator-() const noexcept(coordinates_type::has_identity_validator)
      requires (!is_effectively_absolute)
    {
      return quantity{to_array(this->values(), [](value_type t) { return -t; }), unit_type{}};
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
