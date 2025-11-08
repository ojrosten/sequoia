////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "IndividualTestPathsFreeTest.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "sequoia/TestFramework/IndividualTestPaths.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  [[nodiscard]]
  std::filesystem::path individual_test_paths_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  [[nodiscard]]
  std::filesystem::path individual_test_paths_free_test::fake_project() const
  {
    return working_materials() /= "FakeProject";
  }

  [[nodiscard]]
  fs::path individual_test_paths_free_test::minimal_fake_path() const
  {
    return fake_project().append("build/CMade/FakeExe.txt");
  }

  void individual_test_paths_free_test::run_tests()
  {
    using namespace std::string_literals;

    check_exception_thrown<std::runtime_error>(
      reporter{"Empty file"},
      []() { return test_summary_path{"", project_paths{}, std::nullopt}; }
    );

    check(equality, "", test_summary_path{"Foo.cpp", project_paths{}, std::nullopt}.file_path().generic_string(), "Foo.txt"s);
    check(equality, "", test_summary_path{"Foo.cpp", project_paths{}, "xyz"}.file_path().generic_string(), "Foo_xyz.txt"s);

    {
      commandline_arguments args{{minimal_fake_path().generic_string()}};
      project_paths projPaths{args.size(), args.get(), {}};
      check(equality,
        reporter{"Absolute Path"},
        test_summary_path{working_materials() / "Tests" / "Foo.cpp", projPaths, std::nullopt}.file_path(),
        projPaths.output().test_summaries() / "Tests" / "Foo.txt");

      check(equality,
        reporter{"Non-Absolute Path"},
        test_summary_path{fs::path{"Tests/Foo.cpp"}, projPaths, std::nullopt}.file_path(),
        projPaths.output().test_summaries() / "Tests" / "Foo.txt");
    }

  }
}
