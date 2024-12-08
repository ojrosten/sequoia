////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "QuantityTest.hpp"

namespace sequoia::testing
{
  using namespace physics;

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
    static_assert(quantity_space<mass_space<float>>);

    using mass_t = quantity<mass_space<float>, units::kilogram_t>;

    check_exception_thrown<std::domain_error>("Negative mass", [](){ return mass_t{-1.0, units::kilogram}; });

    mass_t m{1.0, units::kilogram}, m2{2.0, units::kilogram};
    check(equivalence, "", m, 1.0f);
    check(equivalence, "", m2, 2.0f);

    check_semantics("", m, m2, std::weak_ordering::less);
  }
}
