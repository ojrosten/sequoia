////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "CommonGeometryTestingUtilities.hpp"
#include "sequoia/TestFramework/SumTypeCheckers.hpp"

namespace sequoia::testing
{
  template<class Coordinates>
  inline constexpr bool supports_multiplicative_syntactic_sugar{
    requires {
      requires can_multiply<typename Coordinates::value_type, typename Coordinates::frame_type>;
      requires std::same_as<decltype(std::declval<typename Coordinates::value_type>() * std::declval<typename Coordinates::frame_type>()), Coordinates>;
    }
  };
  
  template<class Coordinates>
  class coordinates_operations
  {    
    enum dim_1_label{ delta_one, delta_zero, two, one, zero, delta_neg_one, delta_neg_two, neg_one };
    enum dim_2_label{ one_two, one_one, one_zero, zero_one, zero_zero, zero_neg_one, neg_one_zero, neg_one_neg_one };
    
    using coords_type         = Coordinates;
    using space_type          = Coordinates::space_type;
    using disp_type           = coords_type::displacement_coordinates_type;
    using module_type         = coords_type::free_module_type;
    using disp_value_type     = coords_type::displacement_value_type;
    using representation_type = coords_type::representation_type;
    using value_type          = representation_type::value_type; // TO DO: only just made distinct from disp_value_type. Needs to be properly reasoned through
    using validator_type      = coords_type::validator_type;
    using frame_type          = coords_type::frame_type;
    using variant_type        = std::conditional_t<std::same_as<coords_type, disp_type>, std::variant<coords_type>, std::variant<coords_type, disp_type>>;
    using graph_type          = transition_checker<variant_type>::transition_graph;
    constexpr static std::size_t dimension{Coordinates::dimension};
    constexpr static bool orderable_v{(dimension == 1) && std::totally_ordered<disp_value_type>};
    constexpr static bool has_distinguished_origin_v{maths::has_distinguished_origin_v<space_type>};
    constexpr static bool has_canonical_rep_v{maths::is_canonical_representation_v<representation_type>};

    regular_test& m_Test;
    graph_type m_Graph;
  public:
    explicit coordinates_operations(regular_test& t)
      : m_Test{t}
      , m_Graph{make_graph(m_Test)}
    {}

    void execute()
    {
      transition_checker<variant_type>::check("", m_Graph, make_checker());
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
      return representation_type{}.from_underlying(std::span{vals});
    }

    template<class Coord, class T>
    [[nodiscard]]
    static Coord make_coord(T val) {
      using individual_unit_t = Coord::units_type;
      return Coord{val, individual_unit_t{}};
    }

    // The target's own representation, not the point representation: the two have
    // distinct value types wherever a space is not its own free module.
    template<class To, class T, std::size_t D>
    [[nodiscard]]
    static To from_underlying(const std::array<T, D>& vals)
    {
      using to_representation_t = To::representation_type;
      if constexpr(maths::has_heterogeneous_representation_v<to_representation_t>)
      {        
        return
          [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            using separate_coords_t = to_representation_t::coordinates_type;
            return To{make_coord<std::tuple_element_t<Is, separate_coords_t>>(to_representation_t{}.from_underlying(std::span{vals})[Is])...};
          }(std::make_index_sequence<D>{});
      }
      else
      {
        return To{to_representation_t{}.from_underlying(std::span{vals}), frame_type{}};
      }
    }

    template<class T>
    [[nodiscard]]
    static auto from_underlying(T val) {
      if constexpr(maths::representation_for_span<representation_type, space_type>)
      {
        return from_underlying(std::array{val})[0];
      }
      else
      {
        return representation_type{}.from_underlying(val);
      }
    }

    template<class To, class T>
    [[nodiscard]]
    static To from_underlying(T val) {
      using to_representation_t = To::representation_type;
      if constexpr(maths::representation_for_span<to_representation_t, typename To::space_type>)
      {
        const std::array vals{val};
        return To{to_representation_t{}.from_underlying(std::span{vals})[0], frame_type{}};
      }
      else
      {
        return To{to_representation_t{}.from_underlying(val), frame_type{}};
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
          if constexpr(is_complex_v<disp_value_type>){
            using underlying_value_t = disp_value_type::value_type;
            constexpr auto toler{tolerance<underlying_value_t>()};
            return disp_value_type{toler, toler};
          }
          else if constexpr(std::integral<disp_value_type>)
            return disp_value_type{};
          else
            return tolerance<disp_value_type>();
        }()
      };

