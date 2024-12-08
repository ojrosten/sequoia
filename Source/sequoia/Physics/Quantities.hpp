////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/Maths/Geometry/VectorSpace.hpp"

namespace sequoia::physics
{
  using namespace maths;

  struct absolute_validator
  {
    template<std::floating_point T>
    [[nodiscard]]
    T operator()(const T val) const
    {
      if(val < T{}) throw std::domain_error{std::format("Value {} less than zero", val)};

      return val;
    }
  };

  struct coordinate_basis_type{};

  template<class VectorSpace, class Unit, std::floating_point T>
  struct quantity_displacement_basis
  {
    using vector_space_type      = VectorSpace;
    using unit_type              = Unit;
    using basis_orientation_type = coordinate_basis_type;
  };

  template<class T>
  concept quantity_space = requires {
    typename T::set_type;
    typename T::vector_space_type;
    requires vector_space<typename T::vector_space_type>;
  };

  template<class T>
  concept quantity_unit = requires {
    typename T::validator_type;
  };

  struct unchecked_t {};

  template<quantity_space QuantitySpace, quantity_unit Unit>
  class quantity
  {
  public:
    using quantity_space_type     = QuantitySpace;
    using unit_type               = Unit;
    using displacement_space_type = typename QuantitySpace::vector_space_type;
    using validator_type          = typename Unit::validator_type;
    using field_type              = typename displacement_space_type::field_type;
    using value_type              = field_type;

    quantity(value_type val, unit_type) : m_Value{m_Validator(val)} {}

    quantity(unchecked_t, value_type val, unit_type)
      : m_Value{val}
    {
    }


    [[nodiscard]]
    const value_type& value() const noexcept { return m_Value; }

    [[nodiscard]]
    const validator_type& validator() const noexcept { return m_Validator; }

    [[nodiscard]]
    friend bool operator==(const quantity& lhs, const quantity& rhs) noexcept { return lhs.value() == rhs.value(); }

    [[nodiscard]]
    friend auto operator<=>(const quantity& lhs, const quantity& rhs) noexcept { return lhs.value() <=> rhs.value(); }
  private:
    SEQUOIA_NO_UNIQUE_ADDRESS validator_type m_Validator;
    value_type m_Value;
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