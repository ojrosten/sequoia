////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Maths/Geometry/VectorSpace.hpp"

#include <complex>

namespace sequoia::testing
{
  template<class T>
  struct is_complex : std::false_type {};

  template<std::floating_point T>
  struct is_complex<std::complex<T>> : std::true_type {};

  template<class T>
  inline constexpr bool is_complex_v{is_complex<T>::value};

  template<class T>
  using is_complex_t = typename is_complex<T>::type;

  template<class B>
  inline constexpr bool is_orthonormal_basis_v{
    requires {
      typename B::orthonormal;
      requires std::same_as<typename B::orthonormal, std::true_type>;
    }
  };

  template<class Space>
  inline constexpr bool is_space_v{
    requires {
      typename Space::field_type;
      { Space::dimension } -> std::convertible_to<std::size_t>;
    }
  };


  template<class Element, maths::field Field, std::size_t D>
  struct my_vec_space
  {
    using element_type = Element;
    using field_type = Field;
    constexpr static std::size_t dimension{D};

    template<class Basis>
      requires std::floating_point<Field>&& is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr Field inner_product(const maths::vector_coordinates<my_vec_space, Basis>& lhs, const maths::vector_coordinates<my_vec_space, Basis>& rhs)
    {
      return std::ranges::fold_left(std::views::zip(lhs.values(), rhs.values()), Field{}, [](Field f, const auto& z){ return f + std::get<0>(z) * std::get<1>(z); });
    }

    template<class Basis>
      requires is_complex_v<Field>&& is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr Field inner_product(const maths::vector_coordinates<my_vec_space, Basis>& lhs, const maths::vector_coordinates<my_vec_space, Basis>& rhs)
    {
      return std::ranges::fold_left(std::views::zip(lhs.values(), rhs.values()), Field{}, [](Field f, const auto& z){ return f + conj(std::get<0>(z)) * std::get<1>(z); });
    }
  };


  template<class Element, maths::field Field, std::size_t D>
  struct canonical_basis
  {
    using orthonormal = std::true_type;
  };

  template<class Space>
    requires is_space_v<Space>
  struct geometrical_space_tester
  {
    using type         = Space;
    using field_type   = typename Space::field_type;

    constexpr static std::size_t D{Space::dimension};

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const type& actual, const type& prediction)
    {
      check(equality, "Wrapped values", logger, actual.values(), prediction.values());
      if constexpr(D == 1)
      {
        check(equality, "Wrapped value", logger, actual.value(), prediction.value());
        if constexpr(std::convertible_to<field_type, bool>)
          check(equality, "Conversion to bool", logger, static_cast<bool>(actual), static_cast<bool>(prediction));
      }

      for(auto i : std::views::iota(0_sz, D))
      {
        check(equality, std::format("Value at index {}", i), logger, actual[i], prediction[i]);
      }
    }

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& actual, const std::array<field_type, D>& prediction)
    {
      check(equality, "Wrapped values", logger, actual.values(), std::span<const field_type, D>{prediction});
    }
  };
}
