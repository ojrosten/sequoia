////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "IndividualTestPathsFreeTest.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "sequoia/TestFramework/IndividualTestPaths.hpp"

#include <ranges>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  [[nodiscard]]
  std::filesystem::path individual_test_paths_free_test::source_file()
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

    test_project_folder_deduction();
    test_ancillary_main_cpps();

    check_exception_thrown<std::runtime_error>(
      reporter{"Empty file"},
      []() { return test_summary_path{"", "foo_test", project_paths{}, std::nullopt}; }
    );

    check(equality, "", test_summary_path{"Foo.cpp", "foo_test", project_paths{}, std::nullopt}.file_path().generic_string(), "foo_test.txt"s);
    check(equality, "", test_summary_path{"Foo.cpp", "foo_test", project_paths{}, "xyz"}.file_path().generic_string(), "foo_test_xyz.txt"s);

    {
      commandline_arguments args{{minimal_fake_path().generic_string()}};
      project_paths projPaths{args.size(), args.get(), {}};
      check(
        equality,
        reporter{"Absolute Path"},
        test_summary_path{working_materials() / "Tests" / "Foo.cpp", "foo_test", projPaths, std::nullopt}.file_path(),
        projPaths.output().test_summaries() / "Tests" / "foo_test.txt"
      );

      check(
        equality,
        reporter{"Non-Absolute Path"},
        test_summary_path{fs::path{"Tests/Foo.cpp"}, "foo_test", projPaths, std::nullopt}.file_path(),
        projPaths.output().test_summaries() / "Tests" / "foo_test.txt"
      );
    }
  }

  void individual_test_paths_free_test::test_project_folder_deduction()
  {
    const auto root{working_materials() /= "Deduction"};

    auto make{
      [&root](std::string_view checkout, std::initializer_list<std::string_view> sourceDirs) {
        const auto projectRoot{root / checkout};
        fs::create_directories(projectRoot / "Source");
        for(auto d : sourceDirs) fs::create_directories(projectRoot / "Source" / d);

        return projectRoot;
      }
    };

    {
      const auto projectRoot{make("MyProject", {"myProject"})};
      check(
        equality,
        "The checkout is named after the project",
        source_paths{projectRoot}.project(),
        projectRoot / "Source" / "myProject"
      );
    }

    {
      const auto projectRoot{make("myProject-trunk-wt", {"myProject"})};
      check(
        equality,
        "A worktree: the guess is wrong, and left so for create to refuse on",
        source_paths{projectRoot}.project(),
        projectRoot / "Source" / "myProject-trunk-wt"
      );
    }

    {
      const auto projectRoot{make("myProject-trunk-wt", {"myProject"})};
      check(
        equality,
        "An explicit source_folder settles it",
        source_paths{projectRoot, "myProject"}.project(),
        projectRoot / "Source" / "myProject"
      );
    }

    {
      const auto projectRoot{make("myProject", {"myProject"})};
      check(
        equality,
        "An explicit source_folder, in a checkout named after it",
        source_paths{projectRoot, "myProject"}.project(),
        projectRoot / "Source" / "myProject"
      );
    }
  }

  void individual_test_paths_free_test::test_ancillary_main_cpps()
  {
    commandline_arguments args{{minimal_fake_path().generic_string()}};
    const project_paths projPaths{
      args.size(),
      args.get(),
      {
        .ancillary_main_cpps{
          "TestChamber/TestChamberMain.cpp",
          "TestFrameworkDiagnostics/TestFrameworkDiagnosticsMain.cpp"
        },
        .common_includes{"TestCommon/TestIncludes.hpp"}
      }
    };

    const auto files{
        projPaths.ancillary_main_cpps()
      | std::views::transform([](const main_paths& ancillaryMain) { return ancillaryMain.file(); })
      | std::ranges::to<std::vector>()
    };

    check(
      equality,
      "Each ancillary main is located from the project root",
      files,
      std::vector<fs::path>{fake_project() / "TestChamber/TestChamberMain.cpp",
                            fake_project() / "TestFrameworkDiagnostics/TestFrameworkDiagnosticsMain.cpp"}
    );

    const auto commonIncludes{
        projPaths.ancillary_main_cpps()
      | std::views::transform([](const main_paths& ancillaryMain) { return ancillaryMain.common_includes(); })
      | std::ranges::to<std::vector>()
    };

    check(
      equality,
      "Each ancillary main shares the common includes of the main",
      commonIncludes,
      std::vector<fs::path>(2, fake_project() / "TestCommon/TestIncludes.hpp")
    );
  }
}
