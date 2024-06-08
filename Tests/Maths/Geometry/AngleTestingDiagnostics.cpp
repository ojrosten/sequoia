////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AngleTestingDiagnostics.hpp"

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::filesystem::path angle_false_positive_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void angle_false_positive_test::run_tests()
  {
    test_angle<float, 360.f>();
    test_angle<double, 360.0>();

    test_angle<float, std::numbers::pi_v<float>>();
    test_angle<double, std::numbers::pi_v<double>>();
  }

  template<std::floating_point T, T Period>
  void angle_false_positive_test::test_angle()
  {
    angle<T, Period> theta{}, phi{1};
    check(equivalence, report_line(""), theta, T{1});
    check(equivalence, report_line(""), phi, T{});
    check(equality, report_line(""), theta, phi);
  }
}
