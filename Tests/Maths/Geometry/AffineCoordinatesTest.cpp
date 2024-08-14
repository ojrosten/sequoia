////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AffineCoordinatesTest.hpp"

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    enum affine_1_label{ neg_one, zero, one, two };
    enum affine_2_label{ neg_one_neg_one, neg_one_zero, zero_neg_one, zero_zero, zero_one, one_zero, one_one };

    struct alice {};
  }

  [[nodiscard]]
  std::filesystem::path affine_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void affine_coordinates_test::run_tests()
  {
    test_affine_1_orderable<float, float>();
    test_affine_1_orderable<double, double>();
    test_affine_1_unorderable<std::complex<float>, std::complex<float>>();
  }

  template<class Element, maths::field Field>
  void affine_coordinates_test::test_affine_1_orderable()
  {
    using affine_t     = affine_coordinates<alice, my_vec_space<Element, Field, 1>, canonical_basis<Element, Field, 1>>;
    auto g{coordinates_operations::make_dim_1_orderable_transition_graph<affine_t>()};

    auto checker{
        [this](std::string_view description, const affine_t& obtained, const affine_t& prediction, const affine_t& parent, std::weak_ordering ordering) {
          check(equality, description, obtained, prediction);
          if(ordering != std::weak_ordering::equivalent)
            check_semantics(description, prediction, parent, ordering);
        }
    };

    transition_checker<affine_t>::check(report_line(""), g, checker);
  }

  template<class Element, maths::field Field>
  void affine_coordinates_test::test_affine_1_unorderable()
  {
    using affine_t = affine_coordinates<alice, my_vec_space<Element, Field, 1>, canonical_basis<Element, Field, 1>>;
    auto g{coordinates_operations::make_dim_1_unorderable_transition_graph<affine_t>()};

    auto checker{
        [this](std::string_view description, const affine_t& obtained, const affine_t& prediction, const affine_t& parent, std::size_t host, std::size_t target) {
          check(equality, description, obtained, prediction);
          if(host != target) check_semantics(description, prediction, parent);
        }
    };

    transition_checker<affine_t>::check(report_line(""), g, checker);
  }

  template<class Element, maths::field Field>
  void affine_coordinates_test::test_affine_2()
  {
    using affine_t     = affine_coordinates<alice, my_vec_space<Element, Field, 1>, canonical_basis<Element, Field, 1>>;
    using affine_graph = transition_checker<affine_t>::transition_graph;
    using edge_t       = transition_checker<affine_t>::edge;
    using vec_t        = vector_coordinates<typename affine_t::vector_space_type, typename affine_t::basis_type>;

    affine_graph g{
      {
        {
          edge_t{affine_2_label::one_one,         "- (-1, -1)",          [](vec_t v) -> vec_t { return -v; }},
          edge_t{affine_2_label::neg_one_neg_one, "+ (-1, -1)",          [](vec_t v) -> vec_t { return +v; }},
          edge_t{affine_2_label::neg_one_zero,    "(-1, -1) += (0, 1)",  [](vec_t v) -> vec_t { return v += vec_t{Field{}, Field(1)}; }},
          edge_t{affine_2_label::neg_one_zero,    "(-1, -1) +  (0, 1)",  [](vec_t v) -> vec_t { return v + vec_t{Field{}, Field(1)}; }},
          edge_t{affine_2_label::zero_neg_one,    "(-1, -1) += (1, 0)",  [](vec_t v) -> vec_t { return v += vec_t{Field(1), Field{}}; }},
          edge_t{affine_2_label::zero_neg_one,    "(-1, -1) +  (1, 0)",  [](vec_t v) -> vec_t { return v + vec_t{Field(1), Field{}}; }}
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
      {affine_t{Field(-1), Field(-1)}, affine_t{Field(-1), Field{}}, affine_t{Field{}, Field(-1)}, affine_t{Field{}, Field{}}, affine_t{Field{}, Field(1)}, affine_t{Field(1), Field{}}, affine_t{Field(1), Field(1)}}
    };

    auto checker{
        [this](std::string_view description, const affine_t& obtained, const affine_t& prediction, const affine_t& parent, std::size_t host, std::size_t target) {
          check(equality, description, obtained, prediction);
          if(host != target) check_semantics(description, prediction, parent);
        }
    };

    transition_checker<affine_t>::check(report_line(""), g, checker);
  }

}
