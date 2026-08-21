////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/Maths/Geometry/Spaces.hpp"

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <complex>

namespace sequoia::testing
{
  template<class T, class U>
  inline constexpr bool can_multiply{
    requires(const T& t, const U& u) { t * u; }
  };

  template<class T, class U>
  inline constexpr bool can_divide{
    requires(const T& t, const U& u) { t / u; }
  };

  template<class T, class U>
  inline constexpr bool can_add{
    requires(const T& t, const U& u) { t + u; }
  };

  template<class T, class U>
  inline constexpr bool can_subtract{
    requires(const T& t, const U& u) { t - u; }
  };

  template<class T, class U>
  inline constexpr bool rvalue_addition_combinable{
    requires(T&& t, const U& u) {
      std::move(t) += u;
    }
  };

  template<class T, class U>
  inline constexpr bool addition_combinable{
    !rvalue_addition_combinable<T, U>
    && requires(T& t, const U& u) { { t += u } -> std::convertible_to<T>; }
  };

  template<class T, class U>
  inline constexpr bool rvalue_subtraction_combinable{
    requires(T&& t, const U& u) {
      std::move(t) -= u;
    }
  };

  template<class T, class U>
  inline constexpr bool subtraction_combinable{
       !rvalue_subtraction_combinable<T, U>
    && requires(T& t, const U& u) { { t -= u } -> std::convertible_to<T>; }
  };

  template<class T>
  inline constexpr bool has_unary_plus{
    requires(const T& t) { { +t } -> std::convertible_to<T>; }
  };

  template<class T>
  inline constexpr bool has_unary_minus{
    requires(const T& t) { { -t } -> std::convertible_to<T>; }
  };
  
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
  
  template<class Set, class Field, std::size_t Dim>
    requires maths::identifies_as_field_v<Field>
  struct my_vec_space
  {
    using set_type               = Set;
    using field_type             = Field;
    using structure              = maths::vector_space_tag_t;
    using admits_canonical_basis = std::true_type;
    constexpr static std::size_t dimension{Dim};
    constexpr static std::size_t D{dimension};

    template<
      maths::basis Basis,
      maths::representation_for<my_vec_space> Representation,
      maths::validator_for<my_vec_space, Representation> Validator,
      std::floating_point ValType=Representation::value_type
    >
    // TO DO:  requires is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr ValType inner_product(
      const maths::vector_coordinates<my_vec_space, Basis, Representation, Validator>& lhs,
      const maths::vector_coordinates<my_vec_space, Basis, Representation, Validator>& rhs
    )
    {
      using value_t = ValType;

      return
        std::ranges::fold_left(
          std::views::zip(lhs.values(), rhs.values()), // TO DO: use Representation
          value_t{},
          [](value_t f, const auto& z){ return f + std::get<0>(z) * std::get<1>(z); }
       );
    }

