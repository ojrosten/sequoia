////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "CommonGeometryTestingUtilities.hpp"

namespace sequoia::testing
{
  template<class Coordinates>
  inline constexpr bool supports_multiplicative_syntactic_sugar{
    requires {
      requires can_multiply<typename Coordinates::value_type, typename Coordinates::basis_isomorphism_type>;
      requires std::same_as<decltype(std::declval<typename Coordinates::value_type>() * std::declval<typename Coordinates::basis_isomorphism_type>()), Coordinates>;
    }
  };
  
  template<class Coordinates>
  class coordinates_operations
  {    
    enum dim_1_label{ delta_one, delta_zero, two, one, zero, delta_neg_one, delta_neg_two, neg_one };
    enum dim_2_label{ one_two, one_one, one_zero, zero_one, zero_zero, zero_neg_one, neg_one_zero, neg_one_neg_one };
    
    using coords_t            = Coordinates;
    using space_t             = Coordinates::space_type;
    using disp_t              = coords_t::displacement_coordinates_type;
    using module_t            = coords_t::free_module_type;
    using ring_t              = coords_t::commutative_ring_type;
    using units_t             = coords_t::basis_isomorphism_type;
    using representation_t    = coords_t::representation_type;
    using value_t             = representation_t::value_type; // TO DO: only just made distinct from ring_t. Needs to be properly reasoned through
    using validator_t         = coords_t::validator_type;
    using basis_isomorphism_t = coords_t::basis_isomorphism_type;
    using variant_t           = std::conditional_t<std::same_as<coords_t, disp_t>, std::variant<coords_t>, std::variant<coords_t, disp_t>>;
    using graph_type          = transition_checker<variant_t>::transition_graph;
    constexpr static std::size_t dimension{Coordinates::dimension};
    constexpr static bool orderable{(dimension == 1) && std::totally_ordered<ring_t>};
    constexpr static bool has_distinguished_origin{maths::has_distinguished_origin_v<space_t>};
    constexpr static auto bounds_v{representation_t::bounds_v};
    constexpr static bool has_canonical_rep{std::same_as<representation_t, maths::canonical_representation<bounds_v>>};

    regular_test& m_Test;
    graph_type m_Graph;
  public:
    explicit coordinates_operations(regular_test& t)
      : m_Test{t}
      , m_Graph{make_graph(m_Test)}
    {}

    void execute()
    {
      transition_checker<variant_t>::check("", m_Graph, make_checker());
    }
  private:
    template<std::size_t D>
    struct dimensionality{};
    
    [[nodiscard]]
    static graph_type make_graph(regular_test& test)
    {
      return make_transition_graph(test, dimensionality<dimension>{});
    }

    template<class T, std::size_t D>
    [[nodiscard]]
    static auto from_underlying(const std::array<T, D>& vals)
    {
      return representation_t{}.from_underlying(std::span{vals});
    }

    template<class To, class T, std::size_t D>
    [[nodiscard]]
    static To from_underlying(const std::array<T, D>& vals)
    {
      return To{from_underlying(vals), units_t{}};
    }

    template<class T>
    [[nodiscard]]
    static auto from_underlying(T val) {
      if constexpr(maths::representation_for_span<representation_t, space_t>)
      {
        return from_underlying(std::array{val})[0];
      }
      else
      {
        return representation_t{}.from_underlying(val);
      }
    }

    template<class To, class T>
    [[nodiscard]]
    static To from_underlying(T val) {
      if constexpr(maths::representation_for_span<representation_t, space_t>)
      {
        return To{from_underlying(std::array{val})[0], units_t{}};
      }
      else
      {
        return To{from_underlying(val), units_t{}};
      }
    }
    
    template<std::floating_point T>
    [[nodiscard]]
    constexpr static T tolerance() noexcept
    {
      return std::same_as<T, float> ? T(1e-6) : T(1e-12);
    }

