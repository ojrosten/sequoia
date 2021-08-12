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
      double x{};

      [[nodiscard]]
      friend auto operator<=>(const bar&, const bar&) noexcept = default;

      [[nodiscard]]
      friend bar operator-(const bar& lhs, const bar& rhs)
      {
        return {lhs.x - rhs.x};
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
      return {std::abs(f.x)};
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
      { { edge_t{1, "Adding 1.1", [](const bar& f) -> bar { return {f.x + 1.1}; }, std::weak_ordering::greater }},

        { edge_t{0, "Subtracting 1.1", [](const bar& f) -> bar { return {f.x - 1.1}; }, std::weak_ordering::less},
          edge_t{2, "Multiplying by 2", [](const bar& f) -> bar { return {f.x * 2}; }, std::weak_ordering::greater} },

        { edge_t{1, "Dividing by 2", [](const bar& f) -> bar { return {f.x / 2}; }, std::weak_ordering::less} }
      },
      {{"Empty", bar{}}, {"1.1", bar{1.1}}, {"2.2", bar{2.2}}}
    };

    /*{
      auto checker{
        [this](std::string_view description, const bar& obtained, const bar& prediction, const bar& parent, std::weak_ordering ordering) {
          check_equality(description, obtained, prediction);
          check_relation(description, within_tolerance{bar{0.1}}, obtained, prediction);
          check_semantics(description, prediction, parent, ordering);
        }
      };

      transition_checker<bar>::check(LINE(""), g, checker);
    }

    {
      auto checker{
        [this](std::string_view description, const bar& obtained, const bar& prediction) {
          check_equality(description, obtained, prediction);
          check_relation(description, within_tolerance{bar{0.1}}, obtained, prediction);
        }
      };

      transition_checker<bar>::check(LINE(""), g, checker);
    }*/
  }
}
