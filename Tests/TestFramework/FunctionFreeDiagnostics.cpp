////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "FunctionFreeDiagnostics.hpp"
#include "sequoia/TestFramework/ConcreteTypeCheckers.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view function_false_positive_free_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void function_false_positive_free_diagnostics::run_tests()
  {
    {
      using function = std::function<void()>;
      check(weak_equivalence,
        LINE("Obtained bound but prediction not"),
        function{[]() {}},
        function{});

      check(weak_equivalence,
        LINE("Prediction bound but obtained not"),
        function{},
        function{[]() {}});
    }

    {
      using function = std::function<int()>;
      check(weak_equivalence,
        LINE("Obtained bound but prediction not"),
        function{[]() { return 42; }},
        function{});

      check(weak_equivalence,
        LINE("Prediction bound but obtained not"),
        function{},
        function{[]() { return 42; }});
    }

    {
      using function = std::function<void(int)>;
      check(weak_equivalence,
        LINE("Obtained bound but prediction not"),
        function{[](int) {}},
        function{});

      check(weak_equivalence,
        LINE("Prediction bound but obtained not"),
        function{},
        function{[](int) {}});
    }
  }

  [[nodiscard]]
  std::string_view function_false_negative_free_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void function_false_negative_free_diagnostics::run_tests()
  {
    {
      using function = std::function<void()>;
      check(weak_equivalence, LINE("Both bound"), function{[]() {}}, function{[]() {}});
      check(weak_equivalence, LINE("Neither bound"), function{}, function{});
    }

    {
      using function = std::function<int()>;
      check(weak_equivalence, LINE("Both bound"), function{[]() { return 42; }}, function{[]() { return 42; }});
      check(weak_equivalence, LINE("Neither bound"), function{}, function{});
    }

    {
      using function = std::function<void(int)>;
      check(weak_equivalence, LINE("Both bound"), function{[](int) {}}, function{[](int) {}});
      check(weak_equivalence, LINE("Neither bound"), function{}, function{});
    }
  }
}
