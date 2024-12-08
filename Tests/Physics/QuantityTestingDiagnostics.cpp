////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "QuantityTestingDiagnostics.hpp"

namespace sequoia::testing
{
  using namespace physics;

  [[nodiscard]]
  std::filesystem::path quantity_false_negative_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void quantity_false_negative_test::run_tests()
  {
    using mass_t = quantity<mass_space<float>, units::kilogram_t>;

    mass_t m{1.0, units::kilogram}, m2{2.0, units::kilogram};
    check(equivalence, "", m, 0.0f);
    check(equality, "", m, m2);
  }
}
