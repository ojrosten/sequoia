////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "RegularStateTransitionDiagnostics.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

#include <complex>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view regular_state_transition_false_negative_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void regular_state_transition_false_negative_diagnostics::run_tests()
  {
    test_orderable();
    test_equality_comparable();
  }

  void regular_state_transition_false_negative_diagnostics::test_orderable()
  {
    using double_graph = transition_checker<double>::transition_graph;
    using edge_t       = transition_checker<double>::edge;

    double_graph g{
      { { edge_t{1, "Adding 1.1", [](double f)  { return f + 1.1; }, std::weak_ordering::greater} },

        { edge_t{0, "Subtracting 1.1", [](double f)  { return f - 1.1; }, std::weak_ordering::less},
          edge_t{2, "Multiplying by 2", [](double f)  { return f * 2; }, std::weak_ordering::greater}
        },

        { edge_t{1, "Dividing by 2", [](double f)  { return f / 2; }, std::weak_ordering::less} }
      },
      {0.0, 1.1, 2.2}
    };
    
    {
      auto checker{
        [this](std::string_view description, double obtained, double prediction, double parent, std::weak_ordering ordering) {
          check(equality, description, obtained, prediction);
          check(within_tolerance{0.1}, description, obtained, prediction);
          check_semantics(description, prediction, parent, ordering);
        }
      };

      transition_checker<double>::check(LINE(""), g, checker);
    }

    {
      auto checker{
        [this](std::string_view description, std::function<double()> obtained, std::function<double()> prediction, std::function<double()> parent, std::weak_ordering ordering) {
          check(equality, description, obtained(), prediction());
          check(within_tolerance{0.1}, description, obtained(), prediction());
          check_semantics(description, prediction(), parent(), ordering);
        }
      };

      transition_checker<double>::check(LINE(""), g, checker);
    }

    {
      auto checker{
        [this](std::string_view description, double obtained, double prediction) {
          check(equality, description, obtained, prediction);
          check(within_tolerance{0.1}, description, obtained, prediction);
        }
      };

      transition_checker<double>::check(LINE(""), g, checker);
    }
  }

  void regular_state_transition_false_negative_diagnostics::test_equality_comparable()
  {
    using cmplx         = std::complex<double>;
    using complex_graph = transition_checker<cmplx>::transition_graph;
    using edge_t        = transition_checker<cmplx>::edge;

    complex_graph g{
      { { edge_t{1, "Adding (1.1, -0.7)", [](cmplx f) { return f + cmplx{1.1, -0.7}; } } },

        { edge_t{0, "Subtracting (1.1, -0.7)", [](cmplx f) { return f - cmplx{1.1, -0.7}; } },
          edge_t{2, "Multiplying by 2", [](cmplx f) { return f * 2.0; } }
        },

        { edge_t{1, "Dividing by 2", [](cmplx f) { return f / 2.0; } } }
      },
      {cmplx{0.0, 0.0}, cmplx{1.1, -0.7}, cmplx{2.2, -1.4}}
    };

    {
      auto checker{
        [this](std::string_view description, cmplx obtained, cmplx prediction, cmplx parent) {
          check(equality, description, obtained, prediction);
          check(within_tolerance{0.1}, description, obtained, prediction);
          check_semantics(description, prediction, parent);
        }
      };

      transition_checker<cmplx>::check(LINE(""), g, checker);
    }

    {
      auto checker{
        [this](std::string_view description, std::function<cmplx()> obtained, std::function<cmplx()> prediction, std::function<cmplx()> parent) {
          check(equality, description, obtained(), prediction());
          check(within_tolerance{0.1}, description, obtained(), prediction());
          check_semantics(description, prediction(), parent());
        }
      };

      transition_checker<cmplx>::check(LINE(""), g, checker);
    }

    {
      auto checker{
        [this](std::string_view description, cmplx obtained, cmplx prediction) {
          check(equality, description, obtained, prediction);
          check(within_tolerance{0.1}, description, obtained, prediction);
        }
      };

      transition_checker<cmplx>::check(LINE(""), g, checker);
    }
  }

  [[nodiscard]]
  std::string_view regular_state_transition_false_positive_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void regular_state_transition_false_positive_diagnostics::run_tests()
  {
    test_orderable();
    test_equality_comparable();
  }

  void regular_state_transition_false_positive_diagnostics::test_orderable()
  {
    using double_graph = transition_checker<double>::transition_graph;
    using edge_t       = transition_checker<double>::edge;

    double_graph g{
      { { edge_t{1, "Adding 1.1", [](double f) { return f + 1.0; }, std::weak_ordering::greater } },
        { edge_t{0, "Subtracting 1.1", [](double f) { return f - 1.0; }, std::weak_ordering::less} }
      },
      {0.0, 1.1}
    };

    auto checker{
        [this](std::string_view description, double obtained, double prediction) {
          check(equality, description, obtained, prediction);
        }
    };

    transition_checker<double>::check(LINE("Mistake in transition functions"), g, checker);
  }

  void regular_state_transition_false_positive_diagnostics::test_equality_comparable()
  {
    using cmplx         = std::complex<double>;
    using complex_graph = transition_checker<cmplx>::transition_graph;
    using edge_t        = transition_checker<cmplx>::edge;

    complex_graph g{
      { { edge_t{1, "Adding (1.1, -0.7)", [](cmplx f) { return f + cmplx{1.0, -0.7}; } } },

        { edge_t{0, "Subtracting (1.1, -0.7)", [](cmplx f) { return f - cmplx{1.0, -0.7}; } } }
      },
      {{0.0, 0.0}, cmplx{1.1, -0.7}}
    };

    auto checker{
        [this](std::string_view description, cmplx obtained, cmplx prediction) {
          check(equality, description, obtained, prediction);
        }
    };

    transition_checker<cmplx>::check(LINE("Mistake in transition functions"), g, checker);
  }
}
