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
    using angle_t     = angle<T, Period>;
    using angle_graph = transition_checker<angle_t>::transition_graph;
    using edge_t      = transition_checker<angle_t>::edge;

    angle_graph g{
      { {
          edge_t{1, "0 + 1",  [](angle_t theta) -> angle_t { return theta + angle_t{1};  }, std::weak_ordering::greater},
          edge_t{1, "0 += 1", [](angle_t theta) -> angle_t { return theta += angle_t{1}; }, std::weak_ordering::greater}
        }, // 0: zero
        {
          edge_t{0, "1 - 1",     [](angle_t theta) -> angle_t { return theta - angle_t{1};  }, std::weak_ordering::less},
          edge_t{0, "1 -= 1",    [](angle_t theta) -> angle_t { return theta -= angle_t{1}; }, std::weak_ordering::less},
          edge_t{0, "1 * T{}",   [](angle_t theta) -> angle_t { return theta * T{};         }, std::weak_ordering::less},
          edge_t{0, "1 * 0",     [](angle_t theta) -> angle_t { return theta * 0;           }, std::weak_ordering::less},
          edge_t{0, "T{} * 1",   [](angle_t theta) -> angle_t { return T{} * theta;         }, std::weak_ordering::less},
          edge_t{0, "0 * 1",     [](angle_t theta) -> angle_t { return 0  * theta;          }, std::weak_ordering::less},
          edge_t{0, "1 *= T(0)", [](angle_t theta) -> angle_t { return theta *= T{};        }, std::weak_ordering::less},
          edge_t{0, "1 *= 0",    [](angle_t theta) -> angle_t { return theta *= 0;          }, std::weak_ordering::less},
          edge_t{2, "1 + 1",     [](angle_t theta) -> angle_t { return theta + theta;       }, std::weak_ordering::greater},
          edge_t{2, "1 += 1",    [](angle_t theta) -> angle_t { return theta += theta;      }, std::weak_ordering::greater},
          edge_t{2, "1 * T(2)",  [](angle_t theta) -> angle_t { return theta * T(2);        }, std::weak_ordering::greater},
          edge_t{2, "1 * 2",     [](angle_t theta) -> angle_t { return theta * 2;           }, std::weak_ordering::greater},
          edge_t{2, "1 *= T(2)", [](angle_t theta) -> angle_t { return theta *= T(2);       }, std::weak_ordering::greater},
          edge_t{2, "1 *= 2",    [](angle_t theta) -> angle_t { return theta *= 2;          }, std::weak_ordering::greater},
          edge_t{2, "T(2) * 1",  [](angle_t theta) -> angle_t { return T(2) * theta;        }, std::weak_ordering::greater},
          edge_t{2, "2 * 1",     [](angle_t theta) -> angle_t { return 2 * theta;           }, std::weak_ordering::greater}
        }, // 1: one
        { 
          edge_t{1, "2 / T(2)",  [](angle_t theta) -> angle_t { return theta / T(2);       }, std::weak_ordering::less},
          edge_t{1, "2 / 2",     [](angle_t theta) -> angle_t { return theta / 2;          }, std::weak_ordering::less},
          edge_t{1, "2 /= T(2)", [](angle_t theta) -> angle_t { return theta /= T(2);      }, std::weak_ordering::less},
          edge_t{1, "2 /= 2",    [](angle_t theta) -> angle_t { return theta /= 2;         }, std::weak_ordering::less},
          edge_t{1, "2 - 1",     [](angle_t theta) -> angle_t { return theta - angle_t{1}; }, std::weak_ordering::less},
        }, // 1: two
      },
      {angle_t{}, angle_t{1}, angle_t{2}}
    };

    auto checker{
        [this](std::string_view description, angle_t obtained, angle_t prediction, angle_t parent, std::weak_ordering ordering) {
          check(equality, description, obtained, prediction);
          check_semantics(description, prediction, parent, ordering);
        }
    };

    transition_checker<angle_t>::check(report_line(""), g, checker);
  }
}
