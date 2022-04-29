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
  std::string_view elementary_false_positive_free_diagnostics::source_file() const noexcept
  {
    return __FILE__;
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
    check(LINE("Boolean check"), false);
    check(LINE("Boolean check with advice"), false, tutor{[](bool, bool){
        return "I pity the fool who confuses the bool.";
      }});
    check(LINE("Boolean check with ignored advice"), false, tutor{[](const std::string&, const std::string&){
        return "I pity the fool who confuses the bool.";}
      });

    check(equality, LINE("Integer check"), 5, 4);
    check(equality, LINE("Integer check with advice"), 5, 4, tutor{[](int, int) {
      return "int advice";
      }});

    check(equivalence, LINE("Integer check via fallback"), 5, 4);
    check(equivalence, LINE("Integer check with fallback"), 5, 4, tutor{[](int, int) {
      return "int advice";
      }});

    check(weak_equivalence, LINE("Integer check via two fallbacks"), 5, 4);
    check(weak_equivalence, LINE("Integer check with two fallbacks"), 5, 4, tutor{[](int, int) {
      return "int advice";
      }});

    check(equality, LINE("Double check"), 6.5, 5.6);
    check(equality, LINE("Double check with advice"), 6.5, 5.6, tutor{[](double, double){
        return "Double, double, toil and trouble";
      }});
    check(equality, LINE("Double check with ignored advice"), 6.5, 5.6, tutor{[](int, int) {
        return "int adivce";
      }});
  }

  void elementary_false_positive_free_diagnostics::test_equality_checks()
  {
    check(equality, LINE("Equality checking"), perfectly_normal_type{42}, perfectly_normal_type{43});
    check(equality,
          LINE("Equality checking with advice requiring implicit conversion"),
          perfectly_normal_type{42},
          perfectly_normal_type{43},
          tutor{[](perfectly_normal_type, perfectly_normal_type) { return "perfectly_normal_type advice"; }});
    check(equality,
          LINE("Equality checking with advice"),
          perfectly_normal_type{42},
          perfectly_normal_type{43},
          tutor{[](int, int) { return "int advice"; }});

    check(equivalence, LINE("Equality checking via fallback"), perfectly_normal_type{42}, perfectly_normal_type{43});
    check(equivalence,
          LINE("Equality checking via fallback with advice requiring implicit conversion"),
          perfectly_normal_type{42},
          perfectly_normal_type{43},
          tutor{[](perfectly_normal_type, perfectly_normal_type) { return "perfectly_normal_type advice"; }});
    check(equivalence,
          LINE("Equality checking via fallback with advice"),
          perfectly_normal_type{42},
          perfectly_normal_type{43},
          tutor{[](int, int) { return "int advice"; }});

    check(weak_equivalence, LINE("Equality checking via two fallbacks"), perfectly_normal_type{42}, perfectly_normal_type{43});
    check(weak_equivalence,
          LINE("Equality checking via two fallbacks with advice requiring implicit conversion"),
          perfectly_normal_type{42},
          perfectly_normal_type{43},
          tutor{[](perfectly_normal_type, perfectly_normal_type) { return "perfectly_normal_type advice"; }});
    check(weak_equivalence,
          LINE("Equality checking via two fallbacks with advice"),
          perfectly_normal_type{42},
          perfectly_normal_type{43},
          tutor{[](int, int) { return "int advice"; }});
  }

  void elementary_false_positive_free_diagnostics::test_equivalence_checks()
  {
    check(equivalence, LINE("Equivalence checking"), only_equivalence_checkable{42}, 41);
    check(equivalence,
          LINE("Equivalence checking with advice"),
          only_equivalence_checkable{42},
          41,
          tutor{[](double, double) { return "double advice"; }});

    check(equivalence, LINE("Self-equivalence checking"), only_equivalence_checkable{42}, only_equivalence_checkable{41});
    check(equivalence,
          LINE("Self-equivalence checking with advice requiring implicit conversion"),
          only_equivalence_checkable{42},
          only_equivalence_checkable{41},
          tutor{[](only_equivalence_checkable, only_equivalence_checkable) { return "only_equivalence_checkable advice"; }});
    check(equivalence,
          LINE("Self-equivalence checking with advice"),
          only_equivalence_checkable{42},
          only_equivalence_checkable{41},
          tutor{[](double, double) { return "double advice"; }});

    check(weak_equivalence, LINE("Equivalence checking via fallback"), only_equivalence_checkable{42}, 41);
    check(weak_equivalence,
          LINE("Equivalence checking via fallback with advice"),
          only_equivalence_checkable{42},
          41,
          tutor{[](double, double) { return "double advice"; }});

    check(weak_equivalence, LINE("Self-equivalence checking via fallback"), only_equivalence_checkable{42}, only_equivalence_checkable{41});
    check(weak_equivalence,
          LINE("Self-equivalence checking via fallback with advice requiring implicit conversion"),
          only_equivalence_checkable{42},
          only_equivalence_checkable{41},
          tutor{[](only_equivalence_checkable, only_equivalence_checkable) { return "only_equivalence_checkable advice"; }});
    check(weak_equivalence,
          LINE("Self-equivalence checking via fallback with advice"),
          only_equivalence_checkable{42},
          only_equivalence_checkable{41},
          tutor{[](double, double) { return "double advice"; }});
  }

  void elementary_false_positive_free_diagnostics::test_weak_equivalence_checks()
  {
    check(weak_equivalence,
          LINE("Weak equivalence checking"),
          only_weakly_checkable{42, 3.14},
          std::pair<int, double>{41, 3.13});
    check(weak_equivalence,
          LINE("Weak equivalence checking with advice"),
          only_weakly_checkable{42, 3.14},
          std::pair<int, double>{41, 3.13},
          tutor{bland{}});

    check(weak_equivalence, LINE("Self-weak-equivalence checking"), only_weakly_checkable{42, 3.14}, only_weakly_checkable{41, 3.13});
    check(weak_equivalence,
          LINE("Self-weak-equivalence checking with advice requiring implicit conversion"),
          only_weakly_checkable{42, 3.14},
          only_weakly_checkable{41, 3.13},
          tutor{[](only_weakly_checkable, only_weakly_checkable) { return "only_weakly_checkable advice"; }});
    check(weak_equivalence,
          LINE("Self-weak-equivalence checking with advice"),
          only_weakly_checkable{42, 3.14},
          only_weakly_checkable{41, 3.13},
          tutor{bland{}});
  }


  void elementary_false_positive_free_diagnostics::test_with_best_available_checks()
  {
    check(with_best_available, LINE("Best available for int"), 1, 2);
    check(with_best_available, LINE("Advice for best available for int"), 1, 2, tutor{[](int, int) { return "int advice"; }});

    check(with_best_available,
          LINE("Best available for only_equivalence_checkable"),
          only_equivalence_checkable{1},
          only_equivalence_checkable{2});
    check(with_best_available,
          LINE("Best available for only_equivalence_checkable with advice requiring implicit conversion"),
          only_equivalence_checkable{1},
          only_equivalence_checkable{2},
          tutor{[](only_equivalence_checkable, only_equivalence_checkable) { return "only_equivalence_checkable advice"; }});

    check(with_best_available,
          LINE("Best available for only_weakly_checkable"),
          only_weakly_checkable{1, -1.4},
          only_weakly_checkable{2, 6.7});
    check(with_best_available,
          LINE("Best available for only_weakly_checkable with advice requiring implicit conversion"),
          only_weakly_checkable{1, -1.4},
          only_weakly_checkable{2, 6.7},
          tutor{[](only_weakly_checkable, only_weakly_checkable) { return "only_weakly_checkable advice"; }});
  }

  [[nodiscard]]
  std::string_view elementary_false_negative_free_diagnostics::source_file() const noexcept
  {
    return __FILE__;
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
    check(LINE("Boolean test"), true);

    check(equality, LINE("Integer test"), 5, 5);
    check(equality, LINE("Double test"), 5.0, 5.0);
  }

  void elementary_false_negative_free_diagnostics::test_equivalence_checks()
  {
    check(equivalence, LINE("Equivalence checking"), only_equivalence_checkable{42}, 42);
    check(equivalence, LINE("Eequivalence checking with advice"), only_equivalence_checkable{42}, 42, tutor{bland{}});
  }

  void elementary_false_negative_free_diagnostics::test_weak_equivalence_checks()
  {
    check(weak_equivalence,
          LINE("Weak equivalence checking"),
          only_weakly_checkable{42, 3.14},
          std::pair<int, double>{42, 3.14});

    check(weak_equivalence,
          LINE("Weak equivalence checking with advice"),
          only_weakly_checkable{42, 3.14},
          std::pair<int, double>{42, 3.14},
          tutor{bland{}});
  }

  void elementary_false_negative_free_diagnostics::test_with_best_available_checks()
  {
    check(with_best_available, LINE("Best available for int"), 1, 1);

    check(with_best_available,
          LINE("Best available for only_weakly_checkable"),
          only_weakly_checkable{2, -1.4},
          only_weakly_checkable{2, -1.4});

    check(with_best_available,
          LINE("Best available for only_weakly_checkable"),
          only_weakly_checkable{1, 6.7},
          only_weakly_checkable{1, 6.7});
  }

  void elementary_false_negative_free_diagnostics::test_equality_checks()
  {
    check(equality, LINE("Equality checking"), perfectly_normal_type{42}, perfectly_normal_type{42});
  }
}
