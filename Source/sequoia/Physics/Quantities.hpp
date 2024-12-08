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

  struct absolute_validator
  {
    template<std::floating_point T>
    [[nodiscard]]
    constexpr T operator()(const T val) const
    {
      if(val < T{}) throw std::domain_error{std::format("Value {} less than zero", val)};

      return val;
    }
  };

  template<class T>
  inline constexpr bool is_identity_validator_v{std::is_same_v<T, std::identity>};

  template<class T>
  struct defines_absolute_scale : std::false_type {};

  template<class T>
  using defines_absolute_scale_t = typename defines_absolute_scale<T>::type;

  template<class T>
  inline constexpr bool defines_absolute_scale_v{defines_absolute_scale<T>::value};

  template<>
  struct defines_absolute_scale<absolute_validator> : std::true_type {};

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

  struct unchecked_t {};

  template<convex_space QuantitySpace, quantity_unit Unit>
  class quantity
  {
  public:
    using quantity_space_type     = QuantitySpace;
    using unit_type               = Unit;
    using displacement_space_type = typename QuantitySpace::vector_space_type;
    using validator_type          = typename Unit::validator_type;
    using field_type              = typename displacement_space_type::field_type;
    using value_type              = field_type;

    constexpr static std::size_t dimension{displacement_space_type::dimension};
    constexpr static std::size_t D{dimension};

    constexpr static bool is_absolute{(dimension == 1) && defines_absolute_scale_v<validator_type>};

    constexpr quantity(value_type val, unit_type) requires (D==1)
      : m_Values{m_Validator(val)}
    {}

    constexpr quantity(unchecked_t, value_type val, unit_type) requires (D==1)
      : m_Values{val}
    {}

    [[nodiscard]]
    constexpr const value_type& value() const noexcept requires (D==1) { return m_Values[0]; }

    [[nodiscard]]
    constexpr const validator_type& validator() const noexcept { return m_Validator; }

    [[nodiscard]]
    constexpr friend bool operator==(const quantity& lhs, const quantity& rhs) noexcept { return lhs.m_Values == rhs.m_Values; }

    [[nodiscard]]
    constexpr friend auto operator<=>(const quantity& lhs, const quantity& rhs) noexcept
      requires (D == 1) && std::totally_ordered<value_type>
    {
      return lhs.value() <=> rhs.value();
    }
  private:
    SEQUOIA_NO_UNIQUE_ADDRESS validator_type m_Validator;
    std::array<value_type, D> m_Values{};
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