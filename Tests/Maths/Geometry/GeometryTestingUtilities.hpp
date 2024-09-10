////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/Maths/Geometry/VectorSpace.hpp"

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

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

  template<class Element, maths::field Field, std::size_t D>
  struct my_vec_space
  {
    using element_type = Element;
    using field_type   = Field;
    constexpr static std::size_t dimension{D};

    template<class Basis>
      requires std::floating_point<field_type>&& is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr field_type inner_product(const maths::vector_coordinates<my_vec_space, Basis>& lhs, const maths::vector_coordinates<my_vec_space, Basis>& rhs)
    {
      return std::ranges::fold_left(std::views::zip(lhs.values(), rhs.values()), field_type{}, [](field_type f, const auto& z){ return f + std::get<0>(z) * std::get<1>(z); });
    }

    template<class Basis>
      requires is_complex_v<field_type>&& is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr field_type inner_product(const maths::vector_coordinates<my_vec_space, Basis>& lhs, const maths::vector_coordinates<my_vec_space, Basis>& rhs)
    {
      return std::ranges::fold_left(std::views::zip(lhs.values(), rhs.values()), field_type{}, [](field_type f, const auto& z){ return f + conj(std::get<0>(z)) * std::get<1>(z); });
    }
  };

  template<class Element, maths::field Field, std::size_t D>
  struct my_affine_space
  {
    using set_type          = std::array<Field, D>;
    using vector_space_type = my_vec_space<Element, Field, D>;
  };

  template<class Element, maths::field field_t, std::size_t D>
  struct canonical_basis
  {
    using orthonormal = std::true_type;
  };


  template<maths::affine_space AffineSpace, maths::basis<typename AffineSpace::vector_space_type> Basis, class Origin>
  struct value_tester<maths::affine_coordinates<AffineSpace, Basis, Origin>>
  {
    using coord_type = maths::affine_coordinates<AffineSpace, Basis, Origin>;
    using field_type = typename coord_type::field_type;
    constexpr static std::size_t D{coord_type::dimension};

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const coord_type& actual, const coord_type& prediction)
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
    static void test(equivalence_check_t, test_logger<Mode>& logger, const coord_type& actual, const std::array<field_type, D>& prediction)
    {
      check(equality, "Wrapped values", logger, actual.values(), std::span<const field_type, D>{prediction});
    }
  };

  struct coordinates_operations
  {
    enum dim_1_label{ neg_one, zero, one, two };
    enum dim_2_label{ neg_one_neg_one, neg_one_zero, zero_neg_one, zero_zero, zero_one, one_zero, one_one };

    template<class Coordinates>
    static typename transition_checker<Coordinates>::transition_graph make_dim_1_orderable_transition_graph()
    {
      using coords_t     = Coordinates;
      using coords_graph = transition_checker<coords_t>::transition_graph;
      using edge_t       = transition_checker<coords_t>::edge;
      using vec_t        = maths::vector_coordinates<typename coords_t::vector_space_type, typename coords_t::basis_type>;
      using field_t      = vec_t::field_type;

      coords_graph g{
        {
          {
            edge_t{dim_1_label::one,     "- (-1)", [](coords_t p) -> coords_t { return -p;  }, std::weak_ordering::greater},
            edge_t{dim_1_label::neg_one, "+ (-1)", [](coords_t p) -> coords_t { return +p;  }, std::weak_ordering::equivalent}
          }, // neg_one
          {
            edge_t{dim_1_label::one, "(0) +  (1)", [](coords_t p) -> coords_t { return p + vec_t{field_t(1)}; }, std::weak_ordering::greater},
            edge_t{dim_1_label::one, "(0) += (1)", [](coords_t p) -> coords_t { return p += vec_t{field_t(1)}; }, std::weak_ordering::greater}
          }, // zero
          {
            edge_t{dim_1_label::neg_one, "-(1)",       [](coords_t p) -> coords_t { return -p;                     }, std::weak_ordering::less},
            edge_t{dim_1_label::zero,    "(1)  - (1)", [](coords_t p) -> coords_t { return p - vec_t{field_t(1)}; }, std::weak_ordering::less},
            edge_t{dim_1_label::zero,    "(1) -= (1)", [](coords_t p) -> coords_t { return p -= vec_t{field_t(1)}; }, std::weak_ordering::less},
            edge_t{dim_1_label::one,     "+(1)",       [](coords_t p) -> coords_t { return +p;                     }, std::weak_ordering::equivalent},
            edge_t{dim_1_label::two,     "(1)  + (1)", [](coords_t p) -> coords_t { return p + vec_t{field_t(1)}; }, std::weak_ordering::greater},
            edge_t{dim_1_label::two,     "(1) += (1)", [](coords_t p) -> coords_t { return p += vec_t{field_t(1)}; }, std::weak_ordering::greater},
          }, // one
          {
            edge_t{dim_1_label::one, "(2) - (1)", [](coords_t p) -> coords_t { return p - vec_t{field_t(1)}; }, std::weak_ordering::less}
          }, // two
        },
        {coords_t{field_t(-1)}, coords_t{}, coords_t{field_t(1)}, coords_t{field_t(2)}}
      };

      return g;
    }

    template<class Coordinates>
    static typename transition_checker<Coordinates>::transition_graph make_dim_1_unorderable_transition_graph()
    {
      using coords_t     = Coordinates;
      using coords_graph = transition_checker<coords_t>::transition_graph;
      using edge_t       = transition_checker<coords_t>::edge;
      using vec_t        = maths::vector_coordinates<typename coords_t::vector_space_type, typename coords_t::basis_type>;
      using field_t      = vec_t::field_type;

      coords_graph g{
        {
          {
            edge_t{dim_1_label::one,     "- (-1)", [](coords_t p) -> coords_t { return -p;  }},
            edge_t{dim_1_label::neg_one, "+ (-1)", [](coords_t p) -> coords_t { return +p;  }}
          }, // neg_one
          {
            edge_t{dim_1_label::one, "(0) +  (1)", [](coords_t p) -> coords_t { return p +  vec_t{field_t(1)}; }},
            edge_t{dim_1_label::one, "(0) += (1)", [](coords_t p) -> coords_t { return p += vec_t{field_t(1)}; }}
          }, // zero
          {
            edge_t{dim_1_label::neg_one, "-(1)",       [](coords_t p) -> coords_t { return -p;                     }},
            edge_t{dim_1_label::zero,    "(1)  - (1)", [](coords_t p) -> coords_t { return p -  vec_t{field_t(1)}; }},
            edge_t{dim_1_label::zero,    "(1) -= (1)", [](coords_t p) -> coords_t { return p -= vec_t{field_t(1)}; }},
            edge_t{dim_1_label::one,     "+(1)",       [](coords_t p) -> coords_t { return +p;                     }},
            edge_t{dim_1_label::two,     "(1)  + (1)", [](coords_t p) -> coords_t { return p +  vec_t{field_t(1)}; }},
            edge_t{dim_1_label::two,     "(1) += (1)", [](coords_t p) -> coords_t { return p += vec_t{field_t(1)}; }},
          }, // one
          {
            edge_t{dim_1_label::one, "(2) - (1)", [](coords_t p) -> coords_t { return p - vec_t{field_t(1)}; }}
          }, // two
        },
        {coords_t{field_t(-1)}, coords_t{}, coords_t{field_t(1)}, coords_t{field_t(2)}}
      };

      return g;
    }

    template<class Coordinates>
    static typename transition_checker<Coordinates>::transition_graph make_dim_2_transition_graph()
    {
      using coords_t     = Coordinates;
      using coords_graph = transition_checker<coords_t>::transition_graph;
      using edge_t       = transition_checker<coords_t>::edge;
      using vec_t        = maths::vector_coordinates<typename coords_t::vector_space_type, typename coords_t::basis_type>;
      using field_t      = vec_t::field_type;

      coords_graph g{
        {
          {
            edge_t{dim_2_label::one_one,         "- (-1, -1)",          [](coords_t v) -> coords_t { return -v; }},
            edge_t{dim_2_label::neg_one_neg_one, "+ (-1, -1)",          [](coords_t v) -> coords_t { return +v; }},
            edge_t{dim_2_label::neg_one_zero,    "(-1, -1) +  (0, 1)",  [](coords_t v) -> coords_t { return v +  coords_t{field_t{}, field_t(1)}; }},
            edge_t{dim_2_label::neg_one_zero,    "(-1, -1) += (0, 1)",  [](coords_t v) -> coords_t { return v += coords_t{field_t{}, field_t(1)}; }},
            edge_t{dim_2_label::zero_neg_one,    "(-1, -1) +  (1, 0)",  [](coords_t v) -> coords_t { return v +  coords_t{field_t(1), field_t{}}; }},
            edge_t{dim_2_label::zero_neg_one,    "(-1, -1) += (1, 0)",  [](coords_t v) -> coords_t { return v += coords_t{field_t(1), field_t{}}; }}
          }, // neg_one_neg_one
          {
            edge_t{dim_2_label::neg_one_neg_one, "(-1, 0) -  (0, 1)",  [](coords_t v) -> coords_t { return v -  coords_t{field_t{}, field_t(1)}; }},
            edge_t{dim_2_label::neg_one_neg_one, "(-1, 0) -= (0, 1)",  [](coords_t v) -> coords_t { return v -= coords_t{field_t{}, field_t(1)}; }}
          }, // neg_one_zero
          {
            edge_t{dim_2_label::neg_one_neg_one, "(0, -1) -  (1, 0)",  [](coords_t v) -> coords_t { return v - coords_t{field_t{1}, field_t(0)}; }},
            edge_t{dim_2_label::neg_one_neg_one, "(0, -1) -= (1, 0)",  [](coords_t v) -> coords_t { return v -= coords_t{field_t{1}, field_t(0)}; }}
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
        {coords_t{field_t(-1), field_t(-1)}, coords_t{field_t(-1), field_t{}}, coords_t{field_t{}, field_t(-1)}, coords_t{field_t{}, field_t{}}, coords_t{field_t{}, field_t(1)}, coords_t{field_t(1), field_t{}}, coords_t{field_t(1), field_t(1)}}
      };

      return g;
    }
  };
}
