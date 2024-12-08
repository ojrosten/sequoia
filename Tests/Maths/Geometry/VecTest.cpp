////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "VecTest.hpp"

#include "sequoia/Physics/Quantities.hpp"  // TO DO: relocate this

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <complex>

namespace sequoia::testing
{
  using namespace maths;
  using namespace physics; // TO DO: move to a different tests

  // TO DO: move these
  static_assert(has_plus_v<int>);
  static_assert(has_minus_v<int>);
  static_assert(has_multiply_v<int>);
  static_assert(has_divide_v<int>);

  static_assert(weakly_abelian_group_under_addition_v<int>);
  static_assert(weakly_abelian_group_under_addition_v<double>);
  static_assert(!weakly_abelian_group_under_multiplication_v<int>);
  static_assert(weakly_abelian_group_under_multiplication_v<double>);

  namespace
  {
    template<std::size_t D, std::floating_point T>
    struct world_displacements {};

    template<std::size_t D, std::floating_point T, class Atlas>
    struct world_vector_space
    {
      using set_type          = world_displacements<D, T>;
      using field_type        = T;
      using atlas_type        = Atlas;
      using vector_space_type = world_vector_space;
      constexpr static std::size_t dimension{D};

      template<maths::basis<world_vector_space> Basis>
        requires is_orthonormal_basis_v<Basis>
      [[nodiscard]]
      // We want Units^2
      friend constexpr field_type inner_product(const maths::vector_coordinates<world_vector_space, Basis>& lhs, const maths::vector_coordinates<world_vector_space, Basis>& rhs)
      {
        return std::ranges::fold_left(std::views::zip(lhs.values(), rhs.values()), field_type{}, [](field_type f, const auto& z){ return f + std::get<0>(z) * std::get<1>(z); });
      }
    };

    template<std::size_t D, std::floating_point T, class Units>
    struct world_affine_space
    {
      using set_type = sets::R<D, T>;
      using vector_space_type = world_vector_space<D, T, Units>;
    };

    template<std::size_t D, std::floating_point T, class Units, basis Basis, class Origin>
    using world_affine_coordinates = affine_coordinates<world_affine_space<D, T, Units>, Basis, Origin>;

    template<std::size_t D, std::floating_point T, class Units, basis Basis>
    using world_vector_coordinates = vector_coordinates<world_vector_space<D, T, Units>, Basis>;

    [[nodiscard]]
    std::weak_ordering to_ordering(coordinates_operations::dim_1_label From, coordinates_operations::dim_1_label To)
    {
      return From > To ? std::weak_ordering::less
           : From < To ? std::weak_ordering::greater
                       : std::weak_ordering::equivalent;
    }

    template<maths::network Graph, class Fn>
    void add_transition(Graph& g, coordinates_operations::dim_1_label From, coordinates_operations::dim_1_label To, std::string_view message, Fn f, std::weak_ordering ordering)
    {
      g.join(From, To, std::string{message}, f, ordering);
    }

    template<maths::network Graph, class Fn>
    void add_transition(Graph& g, coordinates_operations::dim_1_label From, coordinates_operations::dim_1_label To, std::string_view message, Fn f)
    {
      g.join(From, To, std::string{message}, f);
    }

    template<class VecCoords, maths::network Graph, class Fn>
      requires std::is_invocable_r_v<VecCoords, Fn, VecCoords>
    void add_transition(Graph& g, coordinates_operations::dim_1_label From, coordinates_operations::dim_1_label To, std::string_view message, Fn f)
    {
      using field_t = VecCoords::field_type;

      if constexpr(std::totally_ordered<field_t>)
      {
        add_transition(g, From, To, message, f, to_ordering(From, To));
      }
      else
      {
        add_transition(g, From, To, message, f);
      }
    }

    template<class VecCoords, maths::network Graph, class Fn>
      requires std::is_invocable_r_v<VecCoords, Fn, VecCoords>
    void add_transition(Graph& g, coordinates_operations::dim_2_label From, coordinates_operations::dim_2_label To, std::string_view message, Fn f)
    {
      g.join(From, To, std::string{message}, f);
    }

