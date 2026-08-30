////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "AbsoluteLogarithmicCoordinatesTest.hpp"

#include <cmath>

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    template<auto Bounds>
      requires bounds<decltype(Bounds)>
    struct logarithmic_representation
    {
      constexpr static auto bounds_v{Bounds};
      using bounds_type = decltype(Bounds);
      using value_type = bounds_value_type_t<bounds_type>;

      using free_module_representation = logarithmic_representation<bounds_v>;

      [[nodiscard]]
      constexpr static value_type to_underlying(value_type val)
      {
        return std::exp(val);
      }

      [[nodiscard]]
      constexpr static value_type from_underlying(value_type val)
      {
        return std::log(val);
      }

      [[nodiscard]]
      static constexpr value_type add(value_type lhs, value_type rhs)
      {
        return lhs + rhs;
      }

      [[nodiscard]]
      static constexpr value_type sub(value_type lhs, value_type rhs)
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
    test_absolute_logarithmic<float >();
    test_absolute_logarithmic<double>();
  }

  template<std::floating_point T>
  void absolute_logarithmic_coordinates_test::test_absolute_logarithmic()
  {
    using space_t      = euclidean_nonnegative_space<1, mathematical_arena>;
    using rep_t        = logarithmic_representation<no_bounds<T>>;
    using basis_data_t = canonical_basis_data<1>;
    using coords_t     = coordinates<space_t, basis_data_t, rep_t, throwing_validator>;
    using delta_t      = coords_t::displacement_coordinates_type;
    using value_t      = T;

    check_static<(representation_for_single_value<rep_t, space_t>)>();
    check_static<(can_multiply<coords_t, value_t>)>();
    check_static<(can_divide<coords_t, value_t>)>();
    check_static<(!can_divide<coords_t, coords_t>)>();
    check_static<(!can_divide<coords_t, delta_t>)>();
    check_static<(!can_divide<delta_t, coords_t>)>();
    check_static<(!can_divide<delta_t, delta_t>)>();
    check_static<(can_add<coords_t, coords_t>)>();
    check_static<(can_add<coords_t, delta_t>)>();
    check_static<(can_subtract<coords_t, coords_t>)>();
    check_static<(can_subtract<coords_t, delta_t>)>();
    check_static<(has_unary_plus<coords_t>)>();
    check_static<(!has_unary_minus<coords_t>)>();
    check_static<(!coords_t::has_freely_mutable_components)>();
    check_static<(defines_addition_for_single_value_v<space_t, logarithmic_representation<no_bounds<T>>>)>();
    check_static<(defines_subtraction_for_single_value_v<space_t, logarithmic_representation<no_bounds<T>>>)>();

    using variant_t  = std::variant<coords_t, delta_t>;
    using graph_type = transition_checker<variant_t>::transition_graph;

    enum node_label {neg_one, zero, one, one_plus_ln_two, delta_neg_one, delta_zero, delta_one };

    graph_type g{
      {
        { // neg_one
          {
            node_label::zero,
            report("(-1) + (1)"),
            [](variant_t p) -> variant_t { return std::get<coords_t>(p) +  coords_t{1}; },
            std::weak_ordering::greater
          },
          {
            node_label::zero,
            report("(-1) + delta(1)"),
            [](variant_t p) -> variant_t { return std::get<coords_t>(p) +  delta_t{1}; },
            std::weak_ordering::greater
          },
          {
            node_label::delta_neg_one,
            report("(-1) - (0)"),
            [](variant_t p) -> variant_t { return std::get<coords_t>(p) -  coords_t{}; },
            std::weak_ordering::equivalent
          }
        },
        { // zero
          {
            node_label::neg_one,
            report("(0) + delta(-1)"),
            [](variant_t p) -> variant_t { return std::get<coords_t>(p) +  delta_t{-1}; },
            std::weak_ordering::less
          },
          {
            node_label::one,
            report("(0) + (1)"),
            [](variant_t p) -> variant_t { return std::get<coords_t>(p) +  coords_t{1}; },
            std::weak_ordering::greater
          },
          {
            node_label::one,
            report("(0) + delta(1)"),
            [](variant_t p) -> variant_t { return std::get<coords_t>(p) +  delta_t{1}; },
            std::weak_ordering::greater
          }
        },
        { // one
          {
            node_label::delta_zero,
            report("(1) - (1)"),
            [](variant_t p) -> variant_t { return std::get<coords_t>(p) -  coords_t{1}; },
            std::weak_ordering::less
          },
          {
            node_label::one_plus_ln_two,
            report("(1) * 2, with the multiplication performed on the underlying values"),
            [](variant_t p) -> variant_t { return std::get<coords_t>(p) * 2; },
            std::weak_ordering::greater
          },
          {
            node_label::delta_one,
            report("(1) - (0)"),
            [](variant_t p) -> variant_t { return std::get<coords_t>(p) -  coords_t{}; },
            std::weak_ordering::equivalent
          },
        },
        { // one_plus_ln_two
          {
            node_label::one,
            report("(1) / 2, with the division performed on the underlying values"),
            [](variant_t p) -> variant_t { return std::get<coords_t>(p) / 2; },
            std::weak_ordering::less
          }
        },
        { // delta_neg_one,
          {
            node_label::zero,
            report("delta(-1) + (1)"),
            [](variant_t p) -> variant_t { return std::get<delta_t>(p) + coords_t{1}; },
            std::weak_ordering::greater
          },
        },
        { // delta_zero
        },
        { // delta_one
        }
      },
      {
        coords_t(-1),
        coords_t(0),
        coords_t(1),
        coords_t(1 + std::log(value_t(2))),
         delta_t(-1),
         delta_t(0),
         delta_t(1),
      }
    };

    auto checkerFn{
      [this](std::string_view description, const variant_t& obtained, const variant_t& prediction, const variant_t& parent, std::weak_ordering ordering)
      {
        constexpr value_t tol{std::same_as<value_t, float> ? value_t(1e-6) : value_t(1e-12)};
        this->check(within_tolerance{tol}, description, obtained, prediction);

        if((ordering != std::weak_ordering::equivalent) && (parent.index() == prediction.index()))
              this->check_semantics(description, prediction, parent, ordering);
      }      
    };

    transition_checker<variant_t>::check("", g, checkerFn);
  }
}
