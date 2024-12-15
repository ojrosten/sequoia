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

      coordinates_operations<mass_t>{*this, producer<mass_t>{}}.execute();
    }

    {
      using unsafe_mass_t = quantity<mass_space<float>, units::kilogram_t, std::identity>;

      coordinates_operations<unsafe_mass_t>{*this, producer<unsafe_mass_t>{}}.execute();
    }
  }
}
