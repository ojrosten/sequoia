////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DependencyAnalyzerFreeTest.hpp"
#include "sequoia/TestFramework/DependencyAnalyzer.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view dependency_analyzer_free_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void dependency_analyzer_free_test::run_tests()
  {
    const auto fake{working_materials() / "FakeProject"};
    const auto sourceRepo{fake / "Source"}, testsRepo{fake / "Tests"};
    // testing materials

    const auto testsToRun{tests_to_run(sourceRepo, testsRepo, std::chrono::file_clock::now())};
  }
}
