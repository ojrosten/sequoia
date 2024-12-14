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

  template<class Set, maths::weak_field Field, std::size_t D>
  struct my_vec_space
  {
    using set_type          = Set;
    using field_type        = Field;
    using vector_space_type = my_vec_space;
    constexpr static std::size_t dimension{D};

    template<maths::basis Basis>
      requires std::floating_point<field_type>&& is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr field_type inner_product(const maths::vector_coordinates<my_vec_space, Basis>& lhs, const maths::vector_coordinates<my_vec_space, Basis>& rhs)
    {
      return std::ranges::fold_left(std::views::zip(lhs.values(), rhs.values()), field_type{}, [](field_type f, const auto& z){ return f + std::get<0>(z) * std::get<1>(z); });
    }

    template<maths::basis Basis>
      requires is_complex_v<field_type>&& is_orthonormal_basis_v<Basis>
    [[nodiscard]]
    friend constexpr field_type inner_product(const maths::vector_coordinates<my_vec_space, Basis>& lhs, const maths::vector_coordinates<my_vec_space, Basis>& rhs)
    {
      return std::ranges::fold_left(std::views::zip(lhs.values(), rhs.values()), field_type{}, [](field_type f, const auto& z){ return f + conj(std::get<0>(z)) * std::get<1>(z); });
    }
  };

  template<class Set, maths::weak_field Field, std::size_t D>
  struct my_affine_space
  {
    using set_type          = Set;
    using vector_space_type = my_vec_space<Set, Field, D>;
  };

  template<class Set, maths::weak_field Field, std::size_t D>
  struct canonical_basis
  {
    using vector_space_type = my_vec_space<Set, Field, D>;
    using orthonormal = std::true_type;
  };

  template<maths::convex_space ConvexSpace, maths::basis Basis, class Origin, class Validator>
    requires maths::basis_for<Basis, typename ConvexSpace::vector_space_type>
  struct value_tester<maths::coordinates<ConvexSpace, Basis, Origin, Validator>>
  {
    using coord_type = maths::coordinates<ConvexSpace, Basis, Origin, Validator>;
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

  /*! Helper functions for building state-transition graphs*/

  template<class Label>
    requires std::convertible_to<Label, std::size_t>
  [[nodiscard]]
  std::weak_ordering to_ordering(Label From, Label To)
  {
    return From < To ? std::weak_ordering::less
         : From > To ? std::weak_ordering::greater
                     : std::weak_ordering::equivalent;
  }

  template<maths::network Graph, class Label, class Fn>
    requires std::convertible_to<Label, std::size_t>
  void add_transition(Graph& g, Label From, Label To, std::string_view message, Fn f, std::weak_ordering ordering)
  {
    g.join(From, To, std::string{message}, f, ordering);
  }

  template<maths::network Graph, class Label, class Fn>
    requires std::convertible_to<Label, std::size_t>
  void add_transition(Graph& g, Label From, Label To, std::string_view message, Fn f)
  {
    g.join(From, To, std::string{message}, f);
  }

  template<class Coords, maths::network Graph, class Label, class Fn>
    requires std::is_invocable_r_v<Coords, Fn, Coords> && std::convertible_to<Label, std::size_t>
  void add_transition(Graph& g, Label From, Label To, std::string_view message, Fn f)
  {
    using field_t = Coords::field_type;

    if constexpr(std::totally_ordered<field_t>)
      {
        add_transition(g, From, To, message, f, to_ordering(From, To));
      }
    else
      {
        add_transition(g, From, To, message, f);
      }
  }
  
  struct coordinates_operations
  {
    enum dim_1_label{ two, one, zero, neg_one };
    enum dim_2_label{ neg_one_neg_one, neg_one_zero, zero_neg_one, zero_zero, zero_one, one_zero, one_one };

    template<class Coordinates>
    struct default_producer
    {
      using coords_t = Coordinates;
      using vec_t    = maths::vector_coordinates<typename coords_t::vector_space_type, typename coords_t::basis_type>;
      using field_t  = vec_t::field_type;

      [[nodiscard]]
      Coordinates operator()(field_t val) const
      {
        return Coordinates{val};
      }
    };

    template<class Coordinates, class CoordGenerator=default_producer<Coordinates>>
    static typename transition_checker<Coordinates>::transition_graph make_dim_1_transition_graph(CoordGenerator producer={})
    {
      using coords_t     = Coordinates;
      using coords_graph = transition_checker<coords_t>::transition_graph;
      using vec_t        = maths::vector_coordinates<typename coords_t::vector_space_type, typename coords_t::basis_type>;
      using field_t      = vec_t::field_type;

      coords_graph g{
        {
          {}, {}, {}
        },
        {producer(2), producer(1), coords_t{}}
      };

      // Joins from zero
      add_transition<coords_t>(
        g,
        dim_1_label::zero,
        dim_1_label::one,
        "(0) +  (1)",
        [](coords_t p) -> coords_t { return p +  vec_t{field_t(1)}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::zero,
        dim_1_label::one,
        "(0) += (1)",
        [](coords_t p) -> coords_t { return p += vec_t{field_t(1)}; }
      );

      // Joins from one

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        "(1)  - (1)",
        [](coords_t p) -> coords_t { return p -  vec_t{field_t(1)}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        "(1) -= (1)", [](coords_t p) -> coords_t { return p -= vec_t{field_t(1)}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::one,
        "+(1)",
        [](coords_t p) -> coords_t { return +p;}
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        "(1)  + (1)",
        [](coords_t p) -> coords_t { return p +  vec_t{field_t(1)}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        "(1) += (1)",
        [](coords_t p) -> coords_t { return p += vec_t{field_t(1)}; }
      );

      // Joins from two

      add_transition<coords_t>(
        g,
        dim_1_label::two,
        dim_1_label::one,
        "(2) - (1)",
        [](coords_t p) -> coords_t { return p - vec_t{field_t(1)}; }
      );

      if constexpr(!maths::defines_absolute_scale_v<typename Coordinates::validator_type>)
      {
        g.add_node(producer(-1));

        // Joins to neg_one
        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::neg_one,
          "-(1)",
          [](coords_t p) -> coords_t { return -p; }
        );

        // Joins from neg_one
        add_transition<coords_t>(
          g,
          dim_1_label::neg_one,
          dim_1_label::one,
          "- (-1)",
          [](coords_t p) -> coords_t { return -p;  }
        );
    
        add_transition<coords_t>(
          g,
          dim_1_label::neg_one,
          dim_1_label::neg_one,
          "+ (-1)",
          [](coords_t p) -> coords_t { return +p;  }
        );

        add_transition<coords_t>(
          g,
          dim_1_label::neg_one,
          dim_1_label::zero,
          "(-1) += 1",
          [](coords_t p) -> coords_t { auto& v{p.value()}; v += 1; return p; }
        );

        add_transition<coords_t>(
          g,
          dim_1_label::neg_one,
          dim_1_label::zero,
          "(-1) + 1",
          [](coords_t p) -> coords_t { auto& v{p.value()}; v += 1; return p; }
        );
      }

      if constexpr(   //maths::defines_absolute_scale_v<typename Coordinates::validator_type>
                   std::is_same_v<typename Coordinates::origin_type, maths::intrinsic_origin>)
      {
        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::zero,
          "(1) * field_t{}",
          [](coords_t v) -> coords_t { return v * field_t{}; }
        );

        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::zero,
          "field_t{} * (1)",
          [](coords_t v) -> coords_t { return field_t{} *v; }
        );

        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::zero,
          "(1) *= field_t{}",
          [](coords_t v) -> coords_t { return v *= field_t{}; }
        );

        // (1) --> (2)

        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::two,
          "(1) * field_t{2}",
          [](coords_t v) -> coords_t { return v * field_t{2}; }
        );

        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::two,
          "field_t{2} * (1)",
          [](coords_t v) -> coords_t { return field_t{2} *v; }
        );

        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::two,
          "(1) *= field_t{2}",
          [](coords_t v) -> coords_t { return v *= field_t{2}; }
        );

        // (2) --> (1)

        add_transition<coords_t>(
          g,
          dim_1_label::two,
          dim_1_label::one,
          "(2) / field_t{2}",
          [](coords_t v) -> coords_t { return v / field_t{2}; }
        );

        add_transition<coords_t>(
          g,
          dim_1_label::two,
          dim_1_label::one,
          "(2) /= field_t{2}",
          [](coords_t v) -> coords_t { return v /= field_t{2}; }
        );
      }

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
          {
            edge_t{dim_1_label::one,     "- (-1)", [](coords_t p) -> coords_t { return -p;  }},
            edge_t{dim_1_label::neg_one, "+ (-1)", [](coords_t p) -> coords_t { return +p;  }}
          }  // neg_one
        },
        {coords_t{}, coords_t{field_t(1)}, coords_t{field_t(2)}, coords_t{field_t(-1)}}
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
        {coords_t{field_t(-1), field_t(-1)},
         coords_t{field_t(-1), field_t{}},
         coords_t{field_t{}, field_t(-1)},
         coords_t{field_t{}, field_t{}},
         coords_t{field_t{}, field_t(1)},
         coords_t{field_t(1), field_t{}},
         coords_t{field_t(1), field_t(1)}}
      };

      return g;
    }
  };
}
