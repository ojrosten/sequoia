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
  class coordinates_operations
  {    
    enum dim_1_label{ two, one, zero, neg_one };
    enum dim_2_label{ neg_one_neg_one, neg_one_zero, zero_neg_one, zero_zero, zero_one, one_zero, one_one, one_two };
    
    using graph_type = transition_checker<Coordinates>::transition_graph;
    using coords_t   = Coordinates;
    using space_t    = Coordinates::space_type;
    using disp_t     = coords_t::displacement_coordinates_type;
    using module_t   = coords_t::free_module_type;
    using ring_t     = coords_t::commutative_ring_type;
    using units_t    = coords_t::basis_isomorphism_type;
    constexpr static std::size_t dimension{Coordinates::dimension};
    constexpr static bool orderable{(dimension == 1) && std::totally_ordered<ring_t>};
    constexpr static bool has_distinguished_origin{maths::has_distinguished_origin_v<space_t>};

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
      return do_make_graph(test);
    }
    
    [[nodiscard]]
    static graph_type do_make_graph(regular_test& test)
    {
      if constexpr     (dimension == 1) return make_dim_1_transition_graph(test);
      else if constexpr(dimension == 2) return make_dim_2_transition_graph(test);
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

    static graph_type make_dim_1_transition_graph(regular_test& test)
    {
      graph_type g{
        {
          {}, {}, {}
        },
        {coords_t{ring_t(2), units_t{}}, coords_t{ring_t(1), units_t{}}, coords_t{}}
      };

      add_dim_1_common_transitions(g, test);

      if constexpr(has_unary_minus<Coordinates>)
      {
        add_dim_1_negative_transitions(g, test);
      }
      else if constexpr(maths::defines_half_line_validator_v<typename Coordinates::validator_type> && std::is_signed_v<ring_t>)
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

    static graph_type make_dim_2_transition_graph(regular_test& test)
    {
      using edge_t = transition_checker<coords_t>::edge;
      graph_type g{
        {
          {
            edge_t{dim_2_label::neg_one_neg_one, test.report("+ (-1, -1)"),          [](coords_t v) -> coords_t { return +v; }},
            edge_t{dim_2_label::neg_one_zero,    test.report("(-1, -1) +  (0, 1)"),  [&](coords_t v) -> coords_t { return v +  disp_t{std::array{ring_t{}, ring_t(1)}, units_t{}}; }},
            edge_t{dim_2_label::neg_one_zero,    test.report("(-1, -1) += (0, 1)"),  [&](coords_t v) -> coords_t { return v += disp_t{std::array{ring_t{}, ring_t(1)}, units_t{}}; }},
            edge_t{dim_2_label::zero_neg_one,    test.report("(-1, -1) +  (1, 0)"),  [&](coords_t v) -> coords_t { return v +  disp_t{std::array{ring_t(1), ring_t{}}, units_t{}}; }},
            edge_t{dim_2_label::zero_neg_one,    test.report("(-1, -1) += (1, 0)"),  [&](coords_t v) -> coords_t { return v += disp_t{std::array{ring_t(1), ring_t{}}, units_t{}}; }}
          }, // neg_one_neg_one
          {
            edge_t{dim_2_label::neg_one_neg_one, test.report("(-1, 0) -  (0, 1)"),  [&](coords_t v) -> coords_t { return v -  disp_t{std::array{ring_t{}, ring_t(1)}, units_t{}}; }},
            edge_t{dim_2_label::neg_one_neg_one, test.report("(-1, 0) -= (0, 1)"),  [&](coords_t v) -> coords_t { return v -= disp_t{std::array{ring_t{}, ring_t(1)}, units_t{}}; }}
          }, // neg_one_zero
          {
            edge_t{dim_2_label::neg_one_neg_one, test.report("(0, -1) -  (1, 0)"),  [&](coords_t v) -> coords_t { return v -  disp_t{std::array{ring_t{1}, ring_t(0)}, units_t{}}; }},
            edge_t{dim_2_label::neg_one_neg_one, test.report("(0, -1) -= (1, 0)"),  [&](coords_t v) -> coords_t { return v -= disp_t{std::array{ring_t{1}, ring_t(0)}, units_t{}}; }}
          }, // zero_neg_one
          {
          }, // zero_zero
          {
          }, // zero_one
          {
          }, // one_zero
          {
          }, // one_one
          {
          }, // one_two
        },
        {coords_t{std::array{ring_t(-1), ring_t(-1)}, units_t{}},
         coords_t{std::array{ring_t(-1), ring_t{}},   units_t{}},
         coords_t{std::array{ring_t{},   ring_t(-1)}, units_t{}},
         coords_t{std::array{ring_t{},   ring_t{}},   units_t{}},
         coords_t{std::array{ring_t{},   ring_t(1)},  units_t{}},
         coords_t{std::array{ring_t(1),  ring_t{}},   units_t{}},
         coords_t{std::array{ring_t(1),  ring_t(1)},  units_t{}},
         coords_t{std::array{ring_t(1),  ring_t(2)},  units_t{}}
        }
      };

      if constexpr(has_distinguished_origin)
      {
        add_dim_2_distinguished_origin_transitions(g, test);
      }

      if constexpr(Coordinates::has_freely_mutable_components)
      {
        add_dim_2_free_mutations(g, test);
      }

      if constexpr(std::constructible_from<coords_t, ring_t, ring_t>)
      {
        add_dim_2_no_unit_construction(g, test);
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
        test.report("(0) +  (1)"),
        [&](coords_t p) -> coords_t { return p +  disp_t{ring_t(1), units_t{}}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::zero,
        dim_1_label::one,
        test.report("(0) += (1)"),
        [&](coords_t p) -> coords_t { return p += disp_t{ring_t(1), units_t{}}; }
      );

      // Joins from one

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1)  - (1)"),
        [&](coords_t p) -> coords_t { return p -  disp_t{ring_t(1), units_t{}}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1) -= (1)"),
        [&](coords_t p) -> coords_t { return p -= disp_t{ring_t(1), units_t{}}; }
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
        [&](coords_t p) -> coords_t { return p +  disp_t{ring_t(1), units_t{}}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1) += (1)"),
        [&](coords_t p) -> coords_t { return p += disp_t{ring_t(1), units_t{}}; }
      );

      // Joins from two

      add_transition<coords_t>(
        g,
        dim_1_label::two,
        dim_1_label::one,
        test.report("(2) - (1)"),
        [&](coords_t p) -> coords_t { return p - disp_t{ring_t(1), units_t{}}; }
      );
    }

    template<class... Units>
    static void add_dim_1_negative_transitions(maths::network auto& g, regular_test& test)
    {
      g.add_node(ring_t(-1), units_t{});

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
        [&](coords_t p) -> coords_t { return p - disp_t{ring_t(2), units_t{}}; },
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

    static void add_dim_1_attempted_negative_transitions(maths::network auto& g, regular_test& test)
    {
      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::one,
        test.report("(1) -= (2)"),
        [&](coords_t p) -> coords_t {
          test.check_exception_thrown<std::domain_error>("", [&](){ return p -= disp_t{ring_t(2), units_t{}};});
          return p;
        }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::one,
        test.report("(1) - (2)"),
        [&](coords_t p) -> coords_t {
          test.check_exception_thrown<std::domain_error>("", [&](){ return p = (p - disp_t{ring_t(2), units_t{}}); });
          return p;
        }
      );

      if constexpr(has_distinguished_origin)
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
        test.report("(1) * span{ring_t{}}"),
        [](coords_t v) -> coords_t { return v * std::array{ring_t{}}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("ring_t{} * (1)"),
        [](coords_t v) -> coords_t { return ring_t{} * v; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("span{ring_t{}} * (1)"),
        [](coords_t v) -> coords_t { return std::array{ring_t{}} * v; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1) *= ring_t{}"),
        [](coords_t v) -> coords_t { return v *= ring_t{}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::zero,
        test.report("(1) *= span{ring_t{}}"),
        [](coords_t v) -> coords_t { return v *= std::array{ring_t{}}; }
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
        test.report("(1) * array{ring_t{2}}"),
        [](coords_t v) -> coords_t { return v * std::array{ring_t{2}}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("ring_t{2} * (1)"),
        [](coords_t v) -> coords_t { return ring_t{2} * v; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("span{ring_t{2}} * (1)"),
        [](coords_t v) -> coords_t { return std::array{ring_t{2}} * v; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1) *= ring_t{2}"),
        [](coords_t v) -> coords_t { return v *= ring_t{2}; }
      );

      add_transition<coords_t>(
        g,
        dim_1_label::one,
        dim_1_label::two,
        test.report("(1) *= span{ring_t{2}}"),
        [](coords_t v) -> coords_t { return v *= std::array{ring_t{2}}; }
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
          test.report("(2) / span{ring_t{2}}"),
          [](coords_t v) -> coords_t { return v / std::array{ring_t{2}}; }
        );

        add_transition<coords_t>(
          g,
          dim_1_label::two,
          dim_1_label::one,
          test.report("(2) /= ring_t{2}"),
          [](coords_t v) -> coords_t { return v /= ring_t{2}; }
        );

        add_transition<coords_t>(
          g,
          dim_1_label::two,
          dim_1_label::one,
          test.report("(2) /= span{ring_t{2}}"),
          [](coords_t v) -> coords_t { return v /= std::array{ring_t{2}}; }
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

    static void add_dim_1_no_unit_construction(maths::network auto& g, regular_test& test)
    {
      // (0) --> (1)
      add_transition<coords_t>(
        g,
        dim_1_label::zero,
        dim_1_label::one,
        test.report("(0) +  (1)"),
        [&](coords_t p) -> coords_t { return p +  disp_t{ring_t(1)}; }
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
        test.report("(-1, -1) *= span{-1, -1}"),
        [](coords_t v) -> coords_t { return v *= std::array<ring_t, 2>{-1, -1}; }
      );

      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::one_one,
        test.report("(-1, -1) * -1"),
        [](coords_t v) -> coords_t { return v * ring_t{-1}; }
      );

      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::one_one,
        test.report("(-1, -1) * span{-1, -1}"),
        [](coords_t v) -> coords_t { return v * std::array<ring_t, 2>{-1, -1}; }
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
          test.report("(-1, -1) /= span{-1, -1}"),
          [](coords_t v) -> coords_t { return v /= std::array<ring_t, 2>{-1, -1}; }
        );

        add_transition<coords_t>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::one_one,
          test.report("(-1, -1) / -1"),
          [](coords_t v) -> coords_t { return v / ring_t{-1}; }
        );

         add_transition<coords_t>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::one_one,
          test.report("(-1, -1) / span{-1, -1}"),
          [](coords_t v) -> coords_t { return v / std::array<ring_t, 2>{-1, -1}; }
        );
      }

      // (-1, -1) --> (1, 0)
      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::one_zero,
        test.report("(-1, -1) *= span{-1, 0}"),
        [](coords_t v) -> coords_t { return v *= std::array<ring_t, 2>{-1, 0}; }
      );

      // (-1, -1) --> (1, 2)
      if constexpr(maths::vector_space<module_t>)
      {
        add_transition<coords_t>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::one_two,
          test.report("(-1, -1) /= span{-1, -0.5}"),
          [](coords_t v) -> coords_t { return v /= std::array<ring_t, 2>{-1, -0.5}; }
        );

        add_transition<coords_t>(
          g,
          dim_2_label::neg_one_neg_one,
          dim_2_label::one_two,
          test.report("(-1, -1) / span{-1, -0.5}"),
          [](coords_t v) -> coords_t { return v / std::array<ring_t, 2>{-1, -0.5}; }
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

    static void add_dim_2_no_unit_construction(maths::network auto& g, regular_test& test)
    {
      // (-1, -1) --> (-1, -1)
   
      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::neg_one_neg_one,
        test.report("(-1, -1) without units"),
        [](coords_t v) -> coords_t { return {v[0], v[1]}; }
      );

      add_transition<coords_t>(
        g,
        dim_2_label::neg_one_neg_one,
        dim_2_label::neg_one_neg_one,
        test.report("(-1, -1) without units"),
        [](coords_t v) -> coords_t { return coords_t{v.values()}; }
      );
    }
  };
}
