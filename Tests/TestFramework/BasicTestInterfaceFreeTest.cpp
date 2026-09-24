////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "BasicTestInterfaceFreeTest.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "sequoia/TestFramework/FreeTestCore.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    class fake_test : public free_test {
    public:
      using free_test::free_test;
    };

    class fake_test_with_discriminated_summary : public free_test {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::string summary_discriminator(const cmake_cache&) { return "bar"; }
    };

    class fake_test_with_discriminated_exceptions : public free_test {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static std::string output_discriminator(const cmake_cache&) { return "baz"; }
    };
  }

  [[nodiscard]]
  std::filesystem::path basic_test_interface_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  [[nodiscard]]
  std::filesystem::path basic_test_interface_free_test::fake_project() const
  {
    return working_materials() /= "FakeProject";
  }

  [[nodiscard]]
  fs::path basic_test_interface_free_test::minimal_fake_path() const
  {
    return fake_project().append("build/CMade/FakeExe.txt");
  }

  void basic_test_interface_free_test::run_tests()
  {
    commandline_arguments args{{(minimal_fake_path()).generic_string()}};

    std::stringstream outputStream{};
    test_runner runner{args.size(),
                       args.get(),
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestSandbox/TestSandbox.cpp"}, .common_includes{"TestShared/SharedIncludes.hpp"}},
                       outputStream};

    test_file_paths(runner.proj_paths());
    test_materials(runner.proj_paths());
  }

  void basic_test_interface_free_test::test_file_paths(const project_paths& projPaths)
  {
    const auto rebasedSource{rebase_from(source_file(), get_project_paths().project_root())};

    {
      fake_test t{test_name<fake_test>(), source_file(), projPaths, {}, {}, {}, {}};

      check(equality,
            reporter{"Summary File Path"},
            t.summary_file_path().file_path(),
            projPaths.output().test_summaries() / rebasedSource.parent_path() / "fake_test.txt");
      
      check(equality,
            reporter{"Exceptions File Path"},
            t.diagnostics_file_paths().caught_exceptions_file_path(),
            projPaths.output().diagnostics() / rebasedSource.parent_path() / "fake_test_Exceptions.txt");
    }

    {
      fake_test t{test_name<fake_test>(), source_file(), projPaths, {}, {}, {""}, {""}};

      check(equality,
        reporter{"Summary File Path"},
        t.summary_file_path().file_path(),
        projPaths.output().test_summaries() / rebasedSource.parent_path() / "fake_test.txt");

      check(equality,
        reporter{"Exceptions File Path"},
        t.diagnostics_file_paths().caught_exceptions_file_path(),
        projPaths.output().diagnostics() / rebasedSource.parent_path() / "fake_test_Exceptions.txt");
    }

    {
      fake_test_with_discriminated_summary t{test_name<fake_test_with_discriminated_summary>(), source_file(), projPaths, {}, {}, {}, {"bar"}};

      check(equality,
            reporter{"Summary File Path"},
            t.summary_file_path().file_path(),
            projPaths.output().test_summaries() / rebasedSource.parent_path() / "fake_test_with_discriminated_summary_bar.txt");

      check(equality,
            reporter{"Exceptions File Path"},
            t.diagnostics_file_paths().caught_exceptions_file_path(),
            projPaths.output().diagnostics() / rebasedSource.parent_path() / "fake_test_with_discriminated_summary_Exceptions.txt");
    }

    {
      fake_test_with_discriminated_exceptions t{test_name<fake_test_with_discriminated_exceptions>(), source_file(), projPaths, {}, {}, {"baz"}, {}};

      check(equality,
            reporter{"Summary File Path"},
            t.summary_file_path().file_path(),
            projPaths.output().test_summaries() / rebasedSource.parent_path() / "fake_test_with_discriminated_exceptions.txt");

      check(equality,
            reporter{"Exceptions File Path"},
            t.diagnostics_file_paths().caught_exceptions_file_path(),
            projPaths.output().diagnostics() / rebasedSource.parent_path() / "fake_test_with_discriminated_exceptions_Exceptions_baz.txt");
    }
  }

  /** Fake tests in the fake project, whose committed materials are inputs alone, predictions alone,
      auxiliary materials alone, none at all, and inputs beside a stray file. Each is staged as the
      runner stages a test.
   */
  void basic_test_interface_free_test::test_materials(const project_paths& projPaths)
  {
    const auto temporaryRoot{
      [&projPaths](std::string_view sourceStem) {
        return projPaths.output().tests_temporary_data() / "Materials" / sourceStem / "fake_test";
      }
    };

    const auto stagedTest{
      [&projPaths](std::string_view sourceStem) {
        const auto source{projPaths.tests().repo() / "Materials" / std::format("{}.cpp", sourceStem)};
        const individual_materials_paths materials{source, "fake_test", projPaths, std::nullopt};
        stage_materials(materials);
        return std::pair{fake_test{"fake_test", source, projPaths, materials, {}, {}, {}}, materials};
      }
    };

    {
      const auto [test, materials]{stagedTest("WithInputs")};

      check(equality,
            "Working copy of inputs alone",
            test.working_materials(),
            temporaryRoot("WithInputs") / "WorkingCopy");

      check("Committed input staged", fs::exists(temporaryRoot("WithInputs") / "WorkingCopy" / "input.txt"));
      check(equality, "Scratchpad beside the materials", test.scratchpad_materials(), temporaryRoot("WithInputs"));

      check_exception_thrown<std::runtime_error>(
        "No predictions",
        [&test]() { return test.predictive_materials(); });

      check_exception_thrown<std::runtime_error>(
        "No auxiliary materials",
        [&test]() { return test.auxiliary_materials(); });
    }

    {
      const auto [test, materials]{stagedTest("WithPredictions")};

      check(equality,
            "Working copy of predictions alone",
            test.working_materials(),
            temporaryRoot("WithPredictions") / "WorkingCopy");

      check("Working copy made empty", fs::is_empty(test.working_materials()));
      check(equality, "Predictions", test.predictive_materials(), materials.original_materials_root() / "Prediction");
      check("Predictions not staged", !fs::exists(temporaryRoot("WithPredictions") / "Prediction"));
    }

    {
      const auto [test, materials]{stagedTest("WithAuxiliary")};

      const auto staged{temporaryRoot("WithAuxiliary") / "Auxiliary"};
      check(equality, "Auxiliary materials alone", test.auxiliary_materials(), staged);
      check("Committed auxiliary material staged", fs::exists(staged / "auxiliary.txt"));
    }

    {
      const auto [test, materials]{stagedTest("WithNone")};

      check_exception_thrown<std::runtime_error>("No materials", [&test]() { return test.working_materials(); });
      check("Scratchpad staged empty", fs::is_empty(temporaryRoot("WithNone")));

      // Where the file lands is checked without asking the scratchpad where it is
      write_to_file(test.scratchpad_materials() / "scratch.txt", "", std::ios_base::out);
      check("Scratch file beneath the temporary data", fs::exists(temporaryRoot("WithNone") / "scratch.txt"));
      check("Scratch file not in the current directory", !fs::exists(fs::current_path() / "scratch.txt"));

      stage_materials(materials);
      check("Scratchpad emptied by staging again", fs::is_empty(temporaryRoot("WithNone")));
    }

    {
      const auto [test, materials]{stagedTest("WithInputs")};
      write_to_file(test.working_materials() / "input.txt", "Changed", std::ios_base::out);
      write_to_file(test.working_materials() / "extra.txt", "", std::ios_base::out);

      stage_materials(materials);
      check(equivalence,
            "Working copy restored by staging again",
            temporaryRoot("WithInputs") / "WorkingCopy",
            materials.original_working());
    }

    check_exception_thrown<std::runtime_error>(
      "Stray committed materials",
      [&stagedTest]() { return stagedTest("WithStray"); });

    check_exception_thrown<std::logic_error>(
      "Staging the materials of no test",
      []() { stage_materials(individual_materials_paths{}); });
  }
}
