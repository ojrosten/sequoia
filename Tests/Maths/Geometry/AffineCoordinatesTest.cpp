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

  template<class Element, maths::weak_field Field>
  void affine_coordinates_test::test_affine_1_orderable()
  {
    using affine_t = affine_coordinates<my_affine_space<Element, Field, 1>, canonical_basis<Element, Field, 1>, alice>;
    auto g{coordinates_operations::make_dim_1_orderable_transition_graph<affine_t>()};

    auto checker{
        [this](std::string_view description, const affine_t& obtained, const affine_t& prediction, const affine_t& parent, std::weak_ordering ordering) {
          check(equality, description, obtained, prediction);
          if(ordering != std::weak_ordering::equivalent)
            check_semantics(description, prediction, parent, ordering);
        }
    };

    transition_checker<affine_t>::check(report(""), g, checker);
  }

  template<class Element, maths::weak_field Field>
  void affine_coordinates_test::test_affine_1_unorderable()
  {
    using affine_t = affine_coordinates<my_affine_space<Element, Field, 1>, canonical_basis<Element, Field, 1>, alice>;
    auto g{coordinates_operations::make_dim_1_unorderable_transition_graph<affine_t>()};

    auto checker{
        [this](std::string_view description, const affine_t& obtained, const affine_t& prediction, const affine_t& parent, std::size_t host, std::size_t target) {
          check(equality, description, obtained, prediction);
          if(host != target) check_semantics(description, prediction, parent);
        }
    };

    transition_checker<affine_t>::check(report(""), g, checker);
  }

  template<class Element, maths::weak_field Field>
  void affine_coordinates_test::test_affine_2()
  {
    using affine_t = affine_coordinates<my_affine_space<Element, Field, 2>, canonical_basis<Element, Field, 2>, alice>;
    auto g{coordinates_operations::make_dim_2_transition_graph<affine_t>()};

    auto checker{
        [this](std::string_view description, const affine_t& obtained, const affine_t& prediction, const affine_t& parent, std::size_t host, std::size_t target) {
          check(equality, description, obtained, prediction);
          if(host != target) check_semantics(description, prediction, parent);
        }
    };

    transition_checker<affine_t>::check(report(""), g, checker);
  }

}
