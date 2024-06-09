////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AngleTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    enum class angle_label{zero, one, two};
  }

  [[nodiscard]]
  std::filesystem::path angle_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void angle_test::run_tests()
  {
    test_angle<float, 360.f>();
    test_angle<double, 360.0>();

    test_angle<float, std::numbers::pi_v<float>>();
    test_angle<double, std::numbers::pi_v<double>>();
  }

  template<std::floating_point T, T Period>
  void angle_test::test_angle()
  {
    using angle_t = angle<T, Period>;

    angle_t theta{}, phi{1};
    check(equivalence, report_line(""), theta, T{});
    check(equivalence, report_line(""), phi, T{1});
    check_semantics(report_line(""), theta, phi, std::weak_ordering::less);

    using angle_graph = transition_checker<angle_t>::transition_graph;
    using edge_t      = transition_checker<angle_t>::edge;

    angle_graph g{
      { { edge_t{1, "0 + 1", [](angle_t theta) -> angle_t { return theta + angle_t{1}; }} }, // 0: zero
        { edge_t{0, "1 - 1", [](angle_t theta) -> angle_t { return theta - angle_t{1}; }} }, // 1: one
      },
      {angle_t{}, angle_t{1}}
    };

    auto checker{
        [this](std::string_view description, angle_t obtained, angle_t prediction) {
          check(equality, description, obtained, prediction);
        }
    };

    transition_checker<angle_t>::check(report_line(""), g, checker);
  }
}
