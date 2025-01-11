////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "IndividualTestPathsFreeTest.hpp"
#include "sequoia/TestFramework/IndividualTestPaths.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path individual_test_paths_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void individual_test_paths_free_test::run_tests()
  {
    check_exception_thrown<std::runtime_error>(
      reporter{"Empty file"},
      []() { test_summary_path{"", project_paths{}, std::nullopt}; }
    );
  }
}
