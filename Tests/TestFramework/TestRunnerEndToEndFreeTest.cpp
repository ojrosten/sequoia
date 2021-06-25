////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerEndToEndFreeTest.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"

#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

namespace sequoia::testing
{
  namespace
  {
    [[nodiscard]]
    std::string run_cmd()
    {
      if constexpr (with_msvc_v)
      {
#ifdef CMAKE_INTDIR
        return std::string{CMAKE_INTDIR}.append("\\TestMain.exe");
#else
        throw std::logic_error{"Unable to find preprocessor definition for CMAKE_INTDIR"};
#endif
      }
      else
      {
        return "./TestMain";
      }
    }

    [[nodiscard]]
    std::string create_cmd()
    {
      return run_cmd().append(" create free_test Utilities.hpp"
                              " create free_test \"Utilities/UsefulThings.hpp\" gen-source utils"
                              " create free_test \"Source/generatedProject/Stuff/Bar.hpp\""
                              " create regular_test \"other::functional::maybe<class T>\" \"std::optional<T>\" gen-source Maybe"
                              " create regular_test \"stuff::oldschool\" double --class-header \"NoTemplate.hpp\""
                              " create regular \"maths::probability\" double gen-source Maths"
                              " create move_only_test \"bar::baz::foo<maths::floating_point T>\" T"
                              " create move_only \"stuff::unique_thing\" double gen-source Utilities/Thing"
                              " create regular_allocation_test container"
                              " create move_only_allocation_test house"
                              " create performance_test Container.hpp");
    }

    struct cmd_builder
    {
      std::filesystem::path mainDir, buildDir;

      [[nodiscard]]
      shell_command create_cmake_build_run(const std::filesystem::path& output) const
      {
        return    cd_cmd(buildDir)
               && shell_command{create_cmd(), output / "CreationOutput.txt"}
               && build_cmd(buildDir, "BuildOutput2.txt")
               && shell_command{run_cmd(), output / "TestRunOutput.txt"}
               && shell_command{run_cmd().append(" select ../../../Tests/HouseAllocationTest.cpp")
                                           .append(" select Maybe/MaybeTest.cpp")
                                           .append(" select FooTest.cpp"),
                                  output / "SpecifiedSourceOutput.txt"}
               && shell_command{run_cmd().append(" select Plurgh.cpp test Absent select Foo test FooTest.cpp"), output / "FailedSpecifiedSourceOutput.txt"}
               && shell_command{run_cmd().append(" test Foo"), output / "SpecifiedFamilyOutput.txt"}
               && shell_command{run_cmd().append(" -v"), output / "VerboseOutput.txt"}
               && shell_command{run_cmd().append(" --help"), output / "HelpOutput.txt"};
      }

      [[nodiscard]]
      shell_command rebuild_run(const std::filesystem::path& output) const
      {
        return     cd_cmd(buildDir)
                && cmake_and_build("CMakeOutput3.txt", "BuildOutput3.txt")
                && shell_command{run_cmd(), output / "TestRunOutput.txt"};
      }

      [[nodiscard]]
      shell_command materials_update(const std::filesystem::path& output) const
      {
        return cd_cmd(buildDir) && shell_command(run_cmd().append(" u"), output / "TestRunOutput.txt");
      }

      [[nodiscard]]
      shell_command dump(const std::filesystem::path& output) const
      {
        return cd_cmd(buildDir) && shell_command(run_cmd().append(" --dump"), output / "TestRunOutput.txt");
      }

      [[nodiscard]]
      shell_command recovery(const std::filesystem::path& output) const
      {
        return cd_cmd(buildDir) && shell_command(run_cmd().append(" --recovery"), output / "TestRunOutput.txt");
      }
    private:
      [[nodiscard]]
      shell_command cmake_and_build(std::string_view cmakeOut, std::string_view buildOut) const
      {
        return cd_cmd(mainDir) && cmake_cmd(buildDir, cmakeOut) && build_cmd(buildDir, buildOut);
      }
    };
  }

  [[nodiscard]]
  std::string_view test_runner_end_to_end_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_runner_end_to_end_test::run_tests()
  {
    test_project_creation();
  }

