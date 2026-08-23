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
    using structure         = maths::m_affine_space_tag_t;
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

  /*! Origins for coordinates on an M-affine space, and hence on an affine space
      or a vector space. Two are provided so that transformations between them
      may be exercised; they carry no state, an origin being nothing more than a
      choice.
   */

  struct alice {};

  struct bob {};

  /*! @brief Whether a coordinates type is expected to support a given operation.

      Three-valued, rather than a bool, so that the default is `unstated`: a row
      omitted from an operator_expectations aggregate is a compile-time error
      naming that row, rather than a silent `no`.
   */

  enum class admits : char { unstated, no, yes };

  /*! @brief The complete set of arithmetic operations a coordinates type may support.

      Every coordinates test states every row. That is the whole point: before
      this existed, the affine and M-affine cases each checked a different subset
      and the divergence was invisible.

      Note what this type deliberately does not do. It holds no logic of its own -
      no deduction of one row from another, and above all no consultation of the
      concepts in Spaces.hpp. Each call site simply asserts the fifteen answers it
      expects. Were the expectations instead computed from, say, `field<...>`, a
      broken concept would revise the expectation to match itself and the checks
      would stay green; the test's fidelity would then rest on the correctness of
      the very code under test.

      Throughout, `point` is the coordinates type, `displacement` its associated
      displacement coordinates type - the two coincide for a free module or a
      vector space - and `scalar` the representation's value type.
   */

  struct operator_expectations
  {
    admits point_plus_point                {};
    admits point_plus_displacement         {};
    admits point_minus_point               {};
    admits point_minus_displacement        {};
    admits point_unary_plus                {};
    admits point_unary_minus               {};
    admits point_times_scalar              {};
    admits point_over_scalar               {};
    admits point_over_point                {};
    admits point_over_displacement         {};
    admits displacement_over_point         {};
    admits displacement_times_scalar       {};
    admits displacement_over_scalar        {};
    admits displacement_over_displacement  {};
    admits displacement_unary_minus        {};
  };

  /*! @brief Checks a coordinates type against every row of operator_expectations.

      Each row is a static_assert, so a mismatch is caught at compile time; each
      is additionally registered as a check, with a description naming the
      operation, so that the count reflects what was verified.
   */

  template<class Coordinates, operator_expectations Expected>
  class operator_checks
  {
    using point_t        = Coordinates;
    using displacement_t = Coordinates::displacement_coordinates_type;
    using scalar_t       = Coordinates::representation_type::value_type;

    regular_test& m_Test;
  public:
    explicit operator_checks(regular_test& t)
      : m_Test{t}
    {}

    void execute()
    {
      check_row<Expected.point_plus_point,               can_add<point_t, point_t>>              ("point + point");
      check_row<Expected.point_plus_displacement,        can_add<point_t, displacement_t>>       ("point + displacement");
      check_row<Expected.point_minus_point,              can_subtract<point_t, point_t>>         ("point - point");
      check_row<Expected.point_minus_displacement,       can_subtract<point_t, displacement_t>>  ("point - displacement");
      check_row<Expected.point_unary_plus,               has_unary_plus<point_t>>                ("+point");
      check_row<Expected.point_unary_minus,              has_unary_minus<point_t>>               ("-point");
      check_row<Expected.point_times_scalar,             can_multiply<point_t, scalar_t>>        ("point * scalar");
      check_row<Expected.point_over_scalar,              can_divide<point_t, scalar_t>>          ("point / scalar");
      check_row<Expected.point_over_point,               can_divide<point_t, point_t>>           ("point / point");
      check_row<Expected.point_over_displacement,        can_divide<point_t, displacement_t>>    ("point / displacement");
      check_row<Expected.displacement_over_point,        can_divide<displacement_t, point_t>>    ("displacement / point");
      check_row<Expected.displacement_times_scalar,      can_multiply<displacement_t, scalar_t>> ("displacement * scalar");
      check_row<Expected.displacement_over_scalar,       can_divide<displacement_t, scalar_t>>   ("displacement / scalar");
      check_row<Expected.displacement_over_displacement, can_divide<displacement_t, displacement_t>>("displacement / displacement");
      check_row<Expected.displacement_unary_minus,       has_unary_minus<displacement_t>>        ("-displacement");
    }
  private:
    template<admits Row, bool Actual>
    void check_row(std::string_view description)
    {
      static_assert(Row != admits::unstated, "Every row of operator_expectations must be stated");
      static_assert((Row == admits::yes) == Actual, "Operator availability differs from the stated expectation");

      m_Test.check(description, true);
    }
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
