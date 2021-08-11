////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StateTransitionDiagnosticsTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{

  struct foo
  {
    double x{};

    [[nodiscard]]
    friend auto operator<=>(const foo&, const foo&) noexcept = default;

    [[nodiscard]]
    friend foo operator-(const foo& lhs, const foo& rhs)
    {
      return {lhs.x - rhs.x};
    }
  };

  [[nodiscard]]
  std::string to_string(const foo& f)
  {
    return std::to_string(f.x);
  }

  [[nodiscard]]
  foo abs(const foo& f)
  {
    return {std::abs(f.x)};
  }

  template<>
  struct detailed_equality_checker<foo>
  {
    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const foo& obtained, const foo& prediction)
    {
      check_equality("value of x", logger, obtained.x, prediction.x);
    }
  };

  [[nodiscard]]
  std::string_view state_transition_diagnostics_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void state_transition_diagnostics_test::run_tests()
  {
    using foo_graph = transition_checker<foo>::transition_graph;
    using edge_t = transition_checker<foo>::edge;

    foo_graph g{
      { { edge_t{1, "Adding 1.1", [](const foo& f) -> foo { return {f.x + 1.1}; }, std::weak_ordering::greater }},

        { edge_t{0, "Subtracting 1.1", [](const foo& f) -> foo { return {f.x - 1.1}; }, std::weak_ordering::less},
          edge_t{2, "Multiplying by 2", [](const foo& f) -> foo { return {f.x * 2}; }, std::weak_ordering::greater} },

        { edge_t{1, "Dividing by 2", [](const foo& f) -> foo { return {f.x / 2}; }, std::weak_ordering::less} }
      },
      {{"Empty", foo{}}, {"1.1", foo{1.1}}, {"2.2", foo{2.2}}}
    };

    {
      auto checker{
        [this](std::string_view description, const foo& obtained, const foo& prediction, const foo& parent, std::weak_ordering ordering) {
          check_equality(description, obtained, prediction);
          check_relation(description, within_tolerance{foo{0.1}}, obtained, prediction);
          check_semantics(description, prediction, parent, ordering);
        }
      };

      transition_checker<foo>::check(LINE(""), g, checker);
    }

    {
      auto checker{
        [this](std::string_view description, const foo& obtained, const foo& prediction) {
          check_equality(description, obtained, prediction);
          check_relation(description, within_tolerance{foo{0.1}}, obtained, prediction);
        }
      };

      transition_checker<foo>::check(LINE(""), g, checker);
    }
  }
}