    template<class VecCoords, maths::network Graph>
    void add_dim_1_transitions(Graph& g, vec_test& t)
    {
      using vec_t   = VecCoords;
      using field_t = vec_t::field_type;

      // (1) --> (0)

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_1_label::one,
        coordinates_operations::dim_1_label::zero,
        t.report("(1) * field_t{}"),
        [](vec_t v) -> vec_t { return v * field_t{}; }
      );

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_1_label::one,
        coordinates_operations::dim_1_label::zero,
        t.report("field_t{} * (1)"),
        [](vec_t v) -> vec_t { return field_t{} *v; }
      );

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_1_label::one,
        coordinates_operations::dim_1_label::zero,
        t.report("(1) *= field_t{}"),
        [](vec_t v) -> vec_t { return v *= field_t{}; }
      );

      // (1) --> (2)

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_1_label::one,
        coordinates_operations::dim_1_label::two,
        t.report("(1) * field_t{2}"),
        [](vec_t v) -> vec_t { return v * field_t{2}; }
      );

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_1_label::one,
        coordinates_operations::dim_1_label::two,
        t.report("field_t{2} * (1)"),
        [](vec_t v) -> vec_t { return field_t{2} *v; }
      );

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_1_label::one,
        coordinates_operations::dim_1_label::two,
        t.report("(1) *= field_t{2}"),
        [](vec_t v) -> vec_t { return v *= field_t{2}; }
      );

      // (2) --> (1)

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_1_label::two,
        coordinates_operations::dim_1_label::one,
        t.report("(2) / field_t{2}"),
        [](vec_t v) -> vec_t { return v / field_t{2}; }
      );

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_1_label::two,
        coordinates_operations::dim_1_label::one,
        t.report("(2) /= field_t{2}"),
        [](vec_t v) -> vec_t { return v /= field_t{2}; }
      );
    }

    template<class VecCoords, maths::network Graph>
    void add_dim_2_transitions(Graph& g, vec_test& t)
    {
      using vec_t = VecCoords;
      using field_t = vec_t::field_type;

      // (-1, -1) --> (1, 1)

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_2_label::neg_one_neg_one,
        coordinates_operations::dim_2_label::one_one,
        t.report("(-1, -1) *= -1"),
        [](vec_t v) -> vec_t { return v *= field_t{-1}; }
      );

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_2_label::neg_one_neg_one,
        coordinates_operations::dim_2_label::one_one,
        t.report("(-1, -1) * -1"),
        [](vec_t v) -> vec_t { return v * field_t{-1}; }
      );

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_2_label::neg_one_neg_one,
        coordinates_operations::dim_2_label::one_one,
        t.report("(-1, -1) /= -1"),
        [](vec_t v) -> vec_t { return v /= field_t{-1}; }
      );

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_2_label::neg_one_neg_one,
        coordinates_operations::dim_2_label::one_one,
        t.report("(-1, -1) / -1"),
        [](vec_t v) -> vec_t { return v / field_t{-1}; }
      );
    }
  }

  [[nodiscard]]
  std::filesystem::path vec_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void vec_test::run_tests()
  {
    test_vec_1_orderable<sets::R<1, float>, float>();
    test_vec_1_orderable<sets::R<1,double>, double>();
    test_vec_1_unorderable<sets::C<1, float>, std::complex<float>>();

    test_vec_2<sets::R<1, float>, float>();
    test_vec_2<sets::C<2, double>, std::complex<double>>();
    test_vec_2<sets::C<1, double>, double>(); // Complex numbers over the reals

    test_real_vec_1_inner_prod<sets::R<1, float>, float>();
    test_complex_vec_1_inner_prod<sets::C<1, double>, std::complex<double>>();

    test_masses();
  }

  template<class Set, maths::weak_field Field>
  void vec_test::test_vec_1_orderable()
  {
    using vec_t = vector_coordinates<my_vec_space<Set, Field, 1>, canonical_basis<Set, Field, 1>>;
    auto g{coordinates_operations::make_dim_1_orderable_transition_graph<vec_t>()};

    add_dim_1_transitions<vec_t>(g, *this);

    auto checker{
      [this](std::string_view description, const vec_t& obtained, const vec_t& prediction, const vec_t& parent, std::weak_ordering ordering) {
        check(equality, description, obtained, prediction);
        if(ordering != std::weak_ordering::equivalent)
          check_semantics(description, prediction, parent, ordering);
      }
    };

    transition_checker<vec_t>::check(report(""), g, checker);
  }

  template<class Set, maths::weak_field Field>
  void vec_test::test_vec_1_unorderable()
  {
    using vec_t = vector_coordinates<my_vec_space<Set, Field, 1>, canonical_basis<Set, Field, 1>>;
    auto g{coordinates_operations::make_dim_1_unorderable_transition_graph<vec_t>()};

    add_dim_1_transitions<vec_t>(g, *this);

    auto checker{
        [this](std::string_view description, const vec_t& obtained, const vec_t& prediction, const vec_t& parent, std::size_t host, std::size_t target) {
          check(equality, description, obtained, prediction);
          if(host!= target) check_semantics(description, prediction, parent);
        }
    };

    transition_checker<vec_t>::check(report(""), g, checker);
  }

  template<class Set, maths::weak_field Field>
  void vec_test::test_vec_2()
  {
    using vec_t = vector_coordinates<my_vec_space<Set, Field, 2>, canonical_basis<Set, Field, 2>>;
    auto g{coordinates_operations::make_dim_2_transition_graph<vec_t>()};

    add_dim_2_transitions<vec_t>(g, *this);

    auto checker{
        [this](std::string_view description, const vec_t& obtained, const vec_t& prediction, const vec_t& parent, std::size_t host, std::size_t target) {
          check(equality, description, obtained, prediction);
          if(host != target) check_semantics(description, prediction, parent);
        }
    };

    transition_checker<vec_t>::check(report(""), g, checker);
  }

  template<class Set, std::floating_point Field>
  void vec_test::test_real_vec_1_inner_prod()
  {
    using vec_t = vector_coordinates<my_vec_space<Set, Field, 1>, canonical_basis<Set, Field, 1>>;

    static_assert(basis_for<canonical_basis<Set, Field, 1>, my_vec_space<Set, Field, 1>>);

    check(equality, "", inner_product(vec_t{}, vec_t{Field(1)}), Field{});
    check(equality, "", inner_product(vec_t{Field(1)}, vec_t{}), Field{});
    check(equality, "", inner_product(vec_t{Field(-1)}, vec_t{Field(1)}), Field{-1});
    check(equality, "", inner_product(vec_t{Field(1)}, vec_t{Field(-1)}), Field{-1});
    check(equality, "", inner_product(vec_t{Field(1)}, vec_t{Field(1)}), Field{1});
    check(equality, "", inner_product(vec_t{Field(-7)}, vec_t{Field(42)}), Field{-294});
  }

  template<class Set, class Field>
    requires is_complex_v<Field>
  void vec_test::test_complex_vec_1_inner_prod()
  {
    using vec_t = vector_coordinates<my_vec_space<Set, Field, 1>, canonical_basis<Set, Field, 1>>;

    check(equality, "", inner_product(vec_t{Field(1, 1)}, vec_t{Field(1, 1)}), Field{2});
    check(equality, "", inner_product(vec_t{Field(1, -1)}, vec_t{Field(1, 1)}), Field{0, 2});
  }

  void vec_test::test_masses()
  {
    using mass_t = quantity<mass_space<float>, scalar_atlas<quantity_sets::masses, units::kilogram_t, absolute_validator>>;
    // Independent info:
    // set2s::masses
    // sets2::mass_differences, but this could be derived from masses
    // unit
    // validator
    // but maybe the validator should be part of the unit type...

    mass_t m{2.0, units::kilogram};

    static_assert(atlas<scalar_atlas<quantity_sets::masses, units::kilogram_t, absolute_validator>>);
    static_assert(atlas_for<scalar_atlas<quantity_sets::masses, units::kilogram_t, absolute_validator>, quantity_sets::masses>);

  }
}
