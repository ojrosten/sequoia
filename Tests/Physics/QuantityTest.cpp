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

      mass_t m{1.0, units::kilogram};
      constexpr mass_t m2{2.0, units::kilogram};
      check(equivalence, "", m, 1.0f);
      check(equivalence, "", m2, 2.0f);

      check_semantics("", m, m2, std::weak_ordering::less);
    }

    {
      using unsafe_mass_t = quantity<mass_space<float>, units::kilogram_t, std::identity>;

      auto g{coordinates_operations::make_dim_1_orderable_transition_graph<unsafe_mass_t>(units::kilogram)};

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
