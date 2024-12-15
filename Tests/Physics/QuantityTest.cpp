////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "QuantityTest.hpp"

#include "../Maths/Geometry/GeometryTestingUtilities.hpp"

namespace sequoia::testing
{
  using namespace physics;

  namespace{
    template<class T>
    inline constexpr bool has_unary_minus{
      requires(T t){ { -t } -> std::same_as<T>; }
    };

    template<class Quantity>
    struct producer
    {
      using quantity_type = Quantity;
      using unit_type     = typename Quantity::unit_type;

      [[nodiscard]]
      quantity_type operator()(float val) const
      {
        return quantity_type{val, unit_type{}};
      }
    };
  }

  [[nodiscard]]
  std::filesystem::path quantity_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void quantity_test::run_tests()
  {
    test_masses();
  }

  void quantity_test::test_masses()
  {
    static_assert(convex_space<mass_space<float>>);
    static_assert(!has_unary_minus<mass_space<float>>);

    {
      using mass_t = quantity<mass_space<float>, units::kilogram_t>;

      check_exception_thrown<std::domain_error>("Negative mass", [](){ return mass_t{-1.0, units::kilogram}; });

      auto g{coordinates_operations<mass_t>::make_dim_1_transition_graph(producer<mass_t>{})};

      auto checker{
          [this](std::string_view description, const mass_t& obtained, const mass_t& prediction, const mass_t& parent, std::weak_ordering ordering) {
            check(equality, description, obtained, prediction);
            if(ordering != std::weak_ordering::equivalent)
              check_semantics(description, prediction, parent, ordering);
          }
      };

      transition_checker<mass_t>::check(report(""), g, checker);
    }

    {
      using unsafe_mass_t = quantity<mass_space<float>, units::kilogram_t, std::identity>;

      auto g{coordinates_operations<unsafe_mass_t>::make_dim_1_transition_graph(producer<unsafe_mass_t>{})};

      auto checker{
          [this](std::string_view description, const unsafe_mass_t& obtained, const unsafe_mass_t& prediction, const unsafe_mass_t& parent, std::weak_ordering ordering) {
            check(equality, description, obtained, prediction);
            if(ordering != std::weak_ordering::equivalent)
              check_semantics(description, prediction, parent, ordering);
          }
      };

      transition_checker<unsafe_mass_t>::check(report(""), g, checker);
    }
  }
}