  void test_runner_end_to_end_test::test_project_creation()
  {
    namespace fs = std::filesystem;

    const auto seqRoot{test_repository().parent_path()};
    const auto testMain{seqRoot/"TestAll/TestMain.cpp"};
    const auto includeTarget{seqRoot/"TestCommon/TestIncludes.hpp"};

    const project_paths paths{seqRoot, testMain, includeTarget};

    auto generated{
      [&mat{working_materials()}]() {
        return mat.parent_path() / "GeneratedProject";
      }
    };

    commandline_arguments args{"",
                               "init",
                               "Oliver Jacob Rosten",
                               generated().string(),
                               "\t",
                               "--cmake-output", "CMakeOutput.txt",
                               "--build-output", "BuildOutput.txt"};

    std::stringstream outputStream{};
    test_runner tr{args.size(), args.get(), "Oliver J. Rosten", paths, "  ", outputStream};

    tr.execute();

    fs::create_directory(working_materials() / "InitOutput");
    if(std::ofstream file{working_materials() / "InitOutput" / "io.txt"})
    {
      file << outputStream.rdbuf();
    }

    check_equivalence(LINE(""), working_materials() / "InitOutput", predictive_materials() / "InitOutput");

    check(LINE("Command processor existance"), std::system(nullptr) > 0);

    //////////// Correct platform dependence!
    const cmd_builder b{generated() / "TestAll", generated() / "build" / "CMade" / "win" / "TestAll"};

    // Run the tests
    fs::create_directory(working_materials() / "EmptyRunOutput");
    invoke(cd_cmd(b.buildDir) && shell_command{run_cmd(), working_materials() / "EmptyRunOutput" / "EmptyRunOutput.txt" });

    check(LINE("First CMake output existance"), fs::exists(b.mainDir / "CMakeOutput.txt"));
    check(LINE("First build output existance"), fs::exists(b.buildDir / "BuildOutput.txt"));
    check_equivalence(LINE("Test Runner Output"), working_materials() / "EmptyRunOutput", predictive_materials() / "EmptyRunOutput");

    auto fake{ [&mat{auxiliary_materials()}] () { return mat / "FakeProject"; } };

    // Create tests and run
    fs::copy(fake() / "Source" / "fakeProject", generated() / "Source" / "generatedProject", fs::copy_options::recursive | fs::copy_options::skip_existing);
    fs::create_directory(working_materials() / "Output");

    const auto createTestsCMakeBuildRun{b.create_cmake_build_run(working_materials() / "Output")};
    invoke(createTestsCMakeBuildRun);

    //check(LINE("Second CMake output existance"), fs::exists(createTestsCMakeBuildRun.cmake_output));
    //check(LINE("Second build output existance"), fs::exists(createTestsCMakeBuildRun.build_output));

    check_equivalence(LINE("Test Runner Output"), working_materials() / "Output", predictive_materials() / "Output");

    // Change several of the tests, and some of the source, rebuild and run
    fs::copy(auxiliary_materials() / "TestMaterials", generated() / "TestMaterials", fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    fs::copy(auxiliary_materials() / "ModifiedSource" / "UsefulThings.hpp", generated() / "Source" / "generatedProject" / "Utilities", fs::copy_options::overwrite_existing);
    fs::copy(auxiliary_materials() / "ModifiedSource" / "UsefulThings.cpp", generated() / "Source" / "generatedProject" / "Utilities", fs::copy_options::overwrite_existing);
    fs::copy(auxiliary_materials() / "ModifiedSource" / "Maths", generated() / "Source" / "generatedProject" / "Maths", fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    fs::copy(auxiliary_materials() / "ModifiedSource" / "Thing", generated() / "Source" / "generatedProject" / "Utilities" / "Thing", fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    fs::copy(auxiliary_materials() / "ModifiedTests" / "FooTest.cpp", generated() / "Tests" / "Stuff", fs::copy_options::overwrite_existing);
    fs::copy(auxiliary_materials() / "ModifiedTests" / "UsefulThingsFreeTest.cpp", generated() / "Tests" / "Utilities", fs::copy_options::overwrite_existing);
    fs::copy(auxiliary_materials() / "ModifiedTests" / "Maths", generated() / "Tests" / "Maths", fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    fs::copy(auxiliary_materials() / "ModifiedTests" / "Thing", generated() / "Tests" / "Utilities" / "Thing", fs::copy_options::recursive | fs::copy_options::overwrite_existing);

    fs::last_write_time(generated() / "Source" / "generatedProject" / "Utilities" / "UsefulThings.cpp",          fs::file_time_type::clock::now());
    fs::last_write_time(generated() / "Source" / "generatedProject" / "Maths" / "Probability.cpp",               fs::file_time_type::clock::now());
    fs::last_write_time(generated() / "Source" / "generatedProject" / "Utilities" / "Thing" / "UniqueThing.cpp", fs::file_time_type::clock::now());
    fs::last_write_time(generated() / "Tests" / "Stuff" / "FooTest.cpp",                                         fs::file_time_type::clock::now());
    fs::last_write_time(generated() / "Tests" / "Utilities" / "UsefulThingsFreeTest.cpp",                        fs::file_time_type::clock::now());
    fs::last_write_time(generated() / "Tests" / "Maths" / "ProbabilityTest.cpp",                                 fs::file_time_type::clock::now());
    fs::last_write_time(generated() / "Tests" / "Maths" / "ProbabilityTestingDiagnostics.cpp",                   fs::file_time_type::clock::now());
    fs::last_write_time(generated() / "Tests" / "Utilities" / "Thing" / "UniqueThingTest.cpp",                   fs::file_time_type::clock::now());
    fs::last_write_time(generated() / "Tests" / "Utilities" / "Thing" / "UniqueThingTestingDiagnostics.cpp",     fs::file_time_type::clock::now());
    fs::create_directory(working_materials() / "RebuiltOutput");

    const auto rebuildRun{b.rebuild_run(working_materials() / "RebuiltOutput")};
    invoke(rebuildRun);

    //check(LINE("Third CMake output existance"), fs::exists(rebuildRun.cmake_output));
    //check(LINE("Third build output existance"), fs::exists(rebuildRun.build_output));

    check_equivalence(LINE("Test Runner Output"), working_materials() / "RebuiltOutput", predictive_materials() / "RebuiltOutput");
    fs::create_directory(working_materials() / "TestAll");
    const auto generatedProject{working_materials().parent_path() / "GeneratedProject"};

    const fs::path mainCpp{"TestAll/TestMain.cpp"}, mainCmake{"TestAll/CMakeLists.txt"};
    fs::copy_file(generatedProject / mainCpp,   working_materials() / mainCpp);
    fs::copy_file(generatedProject / mainCmake, working_materials() / mainCmake);
    check_equivalence(LINE("TestMain.cpp"),  working_materials() / mainCpp,   predictive_materials() / mainCpp);
    check_equivalence(LINE("CMakeLists.tt"), working_materials() / mainCmake, predictive_materials() / mainCmake);

    fs::copy(generated() / "TestMaterials", working_materials() / "OriginalTestMaterials", fs::copy_options::recursive);
    check_equivalence(LINE("Original Test Materials"), working_materials() / "OriginalTestMaterials", predictive_materials() / "OriginalTestMaterials");

    // Rerun but update materials
    fs::create_directory(working_materials() / "RunWithUpdateOutput");
    invoke(b.materials_update(working_materials() / "RunWithUpdateOutput"));
    check_equivalence(LINE("Test Runner Output"), working_materials() / "RunWithUpdateOutput", predictive_materials() / "RunWithUpdateOutput");

    fs::copy(generated() / "TestMaterials", working_materials() / "UpdatedTestMaterials", fs::copy_options::recursive);
    check_equivalence(LINE("Updated Test Materials"), working_materials() / "UpdatedTestMaterials", predictive_materials() / "UpdatedTestMaterials");

    // Rerun and do a dump
    fs::create_directory(working_materials() / "RunPostUpdate");
    invoke(b.dump(working_materials() / "RunPostUpdate"));
    check_equivalence(LINE("Test Runner Output"), working_materials() / "RunPostUpdate", predictive_materials() / "RunPostUpdate");

    fs::create_directory(working_materials() / "Dump");
    fs::copy(generated() / "output" / "Recovery" / "Dump.txt", working_materials() / "Dump");
    check_equivalence(LINE("Dump File"), working_materials() / "Dump", predictive_materials() / "Dump");

    // Rename generated() / TestMaterials / Stuff / FooTest / WorkingCopy / RepresentativeCases,
    // in order to cause the check in FooTest.cpp to throw, thereby allowing the recovery
    // mode to be tested.

    const auto generatedWorkingCopy{generated() / "TestMaterials" / "Stuff" / "FooTest" / "WorkingCopy"};
    fs::copy(generatedWorkingCopy / "RepresentativeCases", generatedWorkingCopy / "RepresentativeCasesTemp", fs::copy_options::recursive);
    fs::remove_all(generatedWorkingCopy / "RepresentativeCases");

    fs::create_directory(working_materials() / "RunRecovery");
    invoke(b.recovery(working_materials() / "RunRecovery"));
    check_equivalence(LINE("Test Runner Output"), working_materials() / "RunRecovery", predictive_materials() / "RunRecovery");

    fs::create_directory(working_materials() / "Recovery");
    fs::copy(generated() / "output" / "Recovery" / "Recovery.txt", working_materials() / "Recovery");
    check_equivalence(LINE("Recovery File"), working_materials() / "Recovery", predictive_materials() / "Recovery");

    // Rename generated() / TestMaterials / Stuff / FooTest / Prediction / RepresentativeCases,
    // in order to cause the check in FooTest.cpp to throw mid-check, thereby allowing the recovery
    // mode to be tested.

    const auto generatedPredictive{generated() / "TestMaterials" / "Stuff" / "FooTest" / "Prediction"};
    fs::copy(generatedPredictive / "RepresentativeCases", generatedPredictive / "RepresentativeCasesTemp", fs::copy_options::recursive);
    fs::remove_all(generatedPredictive / "RepresentativeCases");

    fs::create_directory(working_materials() / "RunRecoveryMidCheck");
    invoke(b.recovery(working_materials() / "RunRecoveryMidCheck"));
    check_equivalence(LINE("Test Runner Output"), working_materials() / "RunRecoveryMidCheck", predictive_materials() / "RunRecoveryMidCheck");

    fs::create_directory(working_materials() / "RecoveryMidCheck");
    fs::copy(generated() / "output" / "Recovery" / "Recovery.txt", working_materials() / "RecoveryMidCheck");
    check_equivalence(LINE("Recovery File"), working_materials() / "RecoveryMidCheck", predictive_materials() / "RecoveryMidCheck");
  }
}
