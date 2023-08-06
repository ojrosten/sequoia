////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "TestRunnerEndToEndFreeTest.hpp"
#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "sequoia/TestFramework/ProjectCreator.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"

#include <fstream>
#include <numeric>

namespace sequoia::testing
{
  using namespace runtime;
  namespace fs = std::filesystem;

  namespace
  {
    [[nodiscard]]
    std::string run_cmd()
    {
      return with_msvc_v ? "TestAll.exe" : "./TestAll";
    }

    [[nodiscard]]
    std::string create_cmd()
    {
      return run_cmd().append(" create free_test Utilities.hpp"
        " create free_test \"Utilities/UsefulThings.hpp\" gen-source utils"
        " create free_test \"Source/generatedProject/Stuff/Bar.hpp\""
        " create free \"Unstable/Flipper.hpp\" -s Unstable"
        " create regular_test \"other::functional::maybe<class T>\" \"std::optional<T>\" gen-source Maybe"
        " create regular_test \"stuff::oldschool\" double --header \"NoTemplate.hpp\""
        " create regular \"maths::probability\" double gen-source Maths"
        " create move_only_test \"bar::baz::foo<maths::floating_point T>\" T"
        " create move_only \"stuff::unique_thing\" double gen-source Utilities/Thing"
        " create regular_allocation_test container"
        " create move_only_allocation_test house"
        " create performance_test Container.hpp");
    }
  }

  cmd_builder::cmd_builder(const std::filesystem::path& projRoot, const build_paths& applicationBuildPaths)
    : m_Main{projRoot / main_paths::default_main_cpp_from_root()}
    , m_Build{make_new_build_paths(projRoot, applicationBuildPaths)}
  {}

  [[nodiscard]]
  fs::path cmd_builder::cmake_cache_dir() const
  {
    if(!build().cmake_cache())
      throw std::logic_error{"No CMakeCache.txt"};

    return build().cmake_cache()->parent_path();
  }

  void cmd_builder::create_build_run(const std::filesystem::path& creationOutput, std::string_view buildOutput, const std::filesystem::path& output) const
  {
    invoke(cd_cmd(build().executable_dir())
      && shell_command{"", create_cmd(), creationOutput / "CreationOutput.txt"}
      && build_cmd(build(), buildOutput)
      && cd_cmd(build().executable_dir())
      && shell_command{"", run_cmd(), output / "TestRunOutput.txt"}
      && shell_command{"",
                       run_cmd().append(" select ../../../Tests/HouseAllocationTest.cpp")
                        .append(" select Maybe/MaybeTest.cpp")
                        .append(" select FooTest.cpp"),
                       output / "SpecifiedSourceOutput.txt"}
      && shell_command{"", run_cmd().append(" select FooTest.cpp prune"), output / "SelectedSourcePruneConflictOutput.txt"}
      && shell_command{"", run_cmd().append(" select Plurgh.cpp test Absent select Foo test FooTest.cpp"), output / "FailedSpecifiedSourceOutput.txt"}
      && shell_command{"", run_cmd().append(" test Foo"), output / "SpecifiedSuiteOutput.txt"}
      && shell_command{"", run_cmd().append(" test Foo prune"), output / "SpecifiedSuitePruneConflictOutput.txt"}
      && shell_command{"", run_cmd().append(" prune --cutoff namespace"), output / "FullyPrunedOutput.txt"}
      && shell_command{"", run_cmd().append(" -v"), output / "VerboseOutput.txt"}
      && shell_command{"", run_cmd().append(" -v select FooTest.cpp test Foo"), output / "SelectFromTestedSuiteOutput.txt"}
      && shell_command{"", run_cmd().append(" --help"), output / "HelpOutput.txt"});
  }