      if constexpr(orderable_v)
      {
        return
          [&test=m_Test, tol](std::string_view description, const variant_type& obtained, const variant_type& prediction, const variant_type& parent, std::weak_ordering ordering) {
            if constexpr(has_canonical_rep_v)
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
          [&test=m_Test, tol](std::string_view description, const variant_type& obtained, const variant_type& prediction, const variant_type& parent, std::size_t host, std::size_t target) {
            if constexpr(has_canonical_rep_v)
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
          from_underlying<disp_type>(disp_value_type(1)),
          from_underlying<disp_type>(disp_value_type()),
          from_underlying<coords_type>(value_type(2)),
          from_underlying<coords_type>(value_type(1)),
          from_underlying<coords_type>(value_type{}),          
          from_underlying<disp_type>(disp_value_type(-1)),
          from_underlying<disp_type>(disp_value_type(-2))
        }
      };

      add_dim_1_common_transitions(g, test);
      add_dim_1_syntactic_sugar_checks(g, test);

      if constexpr(!maths::is_non_negative_orthant_v<space_type>)
      {
        add_dim_1_negative_transitions(g, test);
      }
      else if constexpr((representation_type::bounds_v == maths::half_line_bounds<value_type>) && std::is_signed_v<value_type>)
      {
        add_dim_1_attempted_negative_transitions(g, test);
      }

      if constexpr(has_distinguished_origin_v)
      {
        add_dim_1_distinguished_origin_transitions(g, test);
      }

      if constexpr(Coordinates::has_freely_mutable_components_v)
      {
        add_dim_1_free_mutations(g, test);
      }

      if constexpr(std::constructible_from<coords_type, disp_value_type, disp_value_type>)
      {
        add_dim_1_no_unit_construction(g, test);
      }

      return g;
    }

