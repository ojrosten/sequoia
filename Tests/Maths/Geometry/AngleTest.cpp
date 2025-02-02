////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "AngleTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"
#include <iostream>
namespace sequoia::testing
{
  using namespace maths;

  namespace
  {
    enum angle_label{neg_one, zero, one, two};
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

    test_principal_angle<float>();
    test_principal_angle<double>();

    test_conversions<float>();
    test_conversions<double>();

    test_fp_conversions();

    test_trig<float>();
    test_trig<double>();
  }

  template<std::floating_point T, T Period>
  void angle_test::test_angle()
  {
    using angle_t     = angle<T, Period>;
    using angle_graph = transition_checker<angle_t>::transition_graph;
    using edge_t      = transition_checker<angle_t>::edge;

    angle_graph g{
      { 
        {
          edge_t{angle_label::one,     "- -1",  [](angle_t theta) -> angle_t { return -theta;  }, std::weak_ordering::greater},
          edge_t{angle_label::neg_one, "+ -1",  [](angle_t theta) -> angle_t { return +theta;  }, std::weak_ordering::equivalent}
        }, // neg_one
        {
          edge_t{angle_label::one, "0 + 1",  [](angle_t theta) -> angle_t { return theta + angle_t{1};  }, std::weak_ordering::greater},
          edge_t{angle_label::one, "0 += 1", [](angle_t theta) -> angle_t { return theta += angle_t{1}; }, std::weak_ordering::greater}
        }, // zero
        {
          edge_t{angle_label::neg_one, "-1",        [](angle_t theta) -> angle_t { return -theta;              }, std::weak_ordering::less},
          edge_t{angle_label::zero,    "1 - 1",     [](angle_t theta) -> angle_t { return theta - angle_t{1};  }, std::weak_ordering::less},
          edge_t{angle_label::zero,    "1 -= 1",    [](angle_t theta) -> angle_t { return theta -= angle_t{1}; }, std::weak_ordering::less},
          edge_t{angle_label::zero,    "1 * T{}",   [](angle_t theta) -> angle_t { return theta * T{};         }, std::weak_ordering::less},
          edge_t{angle_label::zero,    "1 * 0",     [](angle_t theta) -> angle_t { return theta * 0;           }, std::weak_ordering::less},
          edge_t{angle_label::zero,    "T{} * 1",   [](angle_t theta) -> angle_t { return T{} * theta;         }, std::weak_ordering::less},
          edge_t{angle_label::zero,    "0 * 1",     [](angle_t theta) -> angle_t { return 0  * theta;          }, std::weak_ordering::less},
          edge_t{angle_label::zero,    "1 *= T(0)", [](angle_t theta) -> angle_t { return theta *= T{};        }, std::weak_ordering::less},
          edge_t{angle_label::zero,    "1 *= 0",    [](angle_t theta) -> angle_t { return theta *= 0;          }, std::weak_ordering::less},
          edge_t{angle_label::one,     "-1",        [](angle_t theta) -> angle_t { return +theta;              }, std::weak_ordering::equivalent},
          edge_t{angle_label::two,     "1 + 1",     [](angle_t theta) -> angle_t { return theta + theta;       }, std::weak_ordering::greater},
          edge_t{angle_label::two,     "1 += 1",    [](angle_t theta) -> angle_t { return theta += theta;      }, std::weak_ordering::greater},
          edge_t{angle_label::two,     "1 * T(2)",  [](angle_t theta) -> angle_t { return theta * T(2);        }, std::weak_ordering::greater},
          edge_t{angle_label::two,     "1 * 2",     [](angle_t theta) -> angle_t { return theta * 2;           }, std::weak_ordering::greater},
          edge_t{angle_label::two,     "1 *= T(2)", [](angle_t theta) -> angle_t { return theta *= T(2);       }, std::weak_ordering::greater},
          edge_t{angle_label::two,     "1 *= 2",    [](angle_t theta) -> angle_t { return theta *= 2;          }, std::weak_ordering::greater},
          edge_t{angle_label::two,     "T(2) * 1",  [](angle_t theta) -> angle_t { return T(2) * theta;        }, std::weak_ordering::greater},
          edge_t{angle_label::two,     "2 * 1",     [](angle_t theta) -> angle_t { return 2 * theta;           }, std::weak_ordering::greater}
        }, // one
        { 
          edge_t{angle_label::one, "2 / T(2)",  [](angle_t theta) -> angle_t { return theta / T(2);       }, std::weak_ordering::less},
          edge_t{angle_label::one, "2 / 2",     [](angle_t theta) -> angle_t { return theta / 2;          }, std::weak_ordering::less},
          edge_t{angle_label::one, "2 /= T(2)", [](angle_t theta) -> angle_t { return theta /= T(2);      }, std::weak_ordering::less},
          edge_t{angle_label::one, "2 /= 2",    [](angle_t theta) -> angle_t { return theta /= 2;         }, std::weak_ordering::less},
          edge_t{angle_label::one, "2 - 1",     [](angle_t theta) -> angle_t { return theta - angle_t{1}; }, std::weak_ordering::less},
        }, // two
      },
      {angle_t{-1}, angle_t{}, angle_t{1}, angle_t{2}}
    };

    auto checker{
        [this](std::string_view description, angle_t obtained, angle_t prediction, angle_t parent, std::weak_ordering ordering) {
          check(equality, description, obtained, prediction);
          if(ordering != std::weak_ordering::equivalent)
            check_semantics(description, prediction, parent, ordering);
        }
    };

    transition_checker<angle_t>::check(report(""), g, checker);
  }