  void cmd_builder::rebuild_run(const std::filesystem::path& outputDir, std::string_view cmakeOutput, std::string_view buildOutput, std::string_view options) const
  {
    invoke(cd_cmd(main().dir()) && cmake_cmd(std::nullopt, build(), cmakeOutput) && build_cmd(build(), buildOutput) && run(outputDir, options));
  }

  void cmd_builder::run_executable(const std::filesystem::path& outputDir, std::string_view options) const
  {
    fs::create_directory(outputDir);
    invoke(run(outputDir, options));
  }

  [[nodiscard]]
  shell_command cmd_builder::run(const std::filesystem::path& outputDir, std::string_view options) const
  {
    return cd_cmd(build().executable_dir()) && shell_command("", run_cmd().append(" ").append(options), outputDir / "TestRunOutput.txt");
  }

  [[nodiscard]]
  std::filesystem::path test_runner_end_to_end_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  [[nodiscard]]
  std::filesystem::path test_runner_end_to_end_test::generated_project() const
  {
    return working_materials().parent_path() /= "GeneratedProject";
  }

  void test_runner_end_to_end_test::copy_aux_materials(const std::filesystem::path& relativeFrom, const std::filesystem::path& relativeTo) const
  {
    const auto absoluteFrom{auxiliary_materials() /= relativeFrom};
    const auto absoluteTo{generated_project() / relativeTo};
    fs::copy(absoluteFrom, absoluteTo, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    const auto now{fs::file_time_type::clock::now() + std::chrono::seconds(1)};

    if(fs::is_regular_file(absoluteFrom))
    {
      if(fs::is_directory(absoluteTo))
      {
        fs::last_write_time(absoluteTo / relativeFrom.filename(), now);
      }
    }
    else if(fs::is_directory(absoluteFrom))
    {
      for(auto& entry : fs::recursive_directory_iterator(absoluteTo))
      {
        fs::last_write_time(entry.path(), now);
      }
    }
  }

  void  test_runner_end_to_end_test::create_run_and_check(std::string_view description, const cmd_builder& b)
  {
    auto fake{[this] () { return auxiliary_materials() /= "FakeProject"; }};

    fs::copy(fake() / "Source" / "fakeProject", generated_project() / "Source" / "generatedProject", fs::copy_options::recursive | fs::copy_options::skip_existing);
    fs::create_directory(working_materials() /= "CreationOutput");
    fs::create_directory(working_materials() /= "Output");

    b.create_build_run(working_materials() /= "CreationOutput", "BuildOutput2.txt", working_materials() /= "Output");

    // Note: the act of creation invokes cmake, and so the first check implicitly checks the cmake output
    check(equivalence, description, working_materials() /= "CreationOutput", predictive_materials() /= "CreationOutput");
    check(append_lines(description, "Second build output existance"), fs::exists(b.cmake_cache_dir() / "BuildOutput2.txt"));
    check(equivalence, append_lines(description, "Test Runner Output"), working_materials() /= "Output", predictive_materials() /= "Output");
  }

  void test_runner_end_to_end_test::run_and_check(std::string_view description, const cmd_builder& b, std::string_view relOutputDir, std::string_view options)
  {
    b.run_executable(working_materials() /= relOutputDir, options);
    check(equivalence, description, working_materials() /= relOutputDir, predictive_materials() /= relOutputDir);
  }

  void test_runner_end_to_end_test::rebuild_run_and_check(std::string_view description, const cmd_builder& b, std::string_view relOutputDir, std::string_view CMakeOutput, std::string_view BuildOutput, std::string_view options)
  {
    fs::create_directory(working_materials() /= relOutputDir);

    b.rebuild_run(working_materials() /= relOutputDir, CMakeOutput, BuildOutput, options);
    check(equivalence, description, working_materials() /= relOutputDir, predictive_materials() /= relOutputDir);
    check(append_lines(description, "CMake output existance"), fs::exists(b.main().dir() / CMakeOutput));
    check(append_lines(description, "Build output existance"), fs::exists(b.cmake_cache_dir() / BuildOutput));
  }

  void test_runner_end_to_end_test::check_project_files(std::string_view description, const cmd_builder& b)
  {
    const std::filesystem::path subdirs{"ProjectFiles/win"};
    fs::create_directories(working_materials() /= subdirs);
    if constexpr(with_msvc_v)
    {
      const auto projFile{b.cmake_cache_dir() / "TestAll.vcxproj"};
      fs::copy(projFile, working_materials() /= subdirs);
    }
    else
    {
      // TO DO: fake this for now on other platforms to ensure the number of checks/deep checks match;
      // A solution might be to introduce platform-specific summaries.
      fs::copy(predictive_materials() /= subdirs, working_materials() /= subdirs);
    }

    check(equivalence, description, working_materials() /= subdirs, predictive_materials() /= subdirs);
  }

  void test_runner_end_to_end_test::run_tests()
  {
    test_project_creation();
  }

  void test_runner_end_to_end_test::test_project_creation()
  {
    check(report_line("Command processor existance"), std::system(nullptr) > 0);

    commandline_arguments args{get_project_paths().discovered().executable().generic_string(),
                               "init",
                               "Oliver Jacob Rosten",
                               generated_project().string(),
                               "\t",
                               "--to-files", "GenerationOutput.txt",
                               "--no-ide"};

    std::stringstream outputStream{};

    const auto relativeMainCppPath{rebase_from(get_project_paths().main().file(), project_root())};
    test_runner tr{args.size(), args.get(), "Oliver J. Rosten", {relativeMainCppPath.generic_string(), {}, "TestCommon/TestIncludes.hpp"}, "  ", outputStream};

    //=================== Initialize, cmake and build new project ===================//

    fs::create_directory(working_materials() /= "InitOutput");
    if(std::ofstream file{working_materials() /= "InitOutput/io.txt"})
    {
      file << outputStream.rdbuf();
    }

    const cmd_builder b{generated_project(), get_project_paths().build()};

    check(report_line("First CMake output existance"), fs::exists(b.main().dir() / "GenerationOutput.txt"));
    check(report_line("First build output existance"), fs::exists(b.cmake_cache_dir() / "GenerationOutput.txt"));
    check(report_line("First git output existance"), fs::exists(generated_project() / "GenerationOutput.txt"));
    check(report_line(".git existance"), fs::exists(generated_project() / ".git"));

    fs::copy(generated_project() / "GenerationOutput.txt", working_materials() /= "InitOutput");
    check(equivalence, report_line(""), working_materials() /= "InitOutput", predictive_materials() /= "InitOutput");

    check_project_files(report_line("Project Files"), b);

    //=================== Run the test executable ===================//

    run_and_check(report_line("Empty Run"), b, "EmptyRunOutput", "");

    //=================== Create tests and run ===================//

    create_run_and_check(report_line("Test Runner Creation Output"), b);
    fs::copy(generated_project() /= "output/TestSummaries", working_materials() /= "TestSummaries_0", fs::copy_options::recursive);
    check(equivalence, report_line(""), working_materials() /= "TestSummaries_0", predictive_materials() /= "TestSummaries_0");


    //=================== Rerun with async execution ===================//
    // --> async depth should be automatically set to "suite" since number of families is > 4

    run_and_check(report_line("Run synchronously"), b, "RunSynchronous", "--serial");

    //=================== Rerun with async selecting 3 tests from 3 families ===================//
    // --> async depth should be automatically set to "test" since number of families is < 4

    run_and_check(report_line("Run asynchronously with 3 selected tests"), b, "RunAsyncThreeTests",
                       "select HouseAllocationTest.cpp select Maths/ProbabilityTest.cpp select Maybe/MaybeTest.cpp");

    //=================== Rerun with async selecting 4 tests from 4 families===================//
    // --> async depth should be automatically set to "suite"

    run_and_check(report_line("Run asynchronously with 4 selected tests"), b, "RunAsyncFourTests",
                       "select HouseAllocationTest.cpp select Maths/ProbabilityTest.cpp select Maybe/MaybeTest.cpp"
                       " select Stuff/FooTest.cpp");

    //=================== Rerun with async selecting 4 tests from 4 families, and setting async-depth to test===================//

    run_and_check(report_line("Run asynchronously with 4 selected tests"), b, "RunAsyncFourTestsDepthTest",
                       "select HouseAllocationTest.cpp select Maths/ProbabilityTest.cpp select Maybe/MaybeTest.cpp"
                       " select Stuff/FooTest.cpp");

    //=================== Rerun with async, selecting 2 tests, and setting async-depth to suite ===================//

    run_and_check(report_line("Run asynchronously with 2 selected tests"), b, "RunAsyncTwoTestsDepthSuite",
                       "select HouseAllocationTest.cpp select Maths/ProbabilityTest.cpp");

    //=================== Rerun with async, selecting one suite ===================//

    run_and_check(report_line("Run asynchronously with 1 suite"), b, "RunAsyncOneTestOneSuite", "test Probability");

    //=================== Rerun, seeking instabilities in sandbox mode ===================//

    run_and_check(report_line("Run in sandbox mode"), b, "RunLocateInstabilitySandbox", "locate 2 --sandbox");

    //=================== Change some test materials and run with prune ===================//

    copy_aux_materials("ModifiedTests/Stuff/FooTest.cpp", "Tests/Stuff");
    copy_aux_materials("TestMaterials", "TestMaterials");

    rebuild_run_and_check(report_line("Change Materials (pruned)"), b, "RunWithChangedMaterials", "CMakeOutput3.txt", "BuildOutput3.txt", "prune --cutoff namespace");

    // Check materials are unchanged
    fs::copy(generated_project() / "TestMaterials", working_materials() /= "OriginalTestMaterials", fs::copy_options::recursive);
    check(equivalence, report_line("Original Test Materials"), working_materials() /= "OriginalTestMaterials", predictive_materials() /= "OriginalTestMaterials");

    //=================== Run again, locating instabilities, and try to update ===================//
    //--> update should be suppressed by instability location

    run_and_check(report_line("Instability location suppressing update"), b, "UpdateSuppressedByInstabilityLocation", "locate 2 prune -c namespace u");
    check(equivalence, report_line("Original Test Materials"), working_materials() /= "OriginalTestMaterials", predictive_materials() /= "OriginalTestMaterials");

    //=================== Rerun with prune but update materials ===================//

    run_and_check(report_line("Updated Materials"), b, "RunWithUpdateOutput", "prune --cutoff namespace u");

    fs::copy(generated_project() / "TestMaterials", working_materials() /= "UpdatedTestMaterials", fs::copy_options::recursive);
    check(equivalence, report_line("Updated Test Materials"), working_materials() /= "UpdatedTestMaterials", predictive_materials() /= "UpdatedTestMaterials");

    //=================== Rerun with prune, which should detect the change to materials ===================//

    run_and_check(report_line("Pruned output, post materials update"), b, "RunWithPruneOutput", "prune");

    //=================== Rerun again with prune, which should do nothing  ===================//

    run_and_check(report_line("Prune again, no tests should run"), b, "NullRunWithPruneOutput", "prune");

    //=================== Change a file, don't build and run with prune ===================//

    copy_aux_materials("ModifiedSource/UsefulThings.hpp", "Source/generatedProject/Utilities");
    run_and_check(report_line("Attempt to prune when build is out of date"), b, "PruneWithStaleBuild", "prune");

    //=================== Change several of the tests, and some of the source, rebuild and run asynchronously, with prune ===================//

    copy_aux_materials("ModifiedSource/UsefulThings.cpp", "Source/generatedProject/Utilities");
    copy_aux_materials("ModifiedSource/Maths",            "Source/generatedProject/Maths");
    copy_aux_materials("ModifiedSource/Thing",            "Source/generatedProject/Utilities/Thing");
    copy_aux_materials("ModifiedTests/Utilities",         "Tests/Utilities");
    copy_aux_materials("ModifiedTests/Maths",             "Tests/Maths");
    copy_aux_materials("ModifiedTests/Thing",             "Tests/Utilities/Thing");
    copy_aux_materials("ModifiedTests/Unstable",          "Tests/Unstable");

    rebuild_run_and_check(report_line("Rebuild and run after source/test changes (pruned)"), b, "RebuiltOutput", "CMakeOutput4.txt", "BuildOutput4.txt", "prune --cutoff namespace");

    check(equivalence, report_line("Test Runner Output"), working_materials() /= "RebuiltOutput", predictive_materials() /= "RebuiltOutput");
    fs::create_directory(working_materials() /= "TestAll");
    const auto generatedProject{working_materials().parent_path() /= "GeneratedProject"};

    const fs::path mainCpp{main_paths::default_main_cpp_from_root()},
                   mainCmake{main_paths::default_cmake_from_root()};
    fs::copy_file(generatedProject / mainCpp,   working_materials() /= mainCpp);
    fs::copy_file(generatedProject / mainCmake, working_materials() /= mainCmake);
    check(equivalence, report_line("TestAllMain.cpp"),  working_materials() /= mainCpp,   predictive_materials() /= mainCpp);
    check(equivalence, report_line("CMakeLists.tt"), working_materials() /= mainCmake, predictive_materials() /= mainCmake);

    //=================== Rerun with prune ===================//
    // --> only failing tests should rerun

    run_and_check(report_line("Pruned output, post failures"), b, "RunPrunePostFailureOutput", "prune -c namespace");

    //=================== Rerun and locate instabilities ===================//
    // --> UsefulThingsFreeTest.cpp will continue to exhibit a stable failure,
    // whereas  FlipperFreeTest.cpp is unstable

    run_and_check(report_line("Locate instabilities"), b, "RunLocateInstabilities", "locate 2");

    //=================== Rerun and locate instabilities, with pruning ===================//

    run_and_check(report_line("Locate instabilities"), b, "RunLocateInstabilitiesPrune", "locate 2 prune -c namespace");

    //=================== Rerun with selected, unstable test in sandbox mode ===================//
    // --> The first of the checks in FlipperFreeTest.cpp is stable in sandbox mode, but the second isn't

    run_and_check(report_line("Run in sandbox mode with an explicit selection"), b, "SelectRunLocateInstabilitySandbox",
      "locate 2 --sandbox select FlipperFreeTest.cpp");

    //=================== Rerun and do a dump ===================//

    run_and_check(report_line("Do a dump"), b, "RunPostUpdate", "dump");

    fs::create_directory(working_materials() /= "Dump");
    fs::copy(generated_project() /= "output/Recovery/Dump.txt", working_materials() /= "Dump");
    check(equivalence, report_line("Dump File"), working_materials() /= "Dump", predictive_materials() /= "Dump");

    //=================== Rerun in the presence of an exception ===================//
    // Rename generated_project() / TestMaterials / Stuff / FooTest / WorkingCopy / RepresentativeCases,
    // in order to cause the check in FooTest.cpp to throw, thereby allowing the recovery
    // mode to be tested.

    const auto generatedWorkingCopy{generated_project() /= "TestMaterials/Stuff/FooTest/WorkingCopy"};
    fs::copy(generatedWorkingCopy / "RepresentativeCases", generatedWorkingCopy / "RepresentativeCasesTemp", fs::copy_options::recursive);
    fs::remove_all(generatedWorkingCopy / "RepresentativeCases");

    run_and_check(report_line("Recovery mode"), b, "RunRecovery", "recover");

    fs::create_directory(working_materials() /= "Recovery");
    fs::copy(generated_project() /= "output/Recovery/Recovery.txt", working_materials() /= "Recovery");
    check(equivalence, report_line("Recovery File"), working_materials() /= "Recovery", predictive_materials() /= "Recovery");

    //=================== Rerun in the presence of an exception mid-check ===================//
    // Rename generated_project() / TestMaterials / Stuff / FooTest / Prediction / RepresentativeCases,
    // in order to cause the check in FooTest.cpp to throw mid-check, thereby allowing the recovery
    // mode to be tested.

    const auto generatedPredictive{generated_project() /= "TestMaterials/Stuff/FooTest/Prediction"};
    fs::copy(generatedPredictive / "RepresentativeCases", generatedPredictive / "RepresentativeCasesTemp", fs::copy_options::recursive);
    fs::remove_all(generatedPredictive / "RepresentativeCases");

    run_and_check(report_line("Recovery mode, throw mid-check"), b, "RunRecoveryMidCheck", "recover");

    fs::create_directory(working_materials() /= "RecoveryMidCheck");
    fs::copy(generated_project() /= "output/Recovery/Recovery.txt", working_materials() /= "RecoveryMidCheck");
    check(equivalence, report_line("Recovery File"), working_materials() /= "RecoveryMidCheck", predictive_materials() /= "RecoveryMidCheck");

    //=================== Change one of the failing tests, and 'select' it at the same time as breaking a different test ===================//

    copy_aux_materials("FurtherModifiedTests/UsefulThingsFreeTest.cpp", "Tests/Utilities");
    copy_aux_materials("FurtherModifiedTests/ProbabilityTest.cpp", "Tests/Maths");
    rebuild_run_and_check(report_line("Rebuild, run and 'select' after fixing a test"), b, "RunSelectedFixedTest", "CMakeOutput5.txt", "BuildOutput5.txt", "select UsefulThingsFreeTest.cpp");

    check(equivalence, report_line("Fixed Test Output"), working_materials() /= "RunSelectedFixedTest", predictive_materials() /= "RunSelectedFixedTest");

    //=================== Rerun with prune to confirm that the previously selected test - now passing - is not run ===================//

    run_and_check(report_line("Passing test not included by prune"), b, "PassingTestExcludedByPrune", "prune -c namespace");

    //=================== Fix a failing test and 'select' it ===================//

    fs::copy(generatedPredictive / "RepresentativeCasesTemp", generatedPredictive / "RepresentativeCases", fs::copy_options::recursive);
    fs::remove_all(generatedPredictive / "RepresentativeCasesTemp");

    fs::copy(generatedWorkingCopy / "RepresentativeCasesTemp", generatedWorkingCopy / "RepresentativeCases", fs::copy_options::recursive);
    fs::remove_all(generatedWorkingCopy / "RepresentativeCasesTemp");

    run_and_check(report_line("Critical failure fixed"), b, "RunFixedCriticalFailure", "select FooTest.cpp");

    //=================== Rerun with prune to confirm that the previously selected test - now passing - is not run ===================//

    run_and_check(report_line("Fixed test not included by prune"), b, "AnotherPassingTestExcludedByPrune", "prune -c namespace");

    //=================== Fix the final failing test and 'test' it ===================//

    copy_aux_materials("ModifiedTests/Maths/ProbabilityTest.cpp", "Tests/Maths");

    rebuild_run_and_check(report_line("Rebuild, run and 'test' after fixing a test"), b, "RunSuiteWithFixedTest", "CMakeOutput6.txt", "BuildOutput6.txt", "test Probability");

    check(equivalence, report_line("Fixed Test Output"), working_materials() /= "RunSelectedFixedTest", predictive_materials() /= "RunSelectedFixedTest");

    //=================== Rerun with prune to confirm that the previously selected test - now passing - is not run ===================//

    run_and_check(report_line("Final fixed test not included by prune"), b, "FinalPassingTestExcludedByPrune", "prune -c namespace");
  }
}
