////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AbsoluteLogarithmicCoordinatesTest.hpp"

#include <cmath>

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    struct logarithmic_representation
    {
      using validator_type = std::identity;

      template<weak_commutative_ring T> 
      [[nodiscard]]
      constexpr static T to_underlying(T val)
      {
        return std::exp(val);
      }

      template<weak_commutative_ring T> 
      [[nodiscard]]
      constexpr static T from_underlying(T val)
      {
        return std::log(val);
      }
      
      template<weak_commutative_ring T>
      [[nodiscard]]
      static constexpr T add(T lhs, T rhs)
      {
        return lhs + rhs;
      }

      template<weak_commutative_ring T>
      [[nodiscard]]
      static constexpr T sub(T lhs, T rhs)
      {
        return lhs - rhs;
      }
    };
  }
  
  [[nodiscard]]
  std::filesystem::path absolute_logarithmic_coordinates_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void absolute_logarithmic_coordinates_test::run_tests()
  {
    test_absolute_logarithmic<float , logarithmic_representation>();
  }

  template<std::floating_point T, class Representation>
  void absolute_logarithmic_coordinates_test::test_absolute_logarithmic()
  {
    using space_t     = euclidean_nonnegative_space<T, 1, mathematical_arena>;
    using coords_t    = coordinates<space_t, canonical_right_handed_basis<free_module_type_of_t<space_t>>, Representation>;
    using delta_t     = coords_t::displacement_coordinates_type;
    using value_t     = T;

    STATIC_CHECK(representation_for_single_value<Representation, space_t>);
    STATIC_CHECK(can_multiply<coords_t, value_t>);
    STATIC_CHECK(can_divide<coords_t, value_t>);
    STATIC_CHECK(!can_divide<coords_t, coords_t>);
    STATIC_CHECK(!can_divide<coords_t, delta_t>);
    STATIC_CHECK(!can_divide<delta_t, coords_t>);
    STATIC_CHECK(!can_divide<delta_t, delta_t>);
    STATIC_CHECK(can_add<coords_t, coords_t>);
    STATIC_CHECK(can_add<coords_t, delta_t>);
    STATIC_CHECK(can_subtract<coords_t, coords_t>);
    STATIC_CHECK(can_subtract<coords_t, delta_t>);
    STATIC_CHECK(has_unary_plus<coords_t>);
    STATIC_CHECK(!has_unary_minus<coords_t>);
    STATIC_CHECK(!coords_t::has_freely_mutable_components);

    using variant_t  = std::variant<coords_t, delta_t>;
    using graph_type = transition_checker<variant_t>::transition_graph;

    enum node_label {neg_one, zero, one, one_plus_ln_two, delta_zero, delta_one, delta_two};

    graph_type g{
      {
        { // neg_one
          {
            node_label::zero,
            report("(-1) + (1)"),
            [](variant_t p) -> variant_t { return std::get<coords_t>(p) +  coords_t{1}; },
            std::weak_ordering::greater
          }
        },
        { // zero
          {
            node_label::one,
            report("(0) + (1)"),
            [](variant_t p) -> variant_t { return std::get<coords_t>(p) +  coords_t{1}; },
            std::weak_ordering::greater
          }
        },
        { // one
          {
            node_label::delta_zero,
            report("(1) - (1)"),
            [](variant_t p) -> variant_t { return std::get<coords_t>(p) -  coords_t{1}; },
            std::weak_ordering::equivalent
          },
          {
            node_label::one_plus_ln_two,
            report("(1) * 2, with the multiplication performed on the underlying values"),
            [](variant_t p) -> variant_t { return std::get<coords_t>(p) * 2; },
            std::weak_ordering::greater
          }
        },
        { // one_plus_ln_two
          {
            node_label::one,
            report("(1) / 2, with the division performed on the underlying values"),
            [](variant_t p) -> variant_t { return std::get<coords_t>(p) / 2; },
            std::weak_ordering::less
          }
        },
        { // delta_zero,
        },
        { // delta_one
        },
        { // delta_two
        }
      },
      {
        coords_t(-1),
        coords_t(0),
        coords_t(1),
        coords_t(1 + std::log(value_t(2))),
         delta_t(0),
         delta_t(1),
         delta_t(2),
      }
    };

    auto checker{
      [this](std::string_view description, const variant_t& obtained, const variant_t& prediction)
      {
        constexpr auto tol{std::same_as<value_t, float> ? value_t(1e-6) : value_t(1e-12)};
        this->check(within_tolerance{tol}, description, obtained, prediction);
      }      
    };

    transition_checker<variant_t>::check("", g, checker);
  }
}