    template<
      maths::basis Basis,
      maths::representation_for<my_vec_space> Representation,
      maths::validator_for<my_vec_space, Representation> Validator,
      class ValType=Representation::value_type
    >
    requires is_complex_v<ValType> // TO DO && is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr ValType inner_product(
      const maths::vector_coordinates<my_vec_space, Basis, Representation, Validator>& lhs,
      const maths::vector_coordinates<my_vec_space, Basis, Representation, Validator>& rhs
    )
    {
      using value_t = ValType;

      return
        std::ranges::fold_left(
          std::views::zip(lhs.values(), rhs.values()), // TO DO: use Representation
          value_t{},
          [](value_t f, const auto& z){ return f + conj(std::get<0>(z)) * std::get<1>(z); });
    }
  };

  template<class Set, class Field, std::size_t D>
    requires maths::identifies_as_field_v<Field>
  struct my_affine_space
  {
    using set_type          = Set;
    using vector_space_type = my_vec_space<Set, Field, D>;
    using structure         = maths::affine_space_tag_t;
  };

  template<class Set, class Ring, std::size_t D>
  struct my_module
  {
    using set_type               = Set;
    using commutative_ring_type  = Ring;
    using structure              = maths::free_module_tag_t;
    using admits_canonical_basis = std::true_type;
    constexpr static std::size_t rank{D};
  };

  /*! The action of the free module is total, exactly as for an affine space, but
      the ring is deliberately not a field: this is the case which only the
      M-affine concept admits.
   */
  template<class Set, class Ring, std::size_t D>
    requires (!maths::identifies_as_field_v<Ring>)
  struct my_m_affine_space
  {
    using set_type         = Set;
    using free_module_type = my_module<Set, Ring, D>;
    using structure        = maths::m_affine_space_tag_t;
  };

  template<maths::partial_m_torsor Space, maths::basis Basis, class... Ts>
    requires maths::basis_for<Basis, maths::free_module_type_of_t<Space>>
  struct value_tester<maths::coordinates<Space, Basis, Ts...>>
  {
    using coord_type = maths::coordinates<Space, Basis, Ts...>;
    using displacement_value_type = typename coord_type::displacement_value_type;
    constexpr static std::size_t D{coord_type::dimension};

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const coord_type& actual, const coord_type& prediction)
    {
      check(equality, "Wrapped values", logger, actual.values(), prediction.values());
      if constexpr(D == 1)
      {
        check(equality, "Wrapped value", logger, actual.value(), prediction.value());
        if constexpr(std::convertible_to<displacement_value_type, bool>)
          check(equality, "Conversion to bool", logger, static_cast<bool>(actual), static_cast<bool>(prediction));
      }

      for(auto i : std::views::iota(0uz, D))
      {
        check(equality, std::format("Value at index {}", i), logger, actual[i], prediction[i]);
      }
    }

    template<test_mode Mode, maths::weak_commutative_ring RingRep>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const coord_type& actual, const std::array<RingRep, D>& prediction)
    {
      check(equality, "Wrapped values", logger, actual.values(), std::span<const RingRep, D>{prediction});
      check(equivalence, "Iterators",    logger, std::ranges::subrange{actual.begin(),  actual.end()},    prediction);
      check(equivalence, "c-Iterators",  logger, std::ranges::subrange{actual.cbegin(),  actual.cend()},  prediction);
      check(equivalence, "r-Iterators",  logger, std::ranges::subrange{actual.rbegin(),  actual.rend()},  prediction);
      check(equivalence, "cr-Iterators", logger, std::ranges::subrange{actual.crbegin(), actual.crend()}, prediction);

      for(auto i : std::views::iota(0uz, D))
      {
        check(equality, "operator[]", logger, actual[i], prediction[i]);
      }
    }
  };

  /*! Helper functions for building state-transition graphs*/

  enum class inverted_ordering : bool {no, yes};

  template<class Label>
    requires std::convertible_to<Label, std::size_t>
  [[nodiscard]]
  std::weak_ordering to_ordering(Label From, Label To, inverted_ordering invert)
  {
    const bool inverted{invert == inverted_ordering::yes};
    return (((From < To) && !inverted) || ((From > To) && inverted)) ? std::weak_ordering::less
         : (((From > To) && !inverted) || ((From < To) && inverted)) ? std::weak_ordering::greater
                                                                     : std::weak_ordering::equivalent;
  }

  namespace impl
  {
    template<maths::network Graph, class Label, class Fn>
      requires std::convertible_to<Label, std::size_t>
    void do_add_transition(Graph& g, Label From, Label To, std::string_view message, Fn f, std::weak_ordering ordering)
    {
      g.join(From, To, std::string{message}, f, ordering);
    }

    template<maths::network Graph, class Label, class Fn>
      requires std::convertible_to<Label, std::size_t>
    void do_add_transition(Graph& g, Label From, Label To, std::string_view message, Fn f)
    {
      g.join(From, To, std::string{message}, f);
    }
  }

  template<class Coords, maths::network Graph, class Label, class Fn>
    requires std::convertible_to<Label, std::size_t>
  void add_transition(Graph& g, Label From, Label To, std::string_view message, Fn f, inverted_ordering invert={})
  {
    using disp_value_t = Coords::displacement_value_type;
    constexpr static auto dimension{Coords::dimension};

    if constexpr((dimension == 1) && std::totally_ordered<disp_value_t>)
    {
      impl::do_add_transition(g, From, To, message, f, to_ordering(From, To, invert));
    }
    else
    {
      impl::do_add_transition(g, From, To, message, f);
    }
  }
}
