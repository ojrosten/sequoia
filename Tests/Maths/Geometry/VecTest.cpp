////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "VecTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <complex>

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
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
    void add_dim_1_transitions(Graph& g)
    {
      using vec_t   = VecCoords;
      using field_t = vec_t::field_type;

      // (1) --> (0)

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_1_label::one,
        coordinates_operations::dim_1_label::zero,
        report_line("(1) * field_t{}"),
        [](vec_t v) -> vec_t { return v * field_t{}; }
      );

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_1_label::one,
        coordinates_operations::dim_1_label::zero,
        report_line("field_t{} * (1)"),
        [](vec_t v) -> vec_t { return field_t{} *v; }
      );

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_1_label::one,
        coordinates_operations::dim_1_label::zero,
        report_line("(1) *= field_t{}"),
        [](vec_t v) -> vec_t { return v *= field_t{}; }
      );

      // (1) --> (2)

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_1_label::one,
        coordinates_operations::dim_1_label::two,
        report_line("(1) * field_t{2}"),
        [](vec_t v) -> vec_t { return v * field_t{2}; }
      );

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_1_label::one,
        coordinates_operations::dim_1_label::two,
        report_line("field_t{2} * (1)"),
        [](vec_t v) -> vec_t { return field_t{2} *v; }
      );

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_1_label::one,
        coordinates_operations::dim_1_label::two,
        report_line("(1) *= field_t{2}"),
        [](vec_t v) -> vec_t { return v *= field_t{2}; }
      );

      // (2) --> (1)

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_1_label::two,
        coordinates_operations::dim_1_label::one,
        report_line("(2) / field_t{2}"),
        [](vec_t v) -> vec_t { return v / field_t{2}; }
      );

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_1_label::two,
        coordinates_operations::dim_1_label::one,
        report_line("(2) /= field_t{2}"),
        [](vec_t v) -> vec_t { return v /= field_t{2}; }
      );
    }

    template<class VecCoords, maths::network Graph>
    void add_dim_2_transitions(Graph& g)
    {
      using vec_t = VecCoords;
      using field_t = vec_t::field_type;

      // (-1, -1) --> (1, 1)

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_2_label::neg_one_neg_one,
        coordinates_operations::dim_2_label::one_one,
        report_line("(-1, -1) *= -1"),
        [](vec_t v) -> vec_t { return v *= field_t{-1}; }
      );

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_2_label::neg_one_neg_one,
        coordinates_operations::dim_2_label::one_one,
        report_line("(-1, -1) * -1"),
        [](vec_t v) -> vec_t { return v * field_t{-1}; }
      );

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_2_label::neg_one_neg_one,
        coordinates_operations::dim_2_label::one_one,
        report_line("(-1, -1) /= -1"),
        [](vec_t v) -> vec_t { return v /= field_t{-1}; }
      );

      add_transition<vec_t>(
        g,
        coordinates_operations::dim_2_label::neg_one_neg_one,
        coordinates_operations::dim_2_label::one_one,
        report_line("(-1, -1) / -1"),
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
    test_vec_1_orderable<float, float>();
    test_vec_1_orderable<double, double>();
    test_vec_1_unorderable<std::complex<float>, std::complex<float>>();

    test_vec_2<std::array<float, 2>, float>();
    test_vec_2<std::array<std::complex<double>, 2>, std::complex<double>>();
    test_vec_2<std::complex<double>, double>(); // Complex numbers over the reals

    test_real_vec_1_inner_prod<float, float>();
    test_complex_vec_1_inner_prod<std::complex<double>, std::complex<double>>();
  }

  template<class Element, maths::field Field>
  void vec_test::test_vec_1_orderable()
  {
    using vec_t = vector_coordinates<my_vec_space<Element, Field, 1>, canonical_basis<Element, Field, 1>>;
    auto g{coordinates_operations::make_dim_1_orderable_transition_graph<vec_t>()};

    add_dim_1_transitions<vec_t>(g);

    auto checker{
      [this](std::string_view description, const vec_t& obtained, const vec_t& prediction, const vec_t& parent, std::weak_ordering ordering) {
        check(equality, description, obtained, prediction);
        if(ordering != std::weak_ordering::equivalent)
          check_semantics(description, prediction, parent, ordering);
      }
    };

    transition_checker<vec_t>::check(report_line(""), g, checker);
  }

  template<class Element, maths::field Field>
  void vec_test::test_vec_1_unorderable()
  {
    using vec_t = vector_coordinates<my_vec_space<Element, Field, 1>, canonical_basis<Element, Field, 1>>;
    auto g{coordinates_operations::make_dim_1_unorderable_transition_graph<vec_t>()};

    add_dim_1_transitions<vec_t>(g);

    auto checker{
        [this](std::string_view description, const vec_t& obtained, const vec_t& prediction, const vec_t& parent, std::size_t host, std::size_t target) {
          check(equality, description, obtained, prediction);
          if(host!= target) check_semantics(description, prediction, parent);
        }
    };

    transition_checker<vec_t>::check(report_line(""), g, checker);
  }

  template<class Element, maths::field Field>
  void vec_test::test_vec_2()
  {
    using vec_t = vector_coordinates<my_vec_space<Element, Field, 2>, canonical_basis<Element, Field, 2>>;
    auto g{coordinates_operations::make_dim_2_transition_graph<vec_t>()};

    add_dim_2_transitions<vec_t>(g);

    auto checker{
        [this](std::string_view description, const vec_t& obtained, const vec_t& prediction, const vec_t& parent, std::size_t host, std::size_t target) {
          check(equality, description, obtained, prediction);
          if(host != target) check_semantics(description, prediction, parent);
        }
    };

    transition_checker<vec_t>::check(report_line(""), g, checker);
  }

  template<class Element, std::floating_point Field>
  void vec_test::test_real_vec_1_inner_prod()
  {
    using vec_t = vector_coordinates<my_vec_space<Element, Field, 1>, canonical_basis<Element, Field, 1>>;

    check(equality, report_line(""), inner_product(vec_t{}, vec_t{Field(1)}), Field{});
    check(equality, report_line(""), inner_product(vec_t{Field(1)}, vec_t{}), Field{});
    check(equality, report_line(""), inner_product(vec_t{Field(-1)}, vec_t{Field(1)}), Field{-1});
    check(equality, report_line(""), inner_product(vec_t{Field(1)}, vec_t{Field(-1)}), Field{-1});
    check(equality, report_line(""), inner_product(vec_t{Field(1)}, vec_t{Field(1)}), Field{1});
    check(equality, report_line(""), inner_product(vec_t{Field(-7)}, vec_t{Field(42)}), Field{-294});
  }

  template<class Element, class Field>
    requires is_complex_v<Field>
  void vec_test::test_complex_vec_1_inner_prod()
  {
    using vec_t = vector_coordinates<my_vec_space<Element, Field, 1>, canonical_basis<Element, Field, 1>>;

    check(equality, report_line(""), inner_product(vec_t{Field(1, 1)}, vec_t{Field(1, 1)}), Field{2});
    check(equality, report_line(""), inner_product(vec_t{Field(1, -1)}, vec_t{Field(1, 1)}), Field{0, 2});
  }
}