    [[nodiscard]]
    auto make_checker() const
    {
      constexpr auto tol{
        [](){
          if constexpr(is_complex_v<ring_t>){
            using underlying_value_t = ring_t::value_type;
            constexpr auto toler{tolerance<underlying_value_t>()};
            return ring_t{toler, toler};
          }
          else if constexpr(std::integral<ring_t>)
            return ring_t{};
          else
            return tolerance<ring_t>();
        }()
      };

      if constexpr(orderable)
      {
        return
          [&test=m_Test, tol](std::string_view description, const variant_t& obtained, const variant_t& prediction, const variant_t& parent, std::weak_ordering ordering) {
            if constexpr(has_canonical_rep)
              test.check(equality, description, obtained, prediction);
            else
              test.check(within_tolerance{tol}, description, obtained, prediction);

            if((ordering != std::weak_ordering::equivalent) && (parent.index() == prediction.index()))
              test.check_semantics(description, prediction, parent, ordering);
          };
      }
      else
      {
        return
          [&test=m_Test, tol](std::string_view description, const variant_t& obtained, const variant_t& prediction, const variant_t& parent, std::size_t host, std::size_t target) {
            if constexpr(has_canonical_rep)
              test.check(equality, description, obtained, prediction);
            else
              test.check(within_tolerance{tol}, description, obtained, prediction);

            if((host != target) && (parent.index() == prediction.index()))
              test.check_semantics(description, prediction, parent);
          };
      }
    }

    [[nodiscard]]
    static graph_type make_transition_graph(regular_test& test, dimensionality<1>)
    {
      graph_type g{
        {
          {}, {}, {}, {}, {}, {}, {}
        },
        {
          from_underlying<disp_t>(ring_t(1)),
          from_underlying<disp_t>(ring_t()),
          from_underlying<coords_t>(value_t(2)),
          from_underlying<coords_t>(value_t(1)),
          from_underlying<coords_t>(value_t{}),          
          from_underlying<disp_t>(ring_t(-1)),
          from_underlying<disp_t>(ring_t(-2))
        }
      };

      add_dim_1_common_transitions(g, test);
      add_dim_1_syntactic_sugar_checks(g, test);

      if constexpr(!maths::is_non_negative_orthant_v<space_t>)
      {
        add_dim_1_negative_transitions(g, test);
      }
      else if constexpr((representation_t::bounds_v == maths::half_line_bounds<value_t>) && std::is_signed_v<value_t>)
      {
        add_dim_1_attempted_negative_transitions(g, test);
      }

      if constexpr(has_distinguished_origin)
      {
        add_dim_1_distinguished_origin_transitions(g, test);
      }

      if constexpr(Coordinates::has_freely_mutable_components)
      {
        add_dim_1_free_mutations(g, test);
      }

      if constexpr(std::constructible_from<coords_t, ring_t, ring_t>)
      {
        add_dim_1_no_unit_construction(g, test);
      }

      return g;
    }

    static void add_dim_1_common_transitions(maths::network auto& g, regular_test& test)
    {
      // Joins from zero
      add_transition<coords_t>(
        g,
        dim_1_label::zero,
        dim_1_label::one,
        test.report("(0) + delta(1)"),
        [](variant_t p) -> variant_t { return std::get<coords_t>(p) +  from_underlying<disp_t>(ring_t(1)); }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::zero,
        dim_1_label::one,
        test.report("(0) += delta((1)"),
        [](variant_t p) -> variant_t { return std::get<coords_t>(p) += from_underlying<disp_t>(ring_t(1)); }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::zero,
        dim_1_label::delta_neg_two,
        test.report("(0) - (2)"),
        [](variant_t p) -> variant_t { return std::get<coords_t>(p) - from_underlying<coords_t>(value_t(2)); }
      );

      // Joins from one

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1)  - delta((1)"),
        [](variant_t p) -> variant_t { return std::get<coords_t>(p) -  from_underlying<disp_t>(ring_t(1)); }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1) -= delta((1)"),
        [](variant_t p) -> variant_t { return std::get<coords_t>(p) -= from_underlying<disp_t>(ring_t(1)); }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::one,
        test.report("+(1)"),
        [](variant_t p) -> variant_t { return +std::get<coords_t>(p);}
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1) + delta((1)"),
        [](variant_t p) -> variant_t { return std::get<coords_t>(p) +  from_underlying<disp_t>(ring_t(1)); }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1) += delta((1)"),
        [](variant_t p) -> variant_t { return std::get<coords_t>(p) += from_underlying<disp_t>(ring_t(1)); }
      );

