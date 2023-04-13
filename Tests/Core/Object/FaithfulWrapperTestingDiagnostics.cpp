////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "FaithfulWrapperTestingDiagnostics.hpp"

#include <vector>

namespace sequoia::testing
{
  using namespace object;

  [[nodiscard]]
  std::filesystem::path faithful_wrapper_false_positive_test::source_file() const noexcept
  {
    return std::source_location::current().file_name();
  }

  void faithful_wrapper_false_positive_test::run_tests()
  {
    test_basic_type();
    test_container_type();
    test_aggregate_type();
  }

  void faithful_wrapper_false_positive_test::test_basic_type()
  {
    faithful_wrapper<int> w{1}, v{};
    check(equality, report_line(""), w, v);
  }

  void faithful_wrapper_false_positive_test::test_container_type()
  {
    faithful_wrapper<std::vector<int>> w{}, v{1};
    check(equality, report_line(""), w, v);

    w.mutate([](auto& vec) { vec.push_back(2); });
    check(equality, report_line(""), w, v);

    faithful_wrapper<std::vector<int>> u{std::vector<int>(1)};
    check(equality, report_line(""), u, v);
  }

  void faithful_wrapper_false_positive_test::test_aggregate_type()
  {
    faithful_wrapper<data> w{}, v{1, 2.0};
    check(equality, report_line(""), w, v);
  }
}