  template<std::floating_point T>
  void angle_test::test_principal_angle()
  {
    constexpr auto pi{std::numbers::pi_v<T>};
    check(equality, "", radians<T>{ pi / 2}.principal_angle(), radians<T>{ pi/2});
    check(equality, "", radians<T>{-pi / 2}.principal_angle(), radians<T>{-pi/2});
    check(equality, "", radians<T>{ 2 * pi}.principal_angle(), radians<T>{});
    check(equality, "", radians<T>{-2 * pi}.principal_angle(), radians<T>{});
    // check(equality, "", radians{5 * pi / 2}.principal_angle(), radians<T>{pi/2});

    check(equality, "", degrees<T>{ 180}.principal_angle(), degrees<T>{ 180});
    check(equality, "", degrees<T>{-180}.principal_angle(), degrees<T>{-180});
    check(equality, "", degrees<T>{ 360}.principal_angle(), degrees<T>{});
    check(equality, "", degrees<T>{-360}.principal_angle(), degrees<T>{});
    check(equality, "", degrees<T>{ 540}.principal_angle(), degrees<T>{ 180});
    check(equality, "", degrees<T>{-540}.principal_angle(), degrees<T>{-180});
  }

  template<std::floating_point T>
  void angle_test::test_conversions()
  {
    constexpr auto pi{std::numbers::pi_v<T>};

    check(equality, "", to_degrees(radians<T>{}),    degrees<T>{});
    check(equality, "", to_degrees(radians<T>{pi}),  degrees<T>{180});
    check(equality, "", to_degrees(radians<T>{-pi}), degrees<T>{-180});

    check(equality, "", to_radians(degrees<T>{}),     radians<T>{});
    check(equality, "", to_radians(degrees<T>{180}),  radians<T>{pi});
    check(equality, "", to_radians(degrees<T>{-180}), radians<T>{-pi});
  }

  void angle_test::test_fp_conversions()
  {
    check(equality, "", convert<float(360)>(degrees<double>{20.3}), degrees<float>{20.3f});
  }

  template<std::floating_point T>
  void angle_test::test_trig()
  {
    constexpr auto pi{std::numbers::pi_v<T>};

    check(equality, "", sin(radians<T>{-pi / 2}), -T(1));
    check(equality, "", sin(radians<T>{}), T{});
    check(equality, "", sin(radians<T>{pi/2}), T(1));
    check(equality, "", sin(degrees<T>{-90}), -T(1));
    check(equality, "", sin(degrees<T>{}), T{});
    check(equality, "", sin(degrees<T>{90}), T(1));

    check(equality, "", cos(radians<T>{-pi / 2}), std::cos(-pi/2));
    check(equality, "", cos(radians<T>{}), T{1});
    check(equality, "", cos(radians<T>{pi / 2}), std::cos(pi / 2));
    check(equality, "", cos(degrees<T>{-90}), std::cos(-pi / 2));
    check(equality, "", cos(degrees<T>{}), T{1});
    check(equality, "", cos(degrees<T>{90}), std::cos(pi / 2));

    check(equality, "", tan(radians<T>{-pi / 2}), std::tan(-pi / 2));
    check(equality, "", tan(radians<T>{}), T{});
    check(equality, "", tan(radians<T>{pi / 2}), std::tan(pi / 2));
    check(equality, "", tan(degrees<T>{-90}), std::tan(-pi / 2));
    check(equality, "", tan(degrees<T>{}), T{});
    check(equality, "", tan(degrees<T>{90}), std::tan(pi / 2));

    check(equality, "", asin(T{}), radians<T>{});
    // For some reason, libc++'s answer is slightly off
    check(within_tolerance{radians<T>(T(1e-6))}, "", asin(T{1}), radians<T>{pi/2});

    check(equality, "", acos(T{}), radians<T>{pi / 2});
    check(equality, "", acos(T{1}), radians<T>{});

    check(equality, "", atan(T{}), radians<T>{});
    check(equality, "", atan(T{1}), radians<T>{pi / 4});
  }
}