      // Joins from two

      add_transition<coords_t>(
        g,
        dim_1_label::two,
        dim_1_label::one,
        test.report("(2) - delta((1)"),
        [](variant_t p) -> variant_t { return std::get<coords_t>(p) - from_underlying<disp_t>(ring_t(1)); }
      );

      // Joins from delta_neg_one

      add_transition<coords_t>(
        g,
        dim_1_label::delta_neg_one,
        dim_1_label::zero,
        test.report("delta(-1) + (1)"),
        [](variant_t p) -> variant_t { return std::get<disp_t>(p) + from_underlying<coords_t>(value_t(1)); }
      );
    }

    static void add_dim_1_syntactic_sugar_checks([[maybe_unused]] maths::network auto& g, [[maybe_unused]] regular_test& test)
    {
      if constexpr(supports_multiplicative_syntactic_sugar<Coordinates>)
      {
        add_transition<coords_t>(
          g,
          dim_1_label::zero,
          dim_1_label::zero,
          test.report("0 * unit"),
          [](const variant_t&) -> variant_t { return from_underlying(ring_t{}) * basis_isomorphism_t{}; }
        );

        add_transition<coords_t>(
          g,
          dim_1_label::zero,
          dim_1_label::zero,
          test.report("0 / dual<unit>"),
          [](const variant_t&) -> variant_t { return from_underlying(ring_t{}) / maths::dual_of_t<basis_isomorphism_t>{}; }
        );

        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::one,
          test.report("1 * unit"),
          [](const variant_t&) -> variant_t { return from_underlying(ring_t(1)) * basis_isomorphism_t{}; }
        );

        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::one,
          test.report("1 / dual<unit>"),
          [](const variant_t&) -> variant_t { return from_underlying(ring_t(1)) / maths::dual_of_t<basis_isomorphism_t>{}; }
        );
      }
    }

    static void add_dim_1_negative_transitions(maths::network auto& g, regular_test& test)
    {
      g.add_node(from_underlying<coords_t>(std::array{ring_t(-1)}));

      // Joins to neg_one
      if constexpr(has_unary_minus<Coordinates>)
      {
        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::neg_one,
          test.report("-(1)"),
          [](variant_t p) -> variant_t { return -std::get<coords_t>(p); },
          std::is_unsigned_v<ring_t> ? inverted_ordering::yes : inverted_ordering::no
        );
      }

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::neg_one,
        test.report("(1) - (2)"),
        [](variant_t p) -> variant_t { return std::get<coords_t>(p) - from_underlying<disp_t>(ring_t(2)); },
        std::is_unsigned_v<ring_t> ? inverted_ordering::yes : inverted_ordering::no
      );

      
      // Joins from neg_one
      if constexpr(has_unary_minus<Coordinates>)
      {
        add_transition<coords_t>(
          g,
          dim_1_label::neg_one,
          dim_1_label::one,
          test.report("- (-1)"),
          [](variant_t p) -> variant_t { return -std::get<coords_t>(p);  },
          std::is_unsigned_v<ring_t> ? inverted_ordering::yes : inverted_ordering::no
        );
      }
    
      add_transition<coords_t>(
        g,
        dim_1_label::neg_one,
        dim_1_label::neg_one,
        test.report("+ (-1)"),
        [](variant_t p) -> variant_t { return +std::get<coords_t>(p);  }
      );

      if constexpr(Coordinates::has_freely_mutable_components)
      {
        add_transition<coords_t>(
          g,
          dim_1_label::neg_one,
          dim_1_label::zero,
          test.report("(-1) += 1"),
          [](variant_t v) -> variant_t { auto& p{std::get<coords_t>(v)}; auto& val{p.value()}; val += 1; return p; },
          std::is_unsigned_v<ring_t> ? inverted_ordering::yes : inverted_ordering::no
        );

        add_transition<coords_t>(
          g,
          dim_1_label::neg_one,
          dim_1_label::zero,
          test.report("(-1) + 1"),
          [](variant_t v) -> variant_t { auto& p{std::get<coords_t>(v)}; auto& val{p.value()}; val += 1; return p; },
          std::is_unsigned_v<ring_t> ? inverted_ordering::yes : inverted_ordering::no
        );
      }
    }

    static void add_dim_1_attempted_negative_transitions(maths::network auto& g, regular_test& test)
    {
      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::one,
        test.report("(1) -= (2)"),
        [&](variant_t p) -> variant_t {
          test.check_exception_thrown<std::domain_error>(
            "",
            [&]() -> variant_t { return std::get<coords_t>(p) -= from_underlying<disp_t>(ring_t(2));}
          );
          return p;
        }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::one,
        test.report("(1) - (2)"),
        [&](variant_t v) -> variant_t {
          test.check_exception_thrown<std::domain_error>(
            "",
            [&p{std::get<coords_t>(v)}]() -> variant_t { return p = p - from_underlying<disp_t>(ring_t(2)); }
          );
          return v;
        }
      );

      if constexpr(has_distinguished_origin)
      {
        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::one,
          test.report("(1) *= ring_t{-1}"),
          [&](variant_t v) -> variant_t {
            test.check_exception_thrown<std::domain_error>(
              "",
              [&v]() -> variant_t { return std::get<coords_t>(v) *= ring_t{-1}; }
            );
            return v;
          }          
        );

        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::one,
          test.report("ring_t{-1} * (1)"),
          [&test](variant_t v) -> variant_t {
            test.check_exception_thrown<std::domain_error>(
              "",
              [&p{std::get<coords_t>(v)}]() -> variant_t { return p = ring_t{-1} * p; }
            );
            return v;
          }          
        );

        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::one,
          test.report("(1) /= ring_t{-1}"),
          [&test](variant_t v) -> variant_t {
            test.check_exception_thrown<std::domain_error>(
              "",
              [&v]() -> variant_t { return std::get<coords_t>(v) /= ring_t{-1}; }
            );
            return v;
          }
        );

        add_transition<coords_t>(
          g,
          dim_1_label::one,
          dim_1_label::one,
          test.report("(1) / ring_t{-1}"),
          [&test](variant_t v) -> variant_t {
            test.check_exception_thrown<std::domain_error>(
               "",
               [&p{std::get<coords_t>(v)}]() -> variant_t { return p = p / ring_t{-1}; }
            );
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
        [](variant_t v) -> variant_t { return std::get<coords_t>(v) * ring_t{}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("ring_t{} * (1)"),
        [](variant_t v) -> variant_t { return ring_t{} * std::get<coords_t>(v); }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1) *= ring_t{}"),
        [](variant_t v) -> variant_t { return std::get<coords_t>(v) *= ring_t{}; }
      );

      // (1) --> (2)

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1) * ring_t{2}"),
        [](variant_t v) -> variant_t { return std::get<coords_t>(v) * ring_t{2}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("ring_t{2} * (1)"),
        [](variant_t v) -> variant_t { return ring_t{2} * std::get<coords_t>(v); }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1) *= ring_t{2}"),
        [](variant_t v) -> variant_t { return std::get<coords_t>(v) *= ring_t{2}; }
      );

      // (2) --> (1)

      if constexpr(maths::vector_space<module_t>)
      {
        add_transition<coords_t>(
          g,
          dim_1_label::two,
          dim_1_label::one,
          test.report("(2) / ring_t{2}"),
          [](variant_t v) -> variant_t { return std::get<coords_t>(v) / ring_t{2}; }
        );

        add_transition<coords_t>(
          g,
          dim_1_label::two,
          dim_1_label::one,
          test.report("(2) /= ring_t{2}"),
          [](variant_t v) -> variant_t { return std::get<coords_t>(v) /= ring_t{2}; }
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
        [](variant_t v) -> variant_t { std::get<coords_t>(v)[0] *= ring_t{}; return v; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1).begin[0] * ring_t{}"),
        [](variant_t v) -> variant_t { std::get<coords_t>(v).begin()[0] *= ring_t{}; return v; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1).rbegin[0] * ring_t{}"),
        [](variant_t v) -> variant_t { std::get<coords_t>(v).rbegin()[0] *= ring_t{}; return v; }
      );

      // (1) --> (2)

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1)[0] * ring_t{2}"),
        [](variant_t v) -> variant_t { std::get<coords_t>(v)[0] *= ring_t{2}; return v; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1).begin[0] * ring_t{2}"),
        [](variant_t v) -> variant_t { std::get<coords_t>(v).begin()[0] *= ring_t{2}; return v; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1).rbegin[0] * ring_t{2}"),
        [](variant_t v) -> variant_t { std::get<coords_t>(v).rbegin()[0] *= ring_t{2}; return v; }
      );
    }

    static void add_dim_1_no_unit_construction(maths::network auto& g, regular_test& test)
    {
      // (0) --> (1)
      add_transition<coords_t>(
        g,
        dim_1_label::zero,
        dim_1_label::one,
        test.report("(0) +  (1)"),
        [&](variant_t p) -> variant_t { return std::get<coords_t>(p) +  from_underlying<disp_t>(ring_t(1)); }
      );
    }

    [[nodiscard]]
    static graph_type make_transition_graph(regular_test& test, dimensionality<2>)
    {
      graph_type g{
        {
          { // one_two
          }, 
          { // one_one
          }, 
          { // one_zero
          },
          { // zero_one
          }, 
          { // zero_zero
          }, 
        },
        {
          from_underlying<coords_t>(std::array{ring_t(1),  ring_t(2)}),     
          from_underlying<coords_t>(std::array{ring_t(1),  ring_t(1)}),
          from_underlying<coords_t>(std::array{ring_t(1),  ring_t{ }}),
          from_underlying<coords_t>(std::array{ring_t{},   ring_t(1)}),
          from_underlying<coords_t>(std::array{ring_t{},   ring_t{ }})          
        }
      };

      if constexpr(!maths::is_non_negative_orthant_v<space_t>)
      {
        add_dim_2_negative_transitions(g, test);
      }
      else
      {
        add_dim_2_attempted_negative_transitions(g, test);
      }

      if constexpr(has_distinguished_origin)
      {
        add_dim_2_distinguished_origin_transitions(g, test);
      }

      // TO DO: relax last condition, but test values will need ammending.
      // E.g. (1, 1) -> (sqrt(2), pi/4)
      // multiplying last cmpt by 0 -> (sqrt(2), 0)
      if constexpr(Coordinates::has_freely_mutable_components && has_canonical_rep)
      {
        add_dim_2_free_mutations(g, test);
      }

      if constexpr(std::constructible_from<coords_t, ring_t, ring_t>)
      {
        add_dim_2_no_unit_construction(g, test);
      }

      return g;
    }

    static void add_dim_2_negative_transitions(maths::network auto& g, regular_test& test)
    {      
      g.add_node(from_underlying<coords_t>(std::array{ring_t{},   ring_t(-1)}));
      g.add_node(from_underlying<coords_t>(std::array{ring_t(-1), ring_t{}}));
      g.add_node(from_underlying<coords_t>(std::array{ring_t(-1), ring_t(-1)}));

      // (-1, -1) --> (-1, -1)
      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::neg_one_neg_one,
        test.report("+ (-1, -1)"),
        [](variant_t v) -> variant_t { return +std::get<coords_t>(v); }
      );

      // (-1, -1) --> (-1, 0)
      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::neg_one_zero,
        test.report("(-1, -1) +  (0, 1)"),
        [](variant_t v) -> variant_t { return std::get<coords_t>(v) + from_underlying<disp_t>(std::array{ring_t{}, ring_t(1)}); }
      );

      // (-1, -1) --> (-1, 0)
      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::neg_one_zero,
        test.report("(-1, -1) += (0, 1)"),
        [](variant_t v) -> variant_t { return std::get<coords_t>(v) += from_underlying<disp_t>(std::array{ring_t{}, ring_t(1)}); }
      );

      // (-1, -1) --> (0, -1)
      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::zero_neg_one,
        test.report("(-1, -1) +  (1, 0)"),
        [](variant_t v) -> variant_t { return std::get<coords_t>(v) +  from_underlying<disp_t>(std::array{ring_t(1), ring_t{}}); }
     );

      // (-1, -1) --> (0, -1)
      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::zero_neg_one,
        test.report("(-1, -1) += (1, 0)"),
        [](variant_t v) -> variant_t { return std::get<coords_t>(v) +  from_underlying<disp_t>(std::array{ring_t(1), ring_t{}}); }
     );

      if constexpr (has_unary_minus<Coordinates>)
      {
        // (-1, -1) --> (1, 1)   
        add_transition<coords_t>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::one_one,
          test.report("- (-1, -1)"),
          [](variant_t v) -> variant_t { return -std::get<coords_t>(v); }
        );
      }

      if constexpr(has_distinguished_origin)
      {
         // (-1, -1) --> (1, 1)
        add_transition<coords_t>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::one_one,
          test.report("(-1, -1) *= -1"),
          [](variant_t v) -> variant_t { return std::get<coords_t>(v) *= ring_t{-1}; }
        );

        // (-1, -1) --> (0, 0)
        add_transition<coords_t>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::zero_zero,
          test.report("(-1, -1) * 0"),
          [](variant_t v) -> variant_t { return std::get<coords_t>(v) * ring_t{}; }
        );
      }

      if constexpr(has_distinguished_origin && maths::vector_space<module_t>)
      {
        add_transition<coords_t>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::one_one,
          test.report("(-1, -1) /= -1"),
          [](variant_t v) -> variant_t { return std::get<coords_t>(v) /= ring_t{-1}; }
        );

        add_transition<coords_t>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::one_one,
          test.report("(-1, -1) / -1"),
          [](variant_t v) -> variant_t { return std::get<coords_t>(v) / ring_t{-1}; }
        );
      }

      // (-1, 0) --> (-1, -1)
      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_zero,
        dim_2_label::neg_one_neg_one,
        test.report("(-1, 0) -  (0, 1)"),
        [](variant_t v) -> variant_t { return std::get<coords_t>(v) -  from_underlying<disp_t>(std::array{ring_t{}, ring_t(1)}); }
      );

      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_zero,
        dim_2_label::neg_one_neg_one,
        test.report("(-1, 0) -= (0, 1)"),
        [](variant_t v) -> variant_t { return std::get<coords_t>(v) -= from_underlying<disp_t>(std::array{ring_t{}, ring_t(1)}); }
      );

      // (0, -1) --> (-1, -1)
      add_transition<coords_t>(
        g,
        dim_2_label::zero_neg_one,
        dim_2_label::neg_one_neg_one,
        test.report("(0, -1) -  (1, 0)"),
        [](variant_t v) -> variant_t { return std::get<coords_t>(v) -  from_underlying<disp_t>(std::array{ring_t{1}, ring_t(0)}); }
      );

      add_transition<coords_t>(
        g,
        dim_2_label::zero_neg_one,
        dim_2_label::neg_one_neg_one,
        test.report("(0, -1) -= (1, 0)"),
        [](variant_t v) -> variant_t { return std::get<coords_t>(v) -= from_underlying<disp_t>(std::array{ring_t{1}, ring_t(0)}); }
      );
    }

    static void add_dim_2_attempted_negative_transitions(maths::network auto& g, regular_test& test)
    {
      add_transition<coords_t>(
        g,
        dim_2_label::one_one,
        dim_2_label::one_one,
        test.report("(1, 1) -= (2, 2)"),
        [&](variant_t v) -> variant_t {
          test.check_exception_thrown<std::domain_error>(
            "",
            [&]() -> variant_t { return std::get<coords_t>(v) -= from_underlying<disp_t>(std::array{ring_t{2}, ring_t(2)});}
          );
          return v;
        }
      );
    }

    static void add_dim_2_distinguished_origin_transitions(maths::network auto& g, regular_test& test)
    {
      // (1, 1) --> (0, 0)
      add_transition<coords_t>(
       g,
       dim_2_label::one_one,
       dim_2_label::zero_zero,
       test.report("(1, 1) * 0"),
       [](variant_t v) -> variant_t { return std::get<coords_t>(v) * ring_t{}; }
     );
    }

    static void add_dim_2_free_mutations(maths::network auto& g, regular_test& test)
    {
      if constexpr(!maths::is_non_negative_orthant_v<space_t>)
      {
        // (-1, -1) --> (-1, 0)
        add_transition<coords_t>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::neg_one_zero,
          test.report("(-1, -1)[1] *= 0"),
          [](variant_t v) -> variant_t { std::get<coords_t>(v)[1] *= ring_t{}; return v; }
        );

        add_transition<coords_t>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::neg_one_zero,
          test.report("(-1, -1).begin()[1] *= 0"),
          [](variant_t v) -> variant_t { std::get<coords_t>(v).begin()[1] *= ring_t{}; return v; }
        );

        add_transition<coords_t>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::neg_one_zero,
          test.report("(-1, -1).rbegin()[0] *= 0"),
          [](variant_t v) -> variant_t { std::get<coords_t>(v).rbegin()[0] *= ring_t{}; return v; }
        );
      }

      // (0, 1) --> (1, 1)
      add_transition<coords_t>(
        g,
        dim_2_label::zero_one,
        dim_2_label::one_one,
        test.report("(0, 1)[0] += 1"),
        [](variant_t v) -> variant_t { std::get<coords_t>(v)[0] += ring_t{1}; return v; }
      );

      add_transition<coords_t>(
        g,
        dim_2_label::zero_one,
        dim_2_label::one_one,
        test.report("(0, 1).begin[0] += 1"),
        [](variant_t v) -> variant_t { std::get<coords_t>(v).begin()[0] += ring_t{1}; return v; }
      );

      add_transition<coords_t>(
        g,
        dim_2_label::zero_one,
        dim_2_label::one_one,
        test.report("(0, 1).rbegin[1] += 1"),
        [](variant_t v) -> variant_t { std::get<coords_t>(v).rbegin()[1] += ring_t{1}; return v; }
      );
    }

    static void add_dim_2_no_unit_construction(maths::network auto& g, regular_test& test)
    {
      if constexpr(!maths::is_non_negative_orthant_v<space_t>)
      {
        // (-1, -1) --> (-1, -1)
   
        add_transition<coords_t>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::neg_one_neg_one,
          test.report("(-1, -1) without units"),
          [](variant_t v) -> variant_t {
            auto& p{std::get<coords_t>(v)};
            return coords_t{p[0], p[1]};
          }
        );

        add_transition<coords_t>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::neg_one_neg_one,
          test.report("(-1, -1) without units"),
          [](variant_t v) -> variant_t { return coords_t{std::get<coords_t>(v).values()}; }
        );
      }
    }
  };
}
