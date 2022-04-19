////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ContainerFreeDiagnostics.hpp"
#include "CoreDiagnosticsUtilities.hpp"

#include "sequoia/TestFramework/ConcreteTypeCheckers.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view container_false_positive_free_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void container_false_positive_free_diagnostics::run_tests()
  {
    test_homogeneous();
    test_heterogeneous();
  }

  void container_false_positive_free_diagnostics::test_homogeneous()
  {
    check(equality, LINE("Empty vector check which should fail"), std::vector<double>{}, std::vector<double>{1});
    check(equality, LINE("One element vector check which should fail due to wrong value"), std::vector<double>{1}, std::vector<double>{2});
    check(equality,
         LINE("Advice for one element vector check which should fail due to wrong value"),
         std::vector<double>{1},
         std::vector<double>{2},
         tutor{[](double, double) { return "Vector element advice"; }});
    check(equality, LINE("One element vector check which should fail due to differing sizes"), std::vector<double>{1}, std::vector<double>{1,2});
    check(equality, LINE("Multi-element vector comparison which should fail due to last element"), std::vector<double>{1,5}, std::vector<double>{1,4});
    check(equality, LINE("Multi-element vector comparison which should fail due to first element"), std::vector<double>{1,5}, std::vector<double>{0,5});
    check(equality, LINE("Multi-element vector comparison which should fail due to middle element"), std::vector<double>{1,3.2,5}, std::vector<double>{1,3.3,5});
    check(equality, LINE("Multi-element vector comparison which should fail due to different sizes"), std::vector<double>{1,5,3.2}, std::vector<double>{5,3.2});

    std::vector<float> refs{-4.3f, 2.8f, 6.2f, 7.3f}, ans{1.1f, -4.3f, 2.8f, 6.2f, 8.4f, 7.3f};

    check(equality, LINE("Iterators demarcate differing numbers of elements"), refs.cbegin(), refs.cend(), ans.cbegin(), ans.cend());
    check(equality, LINE("Iterators demarcate differing elements"), refs.cbegin(), refs.cend(), ans.cbegin(), ans.cbegin() + 4);
  }

  void container_false_positive_free_diagnostics::test_heterogeneous()
  {
    check(equality, LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{5, -7.8});
    check(equality, LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{-5, 7.8});
    check(equality, LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{-5, 6.8}, tutor{bland{}});
    check(with_best_available, LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{5, -7.8});
    check(equivalence, LINE(""), std::pair<const int&, double>{5, 7.8}, std::pair<int, const double&>{-5, 6.8});
    check(weak_equivalence, LINE(""), std::pair<const int&, double>{5, 7.8}, std::pair<int, const double&>{-5, 6.8});

    check(equality, LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{0, 3.4, -9.2f});
    check(equality, LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 0.0, -9.2f}, tutor{bland{}});
    check(equality, LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 3.4, -0.0f});
    check(equivalence, LINE(""), std::tuple<const int&, double>{5, 7.8}, std::tuple<int, const double&>{-5, 6.8});
  }

  [[nodiscard]]
  std::string_view container_false_negative_free_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void container_false_negative_free_diagnostics::run_tests()
  {
    test_homogeneous();
    test_heterogeneous();
  }

  void container_false_negative_free_diagnostics::test_homogeneous()
  {
    check(equality, LINE("Empty vector check which should pass"), std::vector<double>{}, std::vector<double>{});
    check(equality, LINE("One element vector check which should pass"), std::vector<double>{2}, std::vector<double>{2});
    check(equality, LINE("Multi-element vector comparison which should pass"), std::vector<double>{1, 5}, std::vector<double>{1, 5});

    std::vector<float> refs{-4.3f, 2.8f, 6.2f, 7.3f}, ans{1.1f, -4.3f, 2.8f, 6.2f, 8.4f, 7.3f};

    check(equality, LINE("Iterators demarcate identical elements"), refs.cbegin(), refs.cbegin() + 3, ans.cbegin() + 1, ans.cbegin() + 4);
  }

  void container_false_negative_free_diagnostics::test_heterogeneous()
  {
    check(equality, LINE(""), std::pair<int, double>{5, 7.8}, std::pair<int, double>{5, 7.8});

    check(equality, LINE(""), std::tuple<int, double, float>{4, 3.4, -9.2f}, std::tuple<int, double, float>{4, 3.4, -9.2f});
  }
}
