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

  template<class T>
  struct difference_space
  {
  };


  namespace sets2
  {
    struct masses
    {
      using topological_space_type = std::true_type;
    };

    //struct mass_differences {};
  }

  namespace units
  {
    struct kilogram_t{};

    inline constexpr kilogram_t kilogram{};
  }

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

  // Maybe drop topological space and trade atlas for coordinate system?
  template<class TopologicalSpace, class Unit, class Validator>
  struct scalar_atlas
  {
    using topological_space_type = TopologicalSpace;
    using unit_type = Unit;
    using validator_type = Validator;
    constexpr static std::size_t dimension{1};

    template<std::floating_point T>
    [[nodiscard]]
    T validate(const T val) const
    {
      return validator(val);
    }

    Validator validator{};
  };

  /*template<class Set, std::floating_point T>
  struct displacement_space
  {
    using set_type = Set;
    using field_type = T;
    using vector_space_type = displacement_space;
    constexpr static std::size_t dimension{1};
  };

  template<class Set, std::floating_point T>
  struct space
  {
    using set_type          = Set;
    using vector_space_type = displacement_space<difference_space<Set>, T>;
  };*/

  struct coordinate_basis_type{};

  template<class VectorSpace, class Unit, std::floating_point T>
  struct quantity_displacement_basis
  {
    using vector_space_type = VectorSpace;
    using unit_type = Unit;
    using basis_alignment_type = coordinate_basis_type;
  };


  template<std::floating_point T>
  struct mass_displacement_space
  {
    using set_type = difference_space<sets2::masses>;
    using field_type = T;
    using vector_space_type = mass_displacement_space;
    constexpr static std::size_t dimension{1};
  };

  template<std::floating_point T>
  struct mass_space
  {
    using set_type = sets2::masses;
    using vector_space_type = mass_displacement_space<T>;
  };

  /*
  template<class Unit, std::floating_point T>
  struct mass_displacement_basis
  {
    using unit_type            = Unit;
    using vector_space_type    = mass_displacement_space<T>;
    using basis_alignment_type = coordinate_basis_type;
  };*/

  template<class T>
  inline constexpr bool has_unit_type_v{requires { typename T::unit_type; }};


  template<class T>
  concept atlas = requires {
    typename T::topological_space_type;
    typename T::unit_type;
    { T::dimension } -> std::convertible_to<std::size_t>;
  };


  template<atlas A, class S>
  inline constexpr bool atlas_for{std::is_same_v<typename A::topological_space_type, S>};

  static_assert(atlas<scalar_atlas<sets2::masses, units::kilogram_t, absolute_validator>>);
  static_assert(atlas_for<scalar_atlas<sets2::masses, units::kilogram_t, absolute_validator>, sets2::masses>);

  template<class T>
  concept quantity_space = requires {
    typename T::set_type;
    typename T::vector_space_type;
      requires vector_space<typename T::vector_space_type>;
  };

  static_assert(quantity_space<mass_space<float>>);

  struct unchecked_t {};

  template<quantity_space QuantitySpace, atlas Atlas>
  struct to_default_basis
  {
    using type = quantity_displacement_basis<typename QuantitySpace::vector_space_type, typename Atlas::unit_type, typename QuantitySpace::vector_space_type::field_type>;
  };

  template<quantity_space QuantitySpace, atlas Atlas>
  using to_default_basis_t = typename to_default_basis<QuantitySpace, Atlas>::type;


  //template<quantity_space QuantitySpace, class Unit, class Validator>
  template<quantity_space QuantitySpace, atlas Atlas, basis Basis = to_default_basis_t<QuantitySpace, Atlas>>
    requires  atlas_for<Atlas, typename QuantitySpace::set_type>
  && basis_for<Basis, typename QuantitySpace::vector_space_type>
    && has_unit_type_v<Basis>
    && std::is_same_v<typename Atlas::unit_type, typename Basis::unit_type> // this could be relaxed to allow e.g. m + cm
    class quantity
  {
  public:
    using quantity_space_type = QuantitySpace;
    using displacement_space_type = typename QuantitySpace::vector_space_type;
    using atlas_type = Atlas;
    using validator_type = typename Atlas::validator_type;
    using unit_type = typename Atlas::unit_type;
    using field_type = typename displacement_space_type::field_type;
    using value_type = field_type;

    quantity(value_type val, unit_type) : m_Value{m_Atlas.validate(val)} {}

    quantity(unchecked_t, value_type val, unit_type)
      : m_Value{val}
    {
    }


    [[nodiscard]]
    const value_type& value() const noexcept { return m_Value; }

    [[nodiscard]]
    const atlas_type& atlas() const noexcept { return m_Atlas; }

    [[nodiscard]]
    friend bool operator==(const quantity& lhs, const quantity& rhs) noexcept { return lhs.value() == rhs.value(); }

    [[nodiscard]]
    friend auto operator<=>(const quantity& lhs, const quantity& rhs) noexcept { return lhs.value() <=> rhs.value(); }
  private:
    SEQUOIA_NO_UNIQUE_ADDRESS atlas_type m_Atlas;
    value_type m_Value;
  };
}