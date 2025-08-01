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
  constexpr bool can_multiply{
    requires(const T& t, const U& u) { t * u; }
  };

  template<class T, class U>
  constexpr bool can_divide{
    requires(const T& t, const U& u) { t / u; }
  };

  template<class T, class U>
  constexpr bool can_add{
    requires(const T& t, const U& u) { t + u; }
  };

  template<class T, class U>
  constexpr bool can_subtract{
    requires(const T& t, const U& u) { t - u; }
  };

  template<class T>
  constexpr bool has_unary_plus{
    requires(const T& t) { { +t } -> std::convertible_to<T>; }
  };

  template<class T>
  constexpr bool has_unary_minus{
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

  template<class Set, maths::weak_field Field, std::size_t D>
  struct my_vec_space
  {
    using set_type        = Set;
    using field_type      = Field;
    using is_vector_space = std::true_type;
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
    using is_affine_space   = std::true_type;
  };

  template<class Set, maths::weak_field Field, std::size_t D>
  struct canonical_basis
  {
    using vector_space_type = my_vec_space<Set, Field, D>;
    using orthonormal = std::true_type;
  };

  template<class Set, maths::weak_commutative_ring Ring, std::size_t D>
  struct my_free_module
  {
    using set_type              = Set;
    using commutative_ring_type = Ring;
    using is_free_module        = std::true_type;
    constexpr static std::size_t dimension{D};
  };

  template<class Set, maths::weak_commutative_ring Ring, std::size_t D>
  struct canonical_free_module_basis
  {
    using free_module_type = my_free_module<Set, Ring, D>;
  };

  template<maths::convex_space ConvexSpace, maths::basis Basis, class Origin, class Validator>
    requires maths::basis_for<Basis, maths::free_module_type_of_t<ConvexSpace>>
  struct value_tester<maths::coordinates<ConvexSpace, Basis, Origin, Validator>>
  {
    using coord_type = maths::coordinates<ConvexSpace, Basis, Origin, Validator>;
    using commutative_ring_type = typename coord_type::commutative_ring_type;
    constexpr static std::size_t D{coord_type::dimension};

    template<test_mode Mode>
    static void test(equality_check_t, test_logger<Mode>& logger, const coord_type& actual, const coord_type& prediction)
    {
      check(equality, "Wrapped values", logger, actual.values(), prediction.values());
      if constexpr(D == 1)
      {
        check(equality, "Wrapped value", logger, actual.value(), prediction.value());
        if constexpr(std::convertible_to<commutative_ring_type, bool>)
          check(equality, "Conversion to bool", logger, static_cast<bool>(actual), static_cast<bool>(prediction));
      }

      for(auto i : std::views::iota(0uz, D))
      {
        check(equality, std::format("Value at index {}", i), logger, actual[i], prediction[i]);
      }
    }

    template<test_mode Mode>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const coord_type& actual, const std::array<commutative_ring_type, D>& prediction)
    {
      check(equality, "Wrapped values", logger, actual.values(), std::span<const commutative_ring_type, D>{prediction});
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
  void add_transition(Graph& g, Label From, Label To, std::string_view message, Fn f, inverted_ordering invert={})
  {
    using ring_t = Coords::commutative_ring_type;
    constexpr static auto dimension{Coords::dimension};

    if constexpr((dimension == 1) && std::totally_ordered<ring_t>)
    {
      add_transition(g, From, To, message, f, to_ordering(From, To, invert));
    }
    else
    {
      add_transition(g, From, To, message, f);
    }
  }

  template<class T>
  inline constexpr bool has_units_type{
    requires { typename T::units_type; }
  };

  
  template<class Coordinates>
  class coordinates_operations
  {    
    enum dim_1_label{ two, one, zero, neg_one };
    enum dim_2_label{ neg_one_neg_one, neg_one_zero, zero_neg_one, zero_zero, zero_one, one_zero, one_one };
    
    using graph_type = transition_checker<Coordinates>::transition_graph;
    using coords_t   = Coordinates;
    using disp_t     = coords_t::displacement_coordinates_type;
    using module_t   = coords_t::free_module_type;
    using ring_t     = coords_t::commutative_ring_type;
    constexpr static std::size_t dimension{Coordinates::dimension};
    constexpr static bool orderable{(dimension == 1) && std::totally_ordered<ring_t>};

    regular_test& m_Test;
    graph_type m_Graph;
  public:
    explicit coordinates_operations(regular_test& t)
      : m_Test{t}
      , m_Graph{make_graph(m_Test)}
    {}

    void execute()
    {
      transition_checker<coords_t>::check("", m_Graph, make_checker());
    }
  private:
    static graph_type make_graph(regular_test& test)
    {
      if constexpr(has_units_type<coords_t>)
        return do_make_graph(test, typename coords_t::units_type{});
      else
        return do_make_graph(test);
    }
    
    template<class... Units>
    [[nodiscard]]
    static graph_type do_make_graph(regular_test& test, Units... units)
    {
      if constexpr     (dimension == 1) return make_dim_1_transition_graph(test, units...);
      else if constexpr(dimension == 2) return make_dim_2_transition_graph(test, units...);
    }
    
    [[nodiscard]]
    auto make_checker() const
    {
      if constexpr(orderable)
      {
        return
          [&test=m_Test](std::string_view description, const coords_t& obtained, const coords_t& prediction, const coords_t& parent, std::weak_ordering ordering) {
            test.check(equality, description, obtained, prediction);
            if(ordering != std::weak_ordering::equivalent)
              test.check_semantics(description, prediction, parent, ordering);
          };
      }
      else
      {
        return
          [&test=m_Test](std::string_view description, const coords_t& obtained, const coords_t& prediction, const coords_t& parent, std::size_t host, std::size_t target) {
            test.check(equality, description, obtained, prediction);
            if(host!= target) test.check_semantics(description, prediction, parent);
          };
      }
    }

    template<class... Units>
    static graph_type make_dim_1_transition_graph(regular_test& test, Units... units)
    {
      graph_type g{
        {
          {}, {}, {}
        },
        {coords_t{ring_t(2), units...}, coords_t{ring_t(1), units...}, coords_t{}}
      };

      add_dim_1_common_transitions(g, test, units...);

      if constexpr(!maths::defines_half_line_v<typename Coordinates::validator_type>)
      {
        add_dim_1_negative_transitions(g, test, units...);
      }
      else if constexpr(std::is_signed_v<ring_t>)
      {
        add_dim_1_attempted_negative_transitions(g, test, units...);
      }

      if constexpr(std::is_same_v<typename Coordinates::origin_type, maths::distinguished_origin>)
      {
        add_dim_1_distinguished_origin_transitions(g, test);
      }

      if constexpr(Coordinates::has_freely_mutable_components)
      {
        add_dim_1_free_mutations(g, test);
      }

      return g;
    }

    template<class... Units>
    static graph_type make_dim_2_transition_graph(regular_test& test, Units... units)
    {
      using edge_t = transition_checker<coords_t>::edge;
      graph_type g{
        {
          {
             edge_t{dim_2_label::neg_one_neg_one, test.report("+ (-1, -1)"),          [](coords_t v) -> coords_t { return +v; }},
            edge_t{dim_2_label::neg_one_zero,    test.report("(-1, -1) +  (0, 1)"),  [&](coords_t v) -> coords_t { return v +  disp_t{std::array{ring_t{}, ring_t(1)}, units...}; }},
            edge_t{dim_2_label::neg_one_zero,    test.report("(-1, -1) += (0, 1)"),  [&](coords_t v) -> coords_t { return v += disp_t{std::array{ring_t{}, ring_t(1)}, units...}; }},
            edge_t{dim_2_label::zero_neg_one,    test.report("(-1, -1) +  (1, 0)"),  [&](coords_t v) -> coords_t { return v +  disp_t{std::array{ring_t(1), ring_t{}}, units...}; }},
            edge_t{dim_2_label::zero_neg_one,    test.report("(-1, -1) += (1, 0)"),  [&](coords_t v) -> coords_t { return v += disp_t{std::array{ring_t(1), ring_t{}}, units...}; }}
          }, // neg_one_neg_one
          {
            edge_t{dim_2_label::neg_one_neg_one, test.report("(-1, 0) -  (0, 1)"),  [&](coords_t v) -> coords_t { return v -  disp_t{std::array{ring_t{}, ring_t(1)}, units...}; }},
            edge_t{dim_2_label::neg_one_neg_one, test.report("(-1, 0) -= (0, 1)"),  [&](coords_t v) -> coords_t { return v -= disp_t{std::array{ring_t{}, ring_t(1)}, units...}; }}
          }, // neg_one_zero
          {
            edge_t{dim_2_label::neg_one_neg_one, test.report("(0, -1) -  (1, 0)"),  [&](coords_t v) -> coords_t { return v -  disp_t{std::array{ring_t{1}, ring_t(0)}, units...}; }},
            edge_t{dim_2_label::neg_one_neg_one, test.report("(0, -1) -= (1, 0)"),  [&](coords_t v) -> coords_t { return v -= disp_t{std::array{ring_t{1}, ring_t(0)}, units...}; }}
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
        {coords_t{std::array{ring_t(-1), ring_t(-1)}, units...},
         coords_t{std::array{ring_t(-1), ring_t{}},   units...},
         coords_t{std::array{ring_t{},   ring_t(-1)}, units...},
         coords_t{std::array{ring_t{},   ring_t{}},   units...},
         coords_t{std::array{ring_t{},   ring_t(1)},  units...},
         coords_t{std::array{ring_t(1),  ring_t{}},   units...},
         coords_t{std::array{ring_t(1),  ring_t(1)},  units...}
        }
      };

      if constexpr(std::is_same_v<typename Coordinates::origin_type, maths::distinguished_origin>)
      {
        add_dim_2_distinguished_origin_transitions(g, test, units...);
      }

      if constexpr(Coordinates::has_freely_mutable_components)
      {
        add_dim_2_free_mutations(g, test);
      }

      return g;
    }

    template<class... Units>
    static void add_dim_1_common_transitions(maths::network auto& g, regular_test& test, Units... units)
    {
      // Joins from zero
      add_transition<coords_t>(
        g,
        dim_1_label::zero,
        dim_1_label::one,
        test.report("(0) +  (1)"),
        [&](coords_t p) -> coords_t { return p +  disp_t{ring_t(1), units...}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::zero,
        dim_1_label::one,
        test.report("(0) += (1)"),
        [&](coords_t p) -> coords_t { return p += disp_t{ring_t(1), units...}; }
      );

      // Joins from one

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1)  - (1)"),
        [&](coords_t p) -> coords_t { return p -  disp_t{ring_t(1), units...}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1) -= (1)"),
        [&](coords_t p) -> coords_t { return p -= disp_t{ring_t(1), units...}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::one,
        test.report("+(1)"),
        [](coords_t p) -> coords_t { return +p;}
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1)  + (1)"),
        [&](coords_t p) -> coords_t { return p +  disp_t{ring_t(1), units...}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1) += (1)"),
        [&](coords_t p) -> coords_t { return p += disp_t{ring_t(1), units...}; }
      );

      // Joins from two

      add_transition<coords_t>(
        g,
        dim_1_label::two,
        dim_1_label::one,
        test.report("(2) - (1)"),
        [&](coords_t p) -> coords_t { return p - disp_t{ring_t(1), units...}; }
      );
    }

    template<class... Units>
    static void add_dim_1_negative_transitions(maths::network auto& g, regular_test& test, Units... units)
    {
      g.add_node(ring_t(-1), units...);

      // Joins to neg_one
      if constexpr(coords_t::has_distinguished_origin && !std::is_unsigned_v<ring_t>)
      {
        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::neg_one,
          test.report("-(1)"),
          [](coords_t p) -> coords_t { return -p; },
          std::is_unsigned_v<ring_t> ? inverted_ordering::yes : inverted_ordering::no
        );
      }

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::neg_one,
        test.report("(1) - (2)"),
        [&](coords_t p) -> coords_t { return p - disp_t{ring_t(2), units...}; },
        std::is_unsigned_v<ring_t> ? inverted_ordering::yes : inverted_ordering::no
      );

      
      // Joins from neg_one
      if constexpr(coords_t::has_distinguished_origin && !std::is_unsigned_v<ring_t>)
      {
        add_transition<coords_t>(
          g,
          dim_1_label::neg_one,
          dim_1_label::one,
          test.report("- (-1)"),
          [](coords_t p) -> coords_t { return -p;  },
          std::is_unsigned_v<ring_t> ? inverted_ordering::yes : inverted_ordering::no
        );
      }
    
      add_transition<coords_t>(
        g,
        dim_1_label::neg_one,
        dim_1_label::neg_one,
        test.report("+ (-1)"),
        [](coords_t p) -> coords_t { return +p;  }
      );

      if constexpr(Coordinates::has_freely_mutable_components)
      {
        add_transition<coords_t>(
          g,
          dim_1_label::neg_one,
          dim_1_label::zero,
          test.report("(-1) += 1"),
          [](coords_t p) -> coords_t { auto& v{p.value()}; v += 1; return p; },
          std::is_unsigned_v<ring_t> ? inverted_ordering::yes : inverted_ordering::no
        );

        add_transition<coords_t>(
          g,
          dim_1_label::neg_one,
          dim_1_label::zero,
          test.report("(-1) + 1"),
          [](coords_t p) -> coords_t { auto& v{p.value()}; v += 1; return p; },
          std::is_unsigned_v<ring_t> ? inverted_ordering::yes : inverted_ordering::no
        );
      }
    }

    template<class... Units>
    static void add_dim_1_attempted_negative_transitions(maths::network auto& g, regular_test& test, Units... units)
    {
      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::one,
        test.report("(1) -= (2)"),
        [&](coords_t p) -> coords_t {
          test.check_exception_thrown<std::domain_error>("", [&](){ return p -= disp_t{ring_t(2), units...};});
          return p;
        }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::one,
        test.report("(1) - (2)"),
        [&](coords_t p) -> coords_t {
          test.check_exception_thrown<std::domain_error>("", [&](){ return p = (p - disp_t{ring_t(2), units...}); });
          return p;
        }
      );

      if constexpr(std::is_same_v<typename Coordinates::origin_type, maths::distinguished_origin>)
      {
        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::one,
          test.report("(1) *= ring_t{-1}"),
          [&](coords_t v) -> coords_t {
            test.check_exception_thrown<std::domain_error>("", [&v](){ return v *= ring_t{-1}; });
            return v;
          }          
        );

        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::one,
          test.report("ring_t{-1} * (1)"),
          [&test](coords_t v) -> coords_t {
            test.check_exception_thrown<std::domain_error>("", [&v](){ return v = ring_t{-1} * v; });
            return v;
          }          
        );

        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::one,
          test.report("(1) /= ring_t{-1}"),
          [&test](coords_t v) -> coords_t {
            test.check_exception_thrown<std::domain_error>("", [&v](){ return v /= ring_t{-1}; });
            return v;
          }          
        );

        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::one,
          test.report("(1) / ring_t{-1}"),
          [&test](coords_t v) -> coords_t {
            test.check_exception_thrown<std::domain_error>("", [&v](){ return v = v / ring_t{-1}; });
            return v;
          }          
        );
      }
    }

    static void add_dim_1_distinguished_origin_transitions(maths::network auto& g, regular_test& test)
    {
      // (0) --> (1)
      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1) * ring_t{}"),
        [](coords_t v) -> coords_t { return v * ring_t{}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("ring_t{} * (1)"),
        [](coords_t v) -> coords_t { return ring_t{} *v; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1) *= ring_t{}"),
        [](coords_t v) -> coords_t { return v *= ring_t{}; }
      );

      // (1) --> (2)

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1) * ring_t{2}"),
        [](coords_t v) -> coords_t { return v * ring_t{2}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("ring_t{2} * (1)"),
        [](coords_t v) -> coords_t { return ring_t{2} *v; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1) *= ring_t{2}"),
        [](coords_t v) -> coords_t { return v *= ring_t{2}; }
      );

      // (2) --> (1)

      if constexpr(maths::vector_space<module_t>)
      {
        add_transition<coords_t>(
          g,
          dim_1_label::two,
          dim_1_label::one,
          test.report("(2) / ring_t{2}"),
          [](coords_t v) -> coords_t { return v / ring_t{2}; }
        );

        add_transition<coords_t>(
          g,
          dim_1_label::two,
          dim_1_label::one,
          test.report("(2) /= ring_t{2}"),
          [](coords_t v) -> coords_t { return v /= ring_t{2}; }
        );
      }
    }

    static void add_dim_1_free_mutations(maths::network auto& g, regular_test& test)
    {
      // (1) --> (0)
      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1)[0] * ring_t{}"),
        [](coords_t v) -> coords_t { v[0] *= ring_t{}; return v; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1).begin[0] * ring_t{}"),
        [](coords_t v) -> coords_t { v.begin()[0] *= ring_t{}; return v; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1).rbegin[0] * ring_t{}"),
        [](coords_t v) -> coords_t { v.rbegin()[0] *= ring_t{}; return v; }
      );

      // (1) --> (2)

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1)[0] * ring_t{2}"),
        [](coords_t v) -> coords_t { v[0] *= ring_t{2}; return v; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1).begin[0] * ring_t{2}"),
        [](coords_t v) -> coords_t { v.begin()[0] *= ring_t{2}; return v; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1).rbegin[0] * ring_t{2}"),
        [](coords_t v) -> coords_t { v.rbegin()[0] *= ring_t{2}; return v; }
      );
    }

    static void add_dim_2_distinguished_origin_transitions(maths::network auto& g, regular_test& test)
    {      
      // (-1, -1) --> (1, 1)
   
      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::one_one,
        test.report("- (-1, -1)"),
        [](coords_t v) -> coords_t { return -v; }
      );

      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::one_one,
        test.report("(-1, -1) *= -1"),
        [](coords_t v) -> coords_t { return v *= ring_t{-1}; }
      );

      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::one_one,
        test.report("(-1, -1) * -1"),
        [](coords_t v) -> coords_t { return v * ring_t{-1}; }
      );

      if constexpr(maths::vector_space<module_t>)
      {
        add_transition<coords_t>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::one_one,
          test.report("(-1, -1) /= -1"),
          [](coords_t v) -> coords_t { return v /= ring_t{-1}; }
        );

        add_transition<coords_t>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::one_one,
          test.report("(-1, -1) / -1"),
          [](coords_t v) -> coords_t { return v / ring_t{-1}; }
        );
      }
    }

    static void add_dim_2_free_mutations(maths::network auto& g, regular_test& test)
    {
      // (-1, -1) --> (-1, 0)

      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::neg_one_zero,
        test.report("(-1, -1)[1] *= 0"),
        [](coords_t v) -> coords_t { v[1] *= ring_t{}; return v; }
      );

      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::neg_one_zero,
        test.report("(-1, -1).begin()[1] *= 0"),
        [](coords_t v) -> coords_t { v.begin()[1] *= ring_t{}; return v; }
      );

      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::neg_one_zero,
        test.report("(-1, -1).rbegin()[0] *= 0"),
        [](coords_t v) -> coords_t { v.rbegin()[0] *= ring_t{}; return v; }
      );

      // (0, 1) --> (1, 1)
      add_transition<coords_t>(
        g,
        dim_2_label::zero_one,
        dim_2_label::one_one,
        test.report("(0, 1)[0] += 1"),
        [](coords_t v) -> coords_t { v[0] += ring_t{1}; return v; }
      );

      add_transition<coords_t>(
        g,
        dim_2_label::zero_one,
        dim_2_label::one_one,
        test.report("(0, 1).begin[0] += 1"),
        [](coords_t v) -> coords_t { v.begin()[0] += ring_t{1}; return v; }
      );

      add_transition<coords_t>(
        g,
        dim_2_label::zero_one,
        dim_2_label::one_one,
        test.report("(0, 1).rbegin[1] += 1"),
        [](coords_t v) -> coords_t { v.rbegin()[1] += ring_t{1}; return v; }
      );
    }
  };
}