    static void add_dim_1_common_transitions(maths::network auto& g, regular_test& test)
    {
      // Joins from zero
      add_transition<coords_type>(
        g,
        dim_1_label::zero,
        dim_1_label::one,
        test.report("(0) + delta(1)"),
        [](variant_type p) -> variant_type { return std::get<coords_type>(p) +  from_underlying<disp_type>(disp_value_type(1)); }
      );

      add_transition<coords_type>(
        g,
        dim_1_label::zero,
        dim_1_label::one,
        test.report("(0) += delta((1)"),
        [](variant_type p) -> variant_type { return std::get<coords_type>(p) += from_underlying<disp_type>(disp_value_type(1)); }
      );

      add_transition<coords_type>(
        g,
        dim_1_label::zero,
        dim_1_label::delta_neg_two,
        test.report("(0) - (2)"),
        [](variant_type p) -> variant_type { return std::get<coords_type>(p) - from_underlying<coords_type>(value_type(2)); }
      );

      // Joins from one

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1)  - delta((1)"),
        [](variant_type p) -> variant_type { return std::get<coords_type>(p) -  from_underlying<disp_type>(disp_value_type(1)); }
      );

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1) -= delta((1)"),
        [](variant_type p) -> variant_type { return std::get<coords_type>(p) -= from_underlying<disp_type>(disp_value_type(1)); }
      );

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::one,
        test.report("+(1)"),
        [](variant_type p) -> variant_type { return +std::get<coords_type>(p);}
      );

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1) + delta((1)"),
        [](variant_type p) -> variant_type { return std::get<coords_type>(p) +  from_underlying<disp_type>(disp_value_type(1)); }
      );

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1) += delta((1)"),
        [](variant_type p) -> variant_type { return std::get<coords_type>(p) += from_underlying<disp_type>(disp_value_type(1)); }
      );

      // Joins from two

      add_transition<coords_type>(
        g,
        dim_1_label::two,
        dim_1_label::one,
        test.report("(2) - delta((1)"),
        [](variant_type p) -> variant_type { return std::get<coords_type>(p) - from_underlying<disp_type>(disp_value_type(1)); }
      );

      // Joins from delta_neg_one

      add_transition<coords_type>(
        g,
        dim_1_label::delta_neg_one,
        dim_1_label::zero,
        test.report("delta(-1) + (1)"),
        [](variant_type p) -> variant_type { return std::get<disp_type>(p) + from_underlying<coords_type>(value_type(1)); }
      );
    }

    static void add_dim_1_syntactic_sugar_checks([[maybe_unused]] maths::network auto& g, [[maybe_unused]] regular_test& test)
    {
      if constexpr(supports_multiplicative_syntactic_sugar<Coordinates>)
      {
        add_transition<coords_type>(
          g,
          dim_1_label::zero,
          dim_1_label::zero,
          test.report("0 * unit"),
          [](const variant_type&) -> variant_type { return from_underlying(disp_value_type{}) * frame_type{}; }
        );

        add_transition<coords_type>(
          g,
          dim_1_label::zero,
          dim_1_label::zero,
          test.report("0 / dual<unit>"),
          [](const variant_type&) -> variant_type { return from_underlying(disp_value_type{}) / maths::dual_of_t<frame_type>{}; }
        );

        add_transition<coords_type>(
          g,
          dim_1_label::one,
          dim_1_label::one,
          test.report("1 * unit"),
          [](const variant_type&) -> variant_type { return from_underlying(disp_value_type(1)) * frame_type{}; }
        );

        add_transition<coords_type>(
          g,
          dim_1_label::one,
          dim_1_label::one,
          test.report("1 / dual<unit>"),
          [](const variant_type&) -> variant_type { return from_underlying(disp_value_type(1)) / maths::dual_of_t<frame_type>{}; }
        );
      }
    }

    static void add_dim_1_negative_transitions(maths::network auto& g, regular_test& test)
    {
      g.add_node(from_underlying<coords_type>(std::array{value_type{-1}}));

      // Joins to neg_one
      if constexpr(has_unary_minus<Coordinates>)
      {
        add_transition<coords_type>(
          g,
          dim_1_label::one,
          dim_1_label::neg_one,
          test.report("-(1)"),
          [](variant_type p) -> variant_type { return -std::get<coords_type>(p); },
          std::is_unsigned_v<disp_value_type> ? inverted_ordering::yes : inverted_ordering::no
        );
      }

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::neg_one,
        test.report("(1) - (2)"),
        [](variant_type p) -> variant_type { return std::get<coords_type>(p) - from_underlying<disp_type>(disp_value_type(2)); },
        std::is_unsigned_v<disp_value_type> ? inverted_ordering::yes : inverted_ordering::no
      );

      
      // Joins from neg_one
      if constexpr(has_unary_minus<Coordinates>)
      {
        add_transition<coords_type>(
          g,
          dim_1_label::neg_one,
          dim_1_label::one,
          test.report("- (-1)"),
          [](variant_type p) -> variant_type { return -std::get<coords_type>(p);  },
          std::is_unsigned_v<disp_value_type> ? inverted_ordering::yes : inverted_ordering::no
        );
      }
    
      add_transition<coords_type>(
        g,
        dim_1_label::neg_one,
        dim_1_label::neg_one,
        test.report("+ (-1)"),
        [](variant_type p) -> variant_type { return +std::get<coords_type>(p);  }
      );

      if constexpr(Coordinates::has_freely_mutable_components_v)
      {
        add_transition<coords_type>(
          g,
          dim_1_label::neg_one,
          dim_1_label::zero,
          test.report("(-1) += 1"),
          [](variant_type v) -> variant_type { auto& p{std::get<coords_type>(v)}; auto& val{p.value()}; val += 1; return p; },
          std::is_unsigned_v<disp_value_type> ? inverted_ordering::yes : inverted_ordering::no
        );

        add_transition<coords_type>(
          g,
          dim_1_label::neg_one,
          dim_1_label::zero,
          test.report("(-1) + 1"),
          [](variant_type v) -> variant_type { auto& p{std::get<coords_type>(v)}; auto& val{p.value()}; val += 1; return p; },
          std::is_unsigned_v<disp_value_type> ? inverted_ordering::yes : inverted_ordering::no
        );
      }
    }

    static void add_dim_1_attempted_negative_transitions(maths::network auto& g, regular_test& test)
    {
      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::one,
        test.report("(1) -= (2)"),
        [&](variant_type p) -> variant_type {
          test.check_exception_thrown<std::domain_error>(
            "",
            [&]() -> variant_type { return std::get<coords_type>(p) -= from_underlying<disp_type>(disp_value_type(2));}
          );
          return p;
        }
      );

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::one,
        test.report("(1) - (2)"),
        [&](variant_type v) -> variant_type {
          test.check_exception_thrown<std::domain_error>(
            "",
            [&p{std::get<coords_type>(v)}]() -> variant_type { return p = p - from_underlying<disp_type>(disp_value_type(2)); }
          );
          return v;
        }
      );

      if constexpr(has_distinguished_origin_v)
      {
        add_transition<coords_type>(
          g,
          dim_1_label::one,
          dim_1_label::one,
          test.report("(1) *= disp_value_type{-1}"),
          [&](variant_type v) -> variant_type {
            test.check_exception_thrown<std::domain_error>(
              "",
              [&v]() -> variant_type { return std::get<coords_type>(v) *= disp_value_type{-1}; }
            );
            return v;
          }          
        );

        add_transition<coords_type>(
          g,
          dim_1_label::one,
          dim_1_label::one,
          test.report("disp_value_type{-1} * (1)"),
          [&test](variant_type v) -> variant_type {
            test.check_exception_thrown<std::domain_error>(
              "",
              [&p{std::get<coords_type>(v)}]() -> variant_type { return p = disp_value_type{-1} * p; }
            );
            return v;
          }          
        );

        add_transition<coords_type>(
          g,
          dim_1_label::one,
          dim_1_label::one,
          test.report("(1) /= disp_value_type{-1}"),
          [&test](variant_type v) -> variant_type {
            test.check_exception_thrown<std::domain_error>(
              "",
              [&v]() -> variant_type { return std::get<coords_type>(v) /= disp_value_type{-1}; }
            );
            return v;
          }
        );

        add_transition<coords_type>(
          g,
          dim_1_label::one,
          dim_1_label::one,
          test.report("(1) / disp_value_type{-1}"),
          [&test](variant_type v) -> variant_type {
            test.check_exception_thrown<std::domain_error>(
               "",
               [&p{std::get<coords_type>(v)}]() -> variant_type { return p = p / disp_value_type{-1}; }
            );
            return v;
          }          
        );
      }
    }

    static void add_dim_1_distinguished_origin_transitions(maths::network auto& g, regular_test& test)
    {
      // (0) --> (1)
      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1) * disp_value_type{}"),
        [](variant_type v) -> variant_type { return std::get<coords_type>(v) * disp_value_type{}; }
      );

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("disp_value_type{} * (1)"),
        [](variant_type v) -> variant_type { return disp_value_type{} * std::get<coords_type>(v); }
      );

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1) *= disp_value_type{}"),
        [](variant_type v) -> variant_type { return std::get<coords_type>(v) *= disp_value_type{}; }
      );

      // (1) --> (2)

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1) * disp_value_type{2}"),
        [](variant_type v) -> variant_type { return std::get<coords_type>(v) * disp_value_type{2}; }
      );

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("disp_value_type{2} * (1)"),
        [](variant_type v) -> variant_type { return disp_value_type{2} * std::get<coords_type>(v); }
      );

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1) *= disp_value_type{2}"),
        [](variant_type v) -> variant_type { return std::get<coords_type>(v) *= disp_value_type{2}; }
      );

      // (2) --> (1)

      if constexpr(maths::vector_space<module_type>
                   // TO DO: remove this: it's a temporary hack while the field / commutative_ring concepts are sorted out
    && (!std::integral<value_type>)
                   )
      {
        add_transition<coords_type>(
          g,
          dim_1_label::two,
          dim_1_label::one,
          test.report("(2) / disp_value_type{2}"),
          [](variant_type v) -> variant_type { return std::get<coords_type>(v) / disp_value_type{2}; }
        );

        add_transition<coords_type>(
          g,
          dim_1_label::two,
          dim_1_label::one,
          test.report("(2) /= disp_value_type{2}"),
          [](variant_type v) -> variant_type { return std::get<coords_type>(v) /= disp_value_type{2}; }
        );
      }
    }

    static void add_dim_1_free_mutations(maths::network auto& g, regular_test& test)
    {
      // (1) --> (0)
      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1)[0] * disp_value_type{}"),
        [](variant_type v) -> variant_type { std::get<coords_type>(v)[0] *= disp_value_type{}; return v; }
      );

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1).begin[0] * disp_value_type{}"),
        [](variant_type v) -> variant_type { std::get<coords_type>(v).begin()[0] *= disp_value_type{}; return v; }
      );

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1).rbegin[0] * disp_value_type{}"),
        [](variant_type v) -> variant_type { std::get<coords_type>(v).rbegin()[0] *= disp_value_type{}; return v; }
      );

      // (1) --> (2)

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1)[0] * disp_value_type{2}"),
        [](variant_type v) -> variant_type { std::get<coords_type>(v)[0] *= disp_value_type{2}; return v; }
      );

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1).begin[0] * disp_value_type{2}"),
        [](variant_type v) -> variant_type { std::get<coords_type>(v).begin()[0] *= disp_value_type{2}; return v; }
      );

      add_transition<coords_type>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1).rbegin[0] * disp_value_type{2}"),
        [](variant_type v) -> variant_type { std::get<coords_type>(v).rbegin()[0] *= disp_value_type{2}; return v; }
      );
    }

    static void add_dim_1_no_unit_construction(maths::network auto& g, regular_test& test)
    {
      // (0) --> (1)
      add_transition<coords_type>(
        g,
        dim_1_label::zero,
        dim_1_label::one,
        test.report("(0) +  (1)"),
        [&](variant_type p) -> variant_type { return std::get<coords_type>(p) +  from_underlying<disp_type>(disp_value_type(1)); }
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
          from_underlying<coords_type>(std::array{value_type{1},  value_type{2}}),     
          from_underlying<coords_type>(std::array{value_type{1},  value_type{1}}),
          from_underlying<coords_type>(std::array{value_type{1},  value_type{ }}),
          from_underlying<coords_type>(std::array{value_type{},   value_type{1}}),
          from_underlying<coords_type>(std::array{value_type{},   value_type{ }})          
        }
      };

      if constexpr(!maths::is_non_negative_orthant_v<space_type>)
      {
        add_dim_2_negative_transitions(g, test);
      }
      else
      {
        add_dim_2_attempted_negative_transitions(g, test);
      }

      if constexpr(has_distinguished_origin_v)
      {
        add_dim_2_distinguished_origin_transitions(g, test);
      }

      // TO DO: relax last condition, but test values will need ammending.
      // E.g. (1, 1) -> (sqrt(2), pi/4)
      // multiplying last cmpt by 0 -> (sqrt(2), 0)
      if constexpr(Coordinates::has_freely_mutable_components_v && has_canonical_rep_v)
      {
        add_dim_2_free_mutations(g, test);
      }

      if constexpr(std::constructible_from<coords_type, disp_value_type, disp_value_type>)
      {
        add_dim_2_no_unit_construction(g, test);
      }

      return g;
    }

    static void add_dim_2_negative_transitions(maths::network auto& g, regular_test& test)
    {      
      g.add_node(from_underlying<coords_type>(std::array{value_type{},   value_type{-1}}));
      g.add_node(from_underlying<coords_type>(std::array{value_type{-1}, value_type{}}));
      g.add_node(from_underlying<coords_type>(std::array{value_type{-1}, value_type{-1}}));

      // (-1, -1) --> (-1, -1)
      add_transition<coords_type>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::neg_one_neg_one,
        test.report("+ (-1, -1)"),
        [](variant_type v) -> variant_type { return +std::get<coords_type>(v); }
      );

      // (-1, -1) --> (-1, 0)
      add_transition<coords_type>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::neg_one_zero,
        test.report("(-1, -1) +  (0, 1)"),
        [](variant_type v) -> variant_type { return std::get<coords_type>(v) + from_underlying<disp_type>(std::array{disp_value_type{}, disp_value_type(1)}); }
      );

      // (-1, -1) --> (-1, 0)
      add_transition<coords_type>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::neg_one_zero,
        test.report("(-1, -1) += (0, 1)"),
        [](variant_type v) -> variant_type { return std::get<coords_type>(v) += from_underlying<disp_type>(std::array{disp_value_type{}, disp_value_type(1)}); }
      );

      // (-1, -1) --> (0, -1)
      add_transition<coords_type>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::zero_neg_one,
        test.report("(-1, -1) +  (1, 0)"),
        [](variant_type v) -> variant_type { return std::get<coords_type>(v) +  from_underlying<disp_type>(std::array{disp_value_type(1), disp_value_type{}}); }
     );

      // (-1, -1) --> (0, -1)
      add_transition<coords_type>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::zero_neg_one,
        test.report("(-1, -1) += (1, 0)"),
        [](variant_type v) -> variant_type { return std::get<coords_type>(v) +  from_underlying<disp_type>(std::array{disp_value_type(1), disp_value_type{}}); }
     );

      if constexpr (has_unary_minus<Coordinates>)
      {
        // (-1, -1) --> (1, 1)   
        add_transition<coords_type>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::one_one,
          test.report("- (-1, -1)"),
          [](variant_type v) -> variant_type { return -std::get<coords_type>(v); }
        );
      }

      if constexpr(has_distinguished_origin_v)
      {
         // (-1, -1) --> (1, 1)
        add_transition<coords_type>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::one_one,
          test.report("(-1, -1) *= -1"),
          [](variant_type v) -> variant_type { return std::get<coords_type>(v) *= disp_value_type{-1}; }
        );

        // (-1, -1) --> (0, 0)
        add_transition<coords_type>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::zero_zero,
          test.report("(-1, -1) * 0"),
          [](variant_type v) -> variant_type { return std::get<coords_type>(v) * disp_value_type{}; }
        );
      }

      if constexpr(has_distinguished_origin_v && maths::vector_space<module_type>)
      {
        add_transition<coords_type>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::one_one,
          test.report("(-1, -1) /= -1"),
          [](variant_type v) -> variant_type { return std::get<coords_type>(v) /= disp_value_type{-1}; }
        );

        add_transition<coords_type>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::one_one,
          test.report("(-1, -1) / -1"),
          [](variant_type v) -> variant_type { return std::get<coords_type>(v) / disp_value_type{-1}; }
        );
      }

      // (-1, 0) --> (-1, -1)
      add_transition<coords_type>(
        g,
        dim_2_label::neg_one_zero,
        dim_2_label::neg_one_neg_one,
        test.report("(-1, 0) -  (0, 1)"),
        [](variant_type v) -> variant_type { return std::get<coords_type>(v) -  from_underlying<disp_type>(std::array{disp_value_type{}, disp_value_type(1)}); }
      );

      add_transition<coords_type>(
        g,
        dim_2_label::neg_one_zero,
        dim_2_label::neg_one_neg_one,
        test.report("(-1, 0) -= (0, 1)"),
        [](variant_type v) -> variant_type { return std::get<coords_type>(v) -= from_underlying<disp_type>(std::array{disp_value_type{}, disp_value_type(1)}); }
      );

      // (0, -1) --> (-1, -1)
      add_transition<coords_type>(
        g,
        dim_2_label::zero_neg_one,
        dim_2_label::neg_one_neg_one,
        test.report("(0, -1) -  (1, 0)"),
        [](variant_type v) -> variant_type { return std::get<coords_type>(v) -  from_underlying<disp_type>(std::array{disp_value_type{1}, disp_value_type(0)}); }
      );

      add_transition<coords_type>(
        g,
        dim_2_label::zero_neg_one,
        dim_2_label::neg_one_neg_one,
        test.report("(0, -1) -= (1, 0)"),
        [](variant_type v) -> variant_type { return std::get<coords_type>(v) -= from_underlying<disp_type>(std::array{disp_value_type{1}, disp_value_type(0)}); }
      );
    }

    static void add_dim_2_attempted_negative_transitions(maths::network auto& g, regular_test& test)
    {
      add_transition<coords_type>(
        g,
        dim_2_label::one_one,
        dim_2_label::one_one,
        test.report("(1, 1) -= (2, 2)"),
        [&](variant_type v) -> variant_type {
          test.check_exception_thrown<std::domain_error>(
            "",
            [&]() -> variant_type { return std::get<coords_type>(v) -= from_underlying<disp_type>(std::array{disp_value_type{2}, disp_value_type(2)});}
          );
          return v;
        }
      );
    }

    static void add_dim_2_distinguished_origin_transitions(maths::network auto& g, regular_test& test)
    {
      // (1, 1) --> (0, 0)
      add_transition<coords_type>(
       g,
       dim_2_label::one_one,
       dim_2_label::zero_zero,
       test.report("(1, 1) * 0"),
       [](variant_type v) -> variant_type { return std::get<coords_type>(v) * disp_value_type{}; }
     );
    }

    static void add_dim_2_free_mutations(maths::network auto& g, regular_test& test)
    {
      if constexpr(!maths::is_non_negative_orthant_v<space_type>)
      {
        // (-1, -1) --> (-1, 0)
        add_transition<coords_type>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::neg_one_zero,
          test.report("(-1, -1)[1] *= 0"),
          [](variant_type v) -> variant_type { std::get<coords_type>(v)[1] *= disp_value_type{}; return v; }
        );

        add_transition<coords_type>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::neg_one_zero,
          test.report("(-1, -1).begin()[1] *= 0"),
          [](variant_type v) -> variant_type { std::get<coords_type>(v).begin()[1] *= disp_value_type{}; return v; }
        );

        add_transition<coords_type>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::neg_one_zero,
          test.report("(-1, -1).rbegin()[0] *= 0"),
          [](variant_type v) -> variant_type { std::get<coords_type>(v).rbegin()[0] *= disp_value_type{}; return v; }
        );
      }

      // (0, 1) --> (1, 1)
      add_transition<coords_type>(
        g,
        dim_2_label::zero_one,
        dim_2_label::one_one,
        test.report("(0, 1)[0] += 1"),
        [](variant_type v) -> variant_type { std::get<coords_type>(v)[0] += disp_value_type{1}; return v; }
      );

      add_transition<coords_type>(
        g,
        dim_2_label::zero_one,
        dim_2_label::one_one,
        test.report("(0, 1).begin[0] += 1"),
        [](variant_type v) -> variant_type { std::get<coords_type>(v).begin()[0] += disp_value_type{1}; return v; }
      );

      add_transition<coords_type>(
        g,
        dim_2_label::zero_one,
        dim_2_label::one_one,
        test.report("(0, 1).rbegin[1] += 1"),
        [](variant_type v) -> variant_type { std::get<coords_type>(v).rbegin()[1] += disp_value_type{1}; return v; }
      );
    }

    static void add_dim_2_no_unit_construction(maths::network auto& g, regular_test& test)
    {
      if constexpr(!maths::is_non_negative_orthant_v<space_type>)
      {
        // (-1, -1) --> (-1, -1)
   
        add_transition<coords_type>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::neg_one_neg_one,
          test.report("(-1, -1) without units"),
          [](variant_type v) -> variant_type {
            auto& p{std::get<coords_type>(v)};
            return coords_type{p[0], p[1]};
          }
        );

        add_transition<coords_type>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::neg_one_neg_one,
          test.report("(-1, -1) without units"),
          [](variant_type v) -> variant_type { return coords_type{std::get<coords_type>(v).values()}; }
        );
      }
    }
  };
}
