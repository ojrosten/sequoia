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
    enum vec_1_label{ neg_one, zero, one, two };
    enum vec_2_label{ neg_one_neg_one, neg_one_zero, zero_neg_one, zero_zero, zero_one, one_zero, one_one};
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

    // (1) --> (0)

    g.join(
      coordinates_operations::dim_1_label::one,
      coordinates_operations::dim_1_label::zero,
      report_line("(1) * Field{}"),
      [](vec_t v) -> vec_t { return v * Field{};}, 
      std::weak_ordering::less
    );

    g.join(
      coordinates_operations::dim_1_label::one,
      coordinates_operations::dim_1_label::zero,
      report_line("Field{} * (1)"),
      [](vec_t v) -> vec_t { return Field{} * v; },
      std::weak_ordering::less
    );

    g.join(
      coordinates_operations::dim_1_label::one,
      coordinates_operations::dim_1_label::zero,
      report_line("(1) *= Field{}"),
      [](vec_t v) -> vec_t { return v *= Field{}; },
      std::weak_ordering::less
    );

    // (1) --> (2)

    g.join(
      coordinates_operations::dim_1_label::one,
      coordinates_operations::dim_1_label::two,
      report_line("(1) * Field{2}"),
      [](vec_t v) -> vec_t { return v * Field{2}; },
      std::weak_ordering::greater
    );

    g.join(
      coordinates_operations::dim_1_label::one,
      coordinates_operations::dim_1_label::two,
      report_line("Field{2} * (1)"),
      [](vec_t v) -> vec_t { return Field{2} * v; },
      std::weak_ordering::greater
    );

    g.join(
      coordinates_operations::dim_1_label::one,
      coordinates_operations::dim_1_label::two,
      report_line("(1) *= Field{2}"),
      [](vec_t v) -> vec_t { return v *= Field{2}; },
      std::weak_ordering::greater
    );

    // (2) --> (1)

    g.join(
      coordinates_operations::dim_1_label::two,
      coordinates_operations::dim_1_label::one,
      report_line("(2) / Field{2}"),
      [](vec_t v) -> vec_t { return v / Field{2}; },
      std::weak_ordering::less
    );

    g.join(
      coordinates_operations::dim_1_label::two,
      coordinates_operations::dim_1_label::one,
      report_line("(2) /= Field{2}"),
      [](vec_t v) -> vec_t { return v /= Field{2}; },
      std::weak_ordering::less
    );

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
    using vec_t     = vector_coordinates<my_vec_space<Element, Field, 1>, canonical_basis<Element, Field, 1>>;
    using vec_graph = transition_checker<vec_t>::transition_graph;
    using edge_t    = transition_checker<vec_t>::edge;

    vec_graph g{
      {
        {
          edge_t{vec_1_label::one,     "- (-1)",  [](vec_t v) -> vec_t { return -v;  }},
          edge_t{vec_1_label::neg_one, "+ (-1)",  [](vec_t v) -> vec_t { return +v;  }}
        }, // neg_one
        {
          edge_t{vec_1_label::one, "(0) + (1)",  [](vec_t v) -> vec_t { return v + vec_t{Field(1)};  }},
          edge_t{vec_1_label::one, "(0) += (1)", [](vec_t v) -> vec_t { return v += vec_t{Field(1)}; }}
        }, // zero
        {
          edge_t{vec_1_label::neg_one, "-(1)",            [](vec_t v) -> vec_t { return -v;                   }},
          edge_t{vec_1_label::zero,    "(1) - (1)",       [](vec_t v) -> vec_t { return v -  vec_t{Field(1)}; }},
          edge_t{vec_1_label::zero,    "(1) -= (1)",      [](vec_t v) -> vec_t { return v -= vec_t{Field(1)}; }},
          edge_t{vec_1_label::zero,    "(1) * Field{}",   [](vec_t v) -> vec_t { return v * Field{};          }},
          edge_t{vec_1_label::zero,    "Field{} * (1)",   [](vec_t v) -> vec_t { return Field{} *v;           }},
          edge_t{vec_1_label::zero,    "(1) *= Field{}",  [](vec_t v) -> vec_t { return v *= Field{};         }},
          edge_t{vec_1_label::one,     "+(1)",            [](vec_t v) -> vec_t { return +v;                   }},
          edge_t{vec_1_label::two,     "(1) + (1)",       [](vec_t v) -> vec_t { return v + v;                }},
          edge_t{vec_1_label::two,     "(1) += (1)",      [](vec_t v) -> vec_t { return v += v;               }},
          edge_t{vec_1_label::two,     "(1) * Field(2)",  [](vec_t v) -> vec_t { return v *  Field(2);        }},
          edge_t{vec_1_label::two,     "(1) *= Field(2)", [](vec_t v) -> vec_t { return v *= Field(2);        }},
          edge_t{vec_1_label::two,     "Field(2) * (1)",  [](vec_t v) -> vec_t { return Field(2) * v;         }}
        }, // one
        {
          edge_t{vec_1_label::one, "(2) / Field(2)",  [](vec_t v) -> vec_t { return v / Field(2);        }},
          edge_t{vec_1_label::one, "(2) /= Field(2)", [](vec_t v) -> vec_t { return v /= Field(2);       }},
          edge_t{vec_1_label::one, "(2) - (1)",       [](vec_t v) -> vec_t { return v - vec_t{Field(1)}; }}
        }, // two
      },
      {vec_t{Field(-1)}, vec_t{}, vec_t{Field(1)}, vec_t{Field(2)}}
    };

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
    using vec_graph = transition_checker<vec_t>::transition_graph;
    using edge_t = transition_checker<vec_t>::edge;

    vec_graph g{
      {
        {
          edge_t{vec_2_label::one_one,         "- (-1, -1)",          [](vec_t v) -> vec_t { return -v; }},
          edge_t{vec_2_label::neg_one_neg_one, "+ (-1, -1)",          [](vec_t v) -> vec_t { return +v; }},
          edge_t{vec_2_label::neg_one_zero,    "(-1, -1) += (0, 1)",  [](vec_t v) -> vec_t { return v += vec_t{Field{}, Field(1)}; }},
          edge_t{vec_2_label::neg_one_zero,    "(-1, -1) +  (0, 1)",  [](vec_t v) -> vec_t { return v +  vec_t{Field{}, Field(1)}; }},
          edge_t{vec_2_label::zero_neg_one,    "(-1, -1) += (1, 0)",  [](vec_t v) -> vec_t { return v += vec_t{Field(1), Field{}}; }},
          edge_t{vec_2_label::zero_neg_one,    "(-1, -1) +  (1, 0)",  [](vec_t v) -> vec_t { return v +  vec_t{Field(1), Field{}}; }},
          edge_t{vec_2_label::one_one,         "(-1, -1) *= -1",      [](vec_t v) -> vec_t { return v *= Field(-1);  }},
          edge_t{vec_2_label::one_one,         "(-1, -1) *  -1",      [](vec_t v) -> vec_t { return v *  Field(-1);  }},
          edge_t{vec_2_label::one_one,         "(-1, -1) /= -1",      [](vec_t v) -> vec_t { return v /= Field(-1);  }},
          edge_t{vec_2_label::one_one,         "(-1, -1) /  -1",      [](vec_t v) -> vec_t { return v /  Field(-1);  }},
        }, // neg_one_neg_one
        {

        }, // neg_one_zero
        {

        }, // zero_neg_one
        {
          
        }, // zero_zero
        {

        }, // zero_one
        {
          
        }, // one_zero
        {
          
        }, // one_one
      },
      {vec_t{Field(-1), Field(-1)}, vec_t{Field(-1), Field{}}, vec_t{Field{}, Field(-1)}, vec_t{Field{}, Field{}}, vec_t{Field{}, Field(1)}, vec_t{Field(1), Field{}}, vec_t{Field(1), Field(1)}}
    };

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
