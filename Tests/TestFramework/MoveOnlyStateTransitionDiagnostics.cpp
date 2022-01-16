////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "MoveOnlyStateTransitionDiagnostics.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  namespace
  {
    struct bar
    {
      bar() = default;

      explicit bar(double val)
        : x{val}
      {}

      bar(const bar&)     = delete;
      bar(bar&&) noexcept = default;

      bar& operator=(const bar&)     = delete;
      bar& operator=(bar&&) noexcept = default;

      double x{};

      [[nodiscard]]
      friend auto operator<=>(const bar&, const bar&) noexcept = default;

      [[nodiscard]]
      friend bar operator-(const bar& lhs, const bar& rhs)
      {
        return bar{lhs.x - rhs.x};
      }

      template<class Stream>
      friend Stream& operator<<(Stream& s, const bar& b)
      {
        s << b.x;
        return s;
      }
    };

    [[nodiscard]]
    std::string to_string(const bar& f)
    {
      return std::to_string(f.x);
    }

    [[nodiscard]]
    bar abs(const bar& f)
    {
      return bar{std::abs(f.x)};
    }
  }

  [[nodiscard]]
  std::string_view move_only_state_transition_false_negative_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void move_only_state_transition_false_negative_diagnostics::run_tests()
  {
    using bar_graph = transition_checker<bar>::transition_graph;
    using edge_t = transition_checker<bar>::edge;

    bar_graph g{
      { { edge_t{1, "Adding 1.1", [](const bar& f) -> bar { return bar{f.x + 1.1}; }, std::weak_ordering::greater }},

        { edge_t{0, "Subtracting 1.1", [](const bar& f) -> bar { return bar{f.x - 1.1}; }, std::weak_ordering::less},
          edge_t{2, "Multiplying by 2", [](const bar& f) -> bar { return bar{f.x * 2}; }, std::weak_ordering::greater} },

        { edge_t{1, "Dividing by 2", [](const bar& f) -> bar { return bar{f.x / 2}; }, std::weak_ordering::less} }
      },
      {{}, {1.1}, {2.2}}
    };

    {
      auto checker{
        [this](std::string_view description, std::function<bar()> obtained, std::function<bar()> prediction, std::function<bar()> parent, std::weak_ordering ordering) {
          check(equality, description, obtained(), prediction());
          check(within_tolerance{bar{0.1}}, description, obtained(), prediction());
          check_semantics(description, prediction, parent, ordering);
        }
      };

      transition_checker<bar>::check(LINE(""), g, checker);
    }

    {
      auto checker{
        [this](std::string_view description, const bar& obtained, const bar& prediction) {
          check(equality, description, obtained, prediction);
          check(within_tolerance{bar{0.1}}, description, obtained, prediction);
        }
      };

      transition_checker<bar>::check(LINE(""), g, checker);
    }
  }

  [[nodiscard]]
  std::string_view move_only_state_transition_false_positive_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void move_only_state_transition_false_positive_diagnostics::run_tests()
  {
    using bar_graph = transition_checker<bar>::transition_graph;
    using edge_t = transition_checker<bar>::edge;
    using node_weight_t = transition_checker<bar>::transition_graph::node_weight_type;
    // Note: the use of node_weight_t below only seems to be necessary
    // with MSVC

    bar_graph g{
      { { edge_t{1, "Adding 1.1", [](const bar& f) -> bar { return bar{f.x + 1.0}; }, std::weak_ordering::greater }},
        { edge_t{0, "Subtracting 1.1", [](const bar& f) -> bar { return bar{f.x - 1.0}; }, std::weak_ordering::less} },
      },
      {node_weight_t{}, node_weight_t{1.1}}
    };

    auto checker{
        [this](std::string_view description, const bar& obtained, const bar& prediction) {
          check(equality, description, obtained, prediction);
        }
    };

    transition_checker<bar>::check(LINE("Mistake in transition functions"), g, checker);
  }
}
