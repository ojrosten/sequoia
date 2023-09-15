////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ElementaryFreeDiagnostics.hpp"
#include "ElementaryFreeDiagnosticsUtilities.hpp"

#include "sequoia/TextProcessing/Substitutions.hpp"

#include <complex>
#include <list>
#include <filesystem>
#include <optional>
#include <set>
#include <variant>
#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path elementary_false_positive_free_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void elementary_false_positive_free_diagnostics::run_tests()
  {
    built_in_type_tests();
    test_equality_checks();
    test_equivalence_checks();
    test_weak_equivalence_checks();
    test_with_best_available_checks();
  }

  void elementary_false_positive_free_diagnostics::built_in_type_tests()
  {
    check(report_line("Boolean check"), false);
    check(report_line("Boolean check with advice"), false, tutor{[](bool, bool){
        return "I pity the fool who confuses the bool.";
      }});
    check(report_line("Boolean check with ignored advice"), false, tutor{[](const std::string&, const std::string&){
        return "I pity the fool who confuses the bool.";}
      });

    check(equality, report_line("Integer check"), 5, 4);
    check(equality, report_line("Integer check with advice"), 5, 4, tutor{[](int, int) {
      return "int advice";
      }});

    check(equivalence, report_line("Integer check via fallback"), 5, 4);
    check(equivalence, report_line("Integer check with fallback"), 5, 4, tutor{[](int, int) {
      return "int advice";
      }});

    check(weak_equivalence, report_line("Integer check via two fallbacks"), 5, 4);
    check(weak_equivalence, report_line("Integer check with two fallbacks"), 5, 4, tutor{[](int, int) {
      return "int advice";
      }});

    check(equality, report_line("Double check"), 6.5, 5.6);
    check(equality, report_line("Double check with small difference"), 2.1, static_cast<double>(2.1f));
    check(equality, report_line("Double check with small difference"), -2.1, -static_cast<double>(2.1f));
    check(equality, report_line("Double check with small difference between absolute value"), 2.1, -static_cast<double>(2.1f));
    check(equality, report_line("Double check with tiny difference"), 1.0, 1.0 + std::numeric_limits<double>::epsilon());
    check(equality, report_line("Double check with tiny difference"), -1.0, -1.0 - std::numeric_limits<double>::epsilon());
    check(equality, report_line("Double check with tiny difference between absolute values"), -1.0, 1.0 + std::numeric_limits<double>::epsilon());

    check(equality, report_line("Double check with advice"), 6.5, 5.6, tutor{[](double, double){
        return "Double, double, toil and trouble";
      }});
    check(equality, report_line("Double check with tiny difference and advice"), 1.0, 1.0 + std::numeric_limits<double>::epsilon(), tutor{[](double, double){
        return "Double, double, toil and trouble";
      }});
    check(equality, report_line("Double check with advice for ints ignored"), 6.5, 5.6, tutor{[](int, int) {
        return "int advice";
      }});
    check(equality, report_line("Double check with advice for floats ignored"), 6.5, 5.6, tutor{[](float, float) {
       return "float advice";
     }});

    check(equality, report_line("Float check"), 4.2f, -1.7f);
    check(equality, report_line("Float check with tiny difference"), 1.0f, 1.0f + std::numeric_limits<float>::epsilon());
    check(equality, report_line("Float check with advice"), 4.2f, -1.7f, tutor{[](float, float) {
        return "Float, float, hmmm... doesn't quite work";
      }});
    check(equality, report_line("Float check with advice for ints ignored"), 6.5, 5.6, tutor{[](int, int) {
        return "int advice";
      }});
    check(equality, report_line("Float check with advice for doubles utilized"), 6.5, 5.6, tutor{[](double, double) {
        return "double advice";
      }});

    {
      int x{42}, y{42}, z{43};
      int* p{};
      check(equality, report_line("Equality for null vs. non-null pointer"), p, &x);
      check(equality, report_line("Equality for non-null vs null pointer"), &x, p);
      check(equality, report_line("Equality for different pointers"), &x, &y);
      check(equivalence, report_line("Equivalence for null vs. non-null pointer"), p, &x);
      check(equivalence, report_line("Different pointers pointing to different values"), &x, &z);
    }

    {
      const int x{42}, y{42}, z{43};
      const int* p{};
      check(equality, report_line("Equality for null vs. non-null pointer"), p, &x);
      check(equality, report_line("Equality for non-null vs null pointer"), &x, p);
      check(equality, report_line("Equality for different pointers"), &x, &y);
      check(equivalence, report_line("Equivalence for null vs. non-null pointer"), p, &x);
      check(equivalence, report_line("Different pointers pointing to different values"), &x, &z);
    }

    {
      int x{42}, y{42}, z{43};
      int* const p{};
      check(equality, report_line("Equality for null vs. non-null pointer"), p, &x);
      check(equality, report_line("Equality for non-null vs null pointer"), &x, p);
      check(equality, report_line("Equality for different pointers"), &x, &y);
      check(equivalence, report_line("Equivalence for null vs. non-null pointer"), p, &x);
      check(equivalence, report_line("Different pointers pointing to different values"), &x, &z);
    }

    {
      const int x{42}, y{42}, z{43};
      const int* const p{};
      check(equality, report_line("Equality for null vs. non-null pointer"), p, &x);
      check(equality, report_line("Equality for non-null vs null pointer"), &x, p);
      check(equality, report_line("Equality for different pointers"), &x, &y);
      check(equivalence, report_line("Equivalence for null vs. non-null pointer"), p, &x);
      check(equivalence, report_line("Different pointers pointing to different values"), &x, &z);
    }

    {
      volatile int x{42}, y{42};
      volatile int* p{};
      check(equality, report_line("Equality for null vs. non-null pointer"), p, &x);
      check(equality, report_line("Equality for non-null vs null pointer"), &x, p);
      check(equality, report_line("Equality for different pointers"), &x, &y);
    }

    {
      const volatile int x{42}, y{42};
      const volatile int* p{};
      check(equality, report_line("Equality for null vs. non-null pointer"), p, &x);
      check(equality, report_line("Equality for non-null vs null pointer"), &x, p);
      check(equality, report_line("Equality for different pointers"), &x, &y);
    }

    {
      int x[1]{42}, y[1]{1729};
      check(equality, report_line("Built-in arrays of the same length"), x, y);
    }
  }

  void elementary_false_positive_free_diagnostics::test_equality_checks()
  {
    check(equality, report_line("Equality checking"), perfectly_normal_type{42}, perfectly_normal_type{43});
    check(equality,
          report_line("Equality checking with advice requiring implicit conversion"),
          perfectly_normal_type{42},
          perfectly_normal_type{43},
          tutor{[](perfectly_normal_type, perfectly_normal_type) { return "perfectly_normal_type advice"; }});
    check(equality,
          report_line("Equality checking with advice"),
          perfectly_normal_type{42},
          perfectly_normal_type{43},
          tutor{[](int, int) { return "int advice"; }});

    check(equivalence, report_line("Equality checking via fallback"), perfectly_normal_type{42}, perfectly_normal_type{43});
    check(equivalence,
          report_line("Equality checking via fallback with advice requiring implicit conversion"),
          perfectly_normal_type{42},
          perfectly_normal_type{43},
          tutor{[](perfectly_normal_type, perfectly_normal_type) { return "perfectly_normal_type advice"; }});
    check(equivalence,
          report_line("Equality checking via fallback with advice"),
          perfectly_normal_type{42},
          perfectly_normal_type{43},
          tutor{[](int, int) { return "int advice"; }});

    check(weak_equivalence, report_line("Equality checking via two fallbacks"), perfectly_normal_type{42}, perfectly_normal_type{43});
    check(weak_equivalence,
          report_line("Equality checking via two fallbacks with advice requiring implicit conversion"),
          perfectly_normal_type{42},
          perfectly_normal_type{43},
          tutor{[](perfectly_normal_type, perfectly_normal_type) { return "perfectly_normal_type advice"; }});
    check(weak_equivalence,
          report_line("Equality checking via two fallbacks with advice"),
          perfectly_normal_type{42},
          perfectly_normal_type{43},
          tutor{[](int, int) { return "int advice"; }});
  }

  void elementary_false_positive_free_diagnostics::test_equivalence_checks()
  {
    check(equivalence, report_line("Equivalence checking"), only_equivalence_checkable{42}, 41);
    check(equivalence,
          report_line("Equivalence checking with advice"),
          only_equivalence_checkable{42},
          41,
          tutor{[](double, double) { return "double advice"; }});

    check(equivalence, report_line("Self-equivalence checking"), only_equivalence_checkable{42}, only_equivalence_checkable{41});
    check(equivalence,
          report_line("Self-equivalence checking with advice requiring implicit conversion"),
          only_equivalence_checkable{42},
          only_equivalence_checkable{41},
          tutor{[](only_equivalence_checkable, only_equivalence_checkable) { return "only_equivalence_checkable advice"; }});
    check(equivalence,
          report_line("Self-equivalence checking with advice"),
          only_equivalence_checkable{42},
          only_equivalence_checkable{41},
          tutor{[](double, double) { return "double advice"; }});

    check(weak_equivalence, report_line("Equivalence checking via fallback"), only_equivalence_checkable{42}, 41);
    check(weak_equivalence,
          report_line("Equivalence checking via fallback with advice"),
          only_equivalence_checkable{42},
          41,
          tutor{[](double, double) { return "double advice"; }});

    check(weak_equivalence, report_line("Self-equivalence checking via fallback"), only_equivalence_checkable{42}, only_equivalence_checkable{41});
    check(weak_equivalence,
          report_line("Self-equivalence checking via fallback with advice requiring implicit conversion"),
          only_equivalence_checkable{42},
          only_equivalence_checkable{41},
          tutor{[](only_equivalence_checkable, only_equivalence_checkable) { return "only_equivalence_checkable advice"; }});
    check(weak_equivalence,
          report_line("Self-equivalence checking via fallback with advice"),
          only_equivalence_checkable{42},
          only_equivalence_checkable{41},
          tutor{[](double, double) { return "double advice"; }});
  }

  void elementary_false_positive_free_diagnostics::test_weak_equivalence_checks()
  {
    check(weak_equivalence,
          report_line("Weak equivalence checking"),
          only_weakly_checkable{42, 3.14},
          std::pair<int, double>{41, 3.13});
    check(weak_equivalence,
          report_line("Weak equivalence checking with advice"),
          only_weakly_checkable{42, 3.14},
          std::pair<int, double>{41, 3.13},
          tutor{bland{}});

    check(weak_equivalence, report_line("Self-weak-equivalence checking"), only_weakly_checkable{42, 3.14}, only_weakly_checkable{41, 3.13});
    check(weak_equivalence,
          report_line("Self-weak-equivalence checking with advice ignored because implicit conversions from either int or double to only_weakly_checkable don't exist"),
          only_weakly_checkable{42, 3.14},
          only_weakly_checkable{41, 3.13},
          tutor{[](only_weakly_checkable, only_weakly_checkable) { return "only_weakly_checkable advice"; }});
    check(weak_equivalence,
          report_line("Self-weak-equivalence checking with advice"),
          only_weakly_checkable{42, 3.14},
          only_weakly_checkable{41, 3.13},
          tutor{bland{}});
  }


  void elementary_false_positive_free_diagnostics::test_with_best_available_checks()
  {
    check(with_best_available, report_line("Best available for int"), 1, 2);
    check(with_best_available, report_line("Advice for best available for int"), 1, 2, tutor{[](int, int) { return "int advice"; }});

    check(with_best_available,
          report_line("Best available for only_equivalence_checkable"),
          only_equivalence_checkable{1},
          only_equivalence_checkable{2});
    check(with_best_available,
          report_line("Best available for only_equivalence_checkable with advice requiring implicit conversion"),
          only_equivalence_checkable{1},
          only_equivalence_checkable{2},
          tutor{[](only_equivalence_checkable, only_equivalence_checkable) { return "only_equivalence_checkable advice"; }});

    check(with_best_available,
          report_line("Best available for only_weakly_checkable"),
          only_weakly_checkable{1, -1.4},
          only_weakly_checkable{2, 6.7});
    check(with_best_available,
          report_line("Best available for only_weakly_checkable with advice"),
          only_weakly_checkable{1, -1.4},
          only_weakly_checkable{2, 6.7},
          tutor{bland{}});
  }

  [[nodiscard]]
  std::filesystem::path elementary_false_negative_free_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void elementary_false_negative_free_diagnostics::run_tests()
  {
    built_in_type_tests();
    test_equality_checks();
    test_equivalence_checks();
    test_weak_equivalence_checks();
    test_with_best_available_checks();
  }

  void elementary_false_negative_free_diagnostics::built_in_type_tests()
  {
    check(report_line("Boolean test"), true);

    check(equality, report_line("Integer test"), 5, 5);
    check(equality, report_line("Double test"), 5.0, 5.0);

    {
      int x{42}, y{42};
      int* p{};

      check(equality, report_line("Equality of null pointer with itself"), p, p);
      check(equality, report_line("Equality of non-null pointer with itself"), &x, &x);
      check(equivalence, report_line("Different pointers pointing to the same values"), &x, &y);
    }

    {
      int x[1]{1729}, y[1]{1729};
      check(equality, report_line("Built-in arrays of the same length"), x, y);
    }
  }

  void elementary_false_negative_free_diagnostics::test_equivalence_checks()
  {
    check(equivalence, report_line("Equivalence checking"), only_equivalence_checkable{42}, 42);
    check(equivalence, report_line("Eequivalence checking with advice"), only_equivalence_checkable{42}, 42, tutor{bland{}});
  }

  void elementary_false_negative_free_diagnostics::test_weak_equivalence_checks()
  {
    check(weak_equivalence,
          report_line("Weak equivalence checking"),
          only_weakly_checkable{42, 3.14},
          std::pair<int, double>{42, 3.14});

    check(weak_equivalence,
          report_line("Weak equivalence checking with advice"),
          only_weakly_checkable{42, 3.14},
          std::pair<int, double>{42, 3.14},
          tutor{bland{}});
  }

  void elementary_false_negative_free_diagnostics::test_with_best_available_checks()
  {
    check(with_best_available, report_line("Best available for int"), 1, 1);

    check(with_best_available,
          report_line("Best available for only_weakly_checkable"),
          only_weakly_checkable{2, -1.4},
          only_weakly_checkable{2, -1.4});

    check(with_best_available,
          report_line("Best available for only_weakly_checkable"),
          only_weakly_checkable{1, 6.7},
          only_weakly_checkable{1, 6.7});
  }

  void elementary_false_negative_free_diagnostics::test_equality_checks()
  {
    check(equality, report_line("Equality checking"), perfectly_normal_type{42}, perfectly_normal_type{42});
  }
}
