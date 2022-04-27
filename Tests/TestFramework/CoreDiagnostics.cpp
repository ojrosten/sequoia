////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "CoreDiagnostics.hpp"
#include "CoreDiagnosticsUtilities.hpp"
#include "ContainerDiagnosticsUtilities.hpp"
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
  std::string_view false_positive_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void false_positive_diagnostics::run_tests()
  {
    basic_tests();
    test_equivalence_checks();
    test_weak_equivalence_checks();
    test_with_best_available_checks();
  }

  void false_positive_diagnostics::basic_tests()
  {
    check(LINE(""), false);
    check(LINE(""), false, tutor{[](bool, bool){
        return std::string{"I pity the fool who confuses the bool."};
      }});
    check(LINE("Advisor ignored"), false, tutor{[](const std::string&, const std::string&){
        return std::string{"I pity the fool who confuses the bool."};}
      });

    check(equality, LINE("Integers which should compare different"), 5, 4);
    check(equality, LINE(""), 6.5, 5.6, tutor{[](double, double){
        return std::string{"Double, double, toil and trouble"};
      }});
  }

  void false_positive_diagnostics::test_equivalence_checks()
  {
    check(equivalence, LINE("Equivalence checking"), only_equivalence_checkable{42}, 41);
    check(equivalence, LINE("Advice for equivalence checking"), only_equivalence_checkable{42}, 41, tutor{bland{}});
  }

  void false_positive_diagnostics::test_weak_equivalence_checks()
  {
    using beast = perfectly_normal_beast<int>;
    check(weak_equivalence, LINE(""), beast{1, 2}, std::initializer_list<int>{1, 1});
    check(weak_equivalence, LINE(""), beast{1, 2}, std::initializer_list<int>{1, 1}, tutor{[](int, int){
        return "Don't mess with the beast.";
      }});

    using prediction = std::initializer_list<std::initializer_list<int>>;
    check(weak_equivalence, LINE(""), std::vector<beast>{{1, 2}, {3, 4}}, prediction{{1, 2}, {3, 5}});

    check(weak_equivalence,
          LINE(""),
          std::vector<beast>{{1, 2}, {3, 4}},
          prediction{{1, 2}, {3, 5}},
          tutor{[](int, int){
            return "Or at least don't mess with a vector of beasts.";
          }});

    check(weak_equivalence,
          LINE("Advice for weak equivalence checking"),
          only_weakly_checkable{42, 3.14},
          std::pair<int, double>{41, 3.13},
          tutor{bland{}});

    check(weak_equivalence,
          LINE("Advice for range weak equivalence, where the containerized form is explicitly specialized"),
          std::vector<only_weakly_checkable>{{42, 3.14}},
          std::vector<std::pair<int, double>>{{41, 3.13}}, tutor{bland{}});

    check(weak_equivalence,
          LINE("Advice for range weak equivalence, where the containerized form is not explicitly specialized"),
          std::list<only_weakly_checkable>{{42, 3.14}},
          std::list<std::pair<int, double>>{{41, 3.13}}, tutor{bland{}});
  }


  void false_positive_diagnostics::test_with_best_available_checks()
  {
    check(with_best_available, LINE("Best available for int"), 1, 2);
    check(with_best_available, LINE("Advice for best available for int"), 1, 2, tutor{[](int, int) { return "int advice"; }});

    check(with_best_available,
          LINE("Best available for only_equivalence_checkable"),
          only_equivalence_checkable{1},
          only_equivalence_checkable{2});
    check(with_best_available,
          LINE("Advice for best available for only_equivalence_checkable"),
          only_equivalence_checkable{1},
          only_equivalence_checkable{2},
          tutor{[](only_equivalence_checkable, only_equivalence_checkable) { return "only_equivalence_checkable advice"; }});

    check(with_best_available,
          LINE("Best available for only_weakly_checkable"),
          only_weakly_checkable{1, -1.4},
          only_weakly_checkable{2, 6.7});
    check(with_best_available,
          LINE("Advice for best available for only_weakly_checkable"),
          only_weakly_checkable{1, -1.4},
          only_weakly_checkable{2, 6.7},
          tutor{[](only_weakly_checkable, only_weakly_checkable) { return "only_weakly_checkable advice"; }});
  }

  [[nodiscard]]
  std::string_view false_negative_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void false_negative_diagnostics::run_tests()
  {
    basic_tests();
    test_equivalence_checks();
    test_weak_equivalence_checks();
    test_with_best_available_checks();
  }

  void false_negative_diagnostics::basic_tests()
  {
    check(LINE(""), true);

    check(equality, LINE(""), 5, 5);
    check(equality, LINE(""), 5.0, 5.0);
  }

  void false_negative_diagnostics::test_equivalence_checks()
  {
    check(equivalence, LINE("Equivalence checking"), only_equivalence_checkable{42}, 42);
    check(equivalence, LINE("Advice for equivalence checking"), only_equivalence_checkable{42}, 42, tutor{bland{}});
  }

  void false_negative_diagnostics::test_weak_equivalence_checks()
  {
    using beast = perfectly_normal_beast<int>;
    check(weak_equivalence, LINE(""), beast{1, 2}, std::initializer_list<int>{1, 2});

    using prediction = std::initializer_list<std::initializer_list<int>>;
    check(weak_equivalence, LINE(""), std::vector<beast>{{1, 2}, {3, 4}}, prediction{{1, 2}, {3, 4}});

    check(weak_equivalence, LINE("Advice for weak equivalence checking"), only_weakly_checkable{42, 3.14}, std::pair<int, double>{42, 3.14}, tutor{bland{}});

    check(weak_equivalence,
          LINE("Advice for range weak equivalence, where the containerized form is explicitly specialized"),
          std::vector<only_weakly_checkable>{{42, 3.14}},
          std::vector<std::pair<int, double>>{{42, 3.14}}, tutor{bland{}});

    check(weak_equivalence,
          LINE("Advice for range weak equivalence, where the containerized form is not explicitly specialized"),
          std::list<only_weakly_checkable>{{42, 3.14}},
          std::list<std::pair<int, double>>{{42, 3.14}}, tutor{bland{}});
  }

  void false_negative_diagnostics::test_with_best_available_checks()
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
}
