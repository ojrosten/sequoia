////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "MoveOnlyStateTransitionDiagnostics.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <complex>

namespace sequoia::testing
{
  namespace
  {
    template<class T>
    struct foo
    {
      foo() = default;

      explicit foo(T val)
        : z{val}
      {}

      foo(const foo&)     = delete;
      foo(foo&&) noexcept = default;

      foo& operator=(const foo&)     = delete;
      foo& operator=(foo&&) noexcept = default;

      T z{};

      [[nodiscard]]
      friend auto operator<=>(const foo&, const foo&) noexcept = default;

      [[nodiscard]]
      friend foo operator-(const foo& lhs, const foo& rhs)
      {
        return foo{lhs.z - rhs.z};
      }

      template<class Stream>
      friend Stream& operator<<(Stream& s, const foo& b)
      {
        s << b.z;
        return s;
      }
    };

    template<class T>
    [[nodiscard]]
    double abs(const foo<T>& f)
    {
      return std::abs(f.z);
    }
  }

  [[nodiscard]]
  std::filesystem::path move_only_state_transition_false_negative_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void move_only_state_transition_false_negative_diagnostics::run_tests()
  {
    test_orderable();
    test_equality_comparable();
  }

  void move_only_state_transition_false_negative_diagnostics::test_orderable()
  {
    using foo_t     = foo<double>;
    using foo_graph = transition_checker<foo_t>::transition_graph;
    using edge_t    = transition_checker<foo_t>::edge;

    foo_graph g{
      { { edge_t{1, "Adding 1.1", [](const foo_t& f) -> foo_t { return foo_t{f.z + 1.1}; }, std::weak_ordering::greater }},

        { edge_t{0, "Subtracting 1.1", [](const foo_t& f) -> foo_t { return foo_t{f.z - 1.1}; }, std::weak_ordering::less},
          edge_t{2, "Multiplying by 2", [](const foo_t& f) -> foo_t { return foo_t{f.z * 2}; }, std::weak_ordering::greater} },

        { edge_t{1, "Dividing by 2", [](const foo_t& f) -> foo_t { return foo_t{f.z / 2}; }, std::weak_ordering::less} }
      },
      {[](){ return foo_t{}; }, [](){ return foo_t{1.1}; }, [](){ return foo_t{2.2}; }}
    };

    {
      auto checker{
        [this](std::string_view description, std::function<foo_t()> obtained, std::function<foo_t()> prediction, std::function<foo_t()> parent, std::weak_ordering ordering) {
          check(equality, description, obtained(), prediction());
          check(within_tolerance{0.1}, description, obtained(), prediction());
          check_semantics(description, prediction, parent, ordering);
        }
      };

      transition_checker<foo_t>::check(report_line(""), g, checker);
    }

    {
      auto checker{
        [this](std::string_view description, const foo_t& obtained, const foo_t& prediction) {
          check(equality, description, obtained, prediction);
          check(within_tolerance{0.1}, description, obtained, prediction);
        }
      };

      transition_checker<foo_t>::check(report_line(""), g, checker);
    }
  }

  void move_only_state_transition_false_negative_diagnostics::test_equality_comparable()
  {
    using cmplx = std::complex<double>;
    using foo_t = foo<cmplx>;
    using complex_graph = transition_checker<foo_t>::transition_graph;
    using edge_t = transition_checker<foo_t>::edge;

    complex_graph g{
      { { edge_t{1, "Adding (1.1, -0.7)", [](const foo_t& f) { return foo_t{f.z + cmplx{1.1, -0.7}}; } } },

        { edge_t{0, "Subtracting (1.1, -0.7)", [](const foo_t& f) { return foo_t{f.z - cmplx{1.1, -0.7}}; } },
          edge_t{2, "Multiplying by 2", [](const foo_t& f) { return foo_t{f.z * 2.0}; } }
        },

        { edge_t{1, "Dividing by 2", [](const foo_t& f) { return foo_t{f.z / 2.0}; }}}
      },
      {[]() { return foo_t{}; }, []() { return foo_t{{1.1, -0.7}}; }, []() { return foo_t{{2.2, -1.4}}; }}
    };

    {
      auto checker{
        [this](std::string_view description, std::function<foo_t()> obtained, std::function<foo_t()> prediction, std::function<foo_t()> parent) {
          check(equality, description, obtained(), prediction());
          check(within_tolerance{0.1}, description, obtained(), prediction());
          check_semantics(description, prediction, parent);
        }
      };

      transition_checker<foo_t>::check(report_line(""), g, checker);
    }

    {
      auto checker{
        [this](std::string_view description, const foo_t& obtained, const foo_t& prediction) {
          check(equality, description, obtained, prediction);
          check(within_tolerance{0.1}, description, obtained, prediction);
        }
      };

      transition_checker<foo_t>::check(report_line(""), g, checker);
    }
  }

  [[nodiscard]]
  std::filesystem::path move_only_state_transition_false_positive_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void move_only_state_transition_false_positive_diagnostics::run_tests()
  {
    test_orderable();
    test_equality_comparable();
  }

  void move_only_state_transition_false_positive_diagnostics::test_orderable()
  {
    using foo_t     = foo<double>;
    using foo_graph = transition_checker<foo_t>::transition_graph;
    using edge_t    = transition_checker<foo_t>::edge;

    foo_graph g{
      { { edge_t{1, "Adding 1.1", [](const foo_t& f) -> foo_t { return foo_t{f.z + 1.0}; }, std::weak_ordering::greater }},
        { edge_t{0, "Subtracting 1.1", [](const foo_t& f) -> foo_t { return foo_t{f.z - 1.0}; }, std::weak_ordering::less} },
      },
      {[](){ return foo_t{}; }, [](){ return foo_t{1.1}; }}
    };

    g.set_node_weight(g.cbegin_node_weights()+1, [](){ return foo_t{1.1}; });

    auto checker{
        [this](std::string_view description, const foo_t& obtained, const foo_t& prediction) {
          check(equality, description, obtained, prediction);
        }
    };

    transition_checker<foo_t>::check(report_line("Mistake in transition functions"), g, checker);
  }

  void move_only_state_transition_false_positive_diagnostics::test_equality_comparable()
  {
    using cmplx = std::complex<double>;
    using foo_t = foo<cmplx>;
    using complex_graph = transition_checker<foo_t>::transition_graph;
    using edge_t = transition_checker<foo_t>::edge;

    complex_graph g{
      { { edge_t{1, "Adding (1.1, -0.7)", [](const foo_t& f) { return foo_t{f.z + cmplx{1.0, -0.7}}; } } },

        { edge_t{0, "Subtracting (1.1, -0.7)", [](const foo_t& f) { return foo_t{f.z - cmplx{1.0, -0.7}}; } } }
      },
      {[]() { return foo_t{}; }, []() { return foo_t{{1.1, -0.7}}; }}
    };

    auto checker{
        [this](std::string_view description, const foo_t& obtained, const foo_t& prediction) {
          check(equality, description, obtained, prediction);
        }
    };

    transition_checker<foo_t>::check(report_line("Mistake in transition functions"), g, checker);
  }
}
