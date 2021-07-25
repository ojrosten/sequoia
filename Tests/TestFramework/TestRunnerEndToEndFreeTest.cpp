////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerEndToEndFreeTest.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"

#include "Parsing/CommandLineArgumentsTestingUtilities.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    [[nodiscard]]
    std::string run_cmd()
    {
      if constexpr (with_msvc_v)
      {
#ifdef CMAKE_INTDIR
        return std::string{CMAKE_INTDIR}.append("\\TestAll.exe");
#else
        throw std::logic_error{"Unable to find preprocessor definition for CMAKE_INTDIR"};
#endif
      }
      else
      {
        return "./TestAll";
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
  }

  cmd_builder::cmd_builder(const std::filesystem::path& projRoot)
    : mainDir{projRoot / "TestAll"}
    , buildDir{project_paths::cmade_build_dir(projRoot, mainDir)}
  {}

  void cmd_builder::create_build_run(const std::filesystem::path& creationOutput, std::string_view buildOutput, const std::filesystem::path& output) const
  {
    invoke(   cd_cmd(buildDir)
           && shell_command{"", create_cmd(), creationOutput / "CreationOutput.txt" }
           && build_cmd(buildDir, buildOutput)
           && shell_command{"", run_cmd(), output / "TestRunOutput.txt" }
           && shell_command{"", run_cmd().append(" select ../../../Tests/HouseAllocationTest.cpp")
                                         .append(" select Maybe/MaybeTest.cpp")
                                         .append(" select FooTest.cpp"),
                            output / "SpecifiedSourceOutput.txt"}
           && shell_command{"", run_cmd().append(" select FooTest.cpp prune"), output / "SelectedSourcePruneConflictOutput.txt"}
           && shell_command{"", run_cmd().append(" select Plurgh.cpp test Absent select Foo test FooTest.cpp"), output / "FailedSpecifiedSourceOutput.txt"}
           && shell_command{"", run_cmd().append(" test Foo"), output / "SpecifiedFamilyOutput.txt"}
           && shell_command{"", run_cmd().append(" test Foo prune"), output / "SpecifiedFamilyPruneConflictOutput.txt"}
           && shell_command{"", run_cmd().append(" prune --cutoff namespace"), output / "FullyPrunedOutput.txt"}
           && shell_command{"", run_cmd().append(" -v"), output / "VerboseOutput.txt"}
           && shell_command{"", run_cmd().append(" --help"), output / "HelpOutput.txt"});
  }

  void cmd_builder::rebuild_run(const std::filesystem::path& outputDir, std::string_view cmakeOutput, std::string_view buildOutput, std::string_view options) const
  {
    invoke(cd_cmd(mainDir) && cmake_cmd(buildDir, cmakeOutput) && build_cmd(buildDir, buildOutput) && run(outputDir, options));
  }

  void cmd_builder::run_executable(const std::filesystem::path& outputDir, std::string_view options) const
  {
    fs::create_directory(outputDir);
    invoke(run(outputDir, options));
  }

  [[nodiscard]]
  shell_command cmd_builder::run(const std::filesystem::path& outputDir, std::string_view options) const
  {
    return cd_cmd(buildDir) && shell_command("", run_cmd().append(" ").append(options), outputDir / "TestRunOutput.txt");
  }

  [[nodiscard]]
  std::string_view test_runner_end_to_end_test::source_file() const noexcept
  {
    return __FILE__;
  }

  [[nodiscard]]
  std::filesystem::path test_runner_end_to_end_test::generated_project() const
  {
    return working_materials().parent_path() / "GeneratedProject";
  }

  void test_runner_end_to_end_test::copy_aux_materials(const std::filesystem::path& relativeFrom, const std::filesystem::path& relativeDirTo) const
  {
    const auto absoluteFrom{auxiliary_materials() / relativeFrom};
    const auto absoluteTo{generated_project() / relativeDirTo};
    fs::copy(absoluteFrom, absoluteTo, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
    const auto now{fs::file_time_type::clock::now()};

    if(fs::is_regular_file(absoluteFrom))
    {
      fs::last_write_time(absoluteTo / relativeFrom.filename(), now);
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
    auto fake{[&mat{auxiliary_materials()}] () { return mat / "FakeProject"; }};

    fs::copy(fake() / "Source" / "fakeProject", generated_project() / "Source" / "generatedProject", fs::copy_options::recursive | fs::copy_options::skip_existing);
    fs::create_directory(working_materials() / "CreationOutput");
    fs::create_directory(working_materials() / "Output");

    b.create_build_run(working_materials() / "CreationOutput", "BuildOutput2.txt", working_materials() / "Output");

    // Note: the act of creation invokes cmake, and so the first check implicitly checks the cmake output
    check_equivalence(description, working_materials() / "CreationOutput", predictive_materials() / "CreationOutput");
    check(append_lines(description, "Second build output existance"), fs::exists(b.buildDir / "BuildOutput2.txt"));
    check_equivalence(append_lines(description, "Test Runner Output"), working_materials() / "Output", predictive_materials() / "Output");
  }

  void test_runner_end_to_end_test::run_and_check(std::string_view description, const cmd_builder& b, std::string_view relOutputDir, std::string_view options)
  {
    b.run_executable(working_materials() / relOutputDir, options);
    check_equivalence(description, working_materials() / relOutputDir, predictive_materials() / relOutputDir);
  }

  void test_runner_end_to_end_test::rebuild_run_and_check(std::string_view description, const cmd_builder& b, std::string_view relOutputDir, std::string_view CMakeOutput, std::string_view BuildOutput, std::string_view options)
  {
    fs::create_directory(working_materials() / relOutputDir);

    b.rebuild_run(working_materials() / relOutputDir, CMakeOutput, BuildOutput, options);
    check_equivalence(description, working_materials() / relOutputDir, predictive_materials() / relOutputDir);
    check(append_lines(description, "CMake output existance"), fs::exists(b.mainDir / CMakeOutput));
    check(append_lines(description, "Build output existance"), fs::exists(b.buildDir / BuildOutput));
  }

  void test_runner_end_to_end_test::run_tests()
  {
    test_project_creation();
  }

  void test_runner_end_to_end_test::test_project_creation()
  {
    const auto seqRoot{test_repository().parent_path()};
    const auto testMain{seqRoot/"TestAll/TestMain.cpp"};
    const auto includeTarget{seqRoot/"TestCommon/TestIncludes.hpp"};

    const project_paths paths{seqRoot, testMain, includeTarget};

    check(LINE("Command processor existance"), std::system(nullptr) > 0);

    commandline_arguments args{"",
                               "init",
                               "Oliver Jacob Rosten",
                               generated_project().string(),
                               "\t",
                               "--to-files", "GenerationOutput.txt",
                               "--no-ide"};

    std::stringstream outputStream{};
    test_runner tr{args.size(), args.get(), "Oliver J. Rosten", paths, "  ", outputStream};

    //=================== Initialize, cmake and build new project ===================//
    tr.execute();

    fs::create_directory(working_materials() / "InitOutput");
    if(std::ofstream file{working_materials() / "InitOutput" / "io.txt"})
    {
      file << outputStream.rdbuf();
    }

    const cmd_builder b{generated_project()};

    check(LINE("First CMake output existance"), fs::exists(b.mainDir / "GenerationOutput.txt"));
    check(LINE("First build output existance"), fs::exists(b.buildDir / "GenerationOutput.txt"));
    check(LINE("First git output existance"), fs::exists(generated_project() / "GenerationOutput.txt"));
    check(LINE(".git existance"), fs::exists(generated_project() / ".git"));
    if constexpr(with_msvc_v)
    {
      // There seems to be a problem whereby std::filesystem::remove_all won't remove
      // .git on windows. This causes clean up to fail when the testing framework is
      // invoked. Hence, it is manually removed here.
      invoke(cd_cmd(generated_project().parent_path()) && std::string{"rd /s /q "}.append((generated_project() / ".git").string()));
    }

    fs::copy(generated_project() / "GenerationOutput.txt", working_materials() / "InitOutput");
    check_equivalence(LINE(""), working_materials() / "InitOutput", predictive_materials() / "InitOutput");

    //=================== Run the test executable ===================//

    run_and_check(LINE("Empty Run"), b, "EmptyRunOutput", "");

    //=================== Create tests and run ===================//

    create_run_and_check(LINE("Test Runner Creation Output"), b);

    //=================== Rerun with async execution ===================//

    run_and_check(LINE("Run asynchronously"), b, "RunAsync", "-a");

    //=================== Change some test materials and run with prune ===================//

    fs::copy(auxiliary_materials() / "ModifiedTests" / "FooTest.cpp", generated_project() / "Tests" / "Stuff", fs::copy_options::overwrite_existing);
    fs::last_write_time(generated_project() / "Tests" / "Stuff" / "FooTest.cpp", fs::file_time_type::clock::now());
    fs::copy(auxiliary_materials() / "TestMaterials", generated_project() / "TestMaterials", fs::copy_options::recursive | fs::copy_options::overwrite_existing);

    rebuild_run_and_check(LINE("Change Materials (pruned)"), b, "RunWithChangedMaterials", "CMakeOutput3.txt", "BuildOutput3.txt", "prune --cutoff namespace");

    // Check materials are unchanged
    fs::copy(generated_project() / "TestMaterials", working_materials() / "OriginalTestMaterials", fs::copy_options::recursive);
    check_equivalence(LINE("Original Test Materials"), working_materials() / "OriginalTestMaterials", predictive_materials() / "OriginalTestMaterials");

    //=================== Rerun with prune but update materials ===================//

    run_and_check(LINE("Updated Materials"), b, "RunWithUpdateOutput", "prune --cutoff namespace u");

    fs::copy(generated_project() / "TestMaterials", working_materials() / "UpdatedTestMaterials", fs::copy_options::recursive);
    check_equivalence(LINE("Updated Test Materials"), working_materials() / "UpdatedTestMaterials", predictive_materials() / "UpdatedTestMaterials");

    //=================== Rerun with prune, which should detect the change to materials ===================//

    run_and_check(LINE("Pruned output, post materials update"), b, "RunWithPruneOutput", "prune");

    //=================== Rerun again with prune, which should do nothing  ===================//

    run_and_check(LINE("Prune again, no tests should run"), b, "NullRunWithPruneOutput", "prune");

    //=================== Change a file, don't build and run the prune ===================//

    copy_aux_materials("ModifiedSource/UsefulThings.hpp", "Source/generatedProject/Utilities");
    run_and_check(LINE("Attempt to prune when build is out of date"), b, "PruneWithStaleBuild", "prune");

    //=================== Change several of the tests, and some of the source, rebuild and run asynchronously ===================//

    copy_aux_materials("ModifiedSource/UsefulThings.cpp",        "Source/generatedProject/Utilities");
    copy_aux_materials("ModifiedSource/Maths",                   "Source/generatedProject/Maths");
    copy_aux_materials("ModifiedSource/Thing",                   "Source/generatedProject/Utilities/Thing");
    copy_aux_materials("ModifiedTests/UsefulThingsFreeTest.cpp", "Tests/Utilities");
    copy_aux_materials("ModifiedTests/Maths",                    "Tests/Maths");
    copy_aux_materials("ModifiedTests/Thing",                    "Tests/Utilities/Thing");

    rebuild_run_and_check(LINE("Rebuild and run after source/test changes (pruned)"), b, "RebuiltOutput", "CMakeOutput4.txt", "BuildOutput4.txt", "prune --cutoff namespace -a");

    check_equivalence(LINE("Test Runner Output"), working_materials() / "RebuiltOutput", predictive_materials() / "RebuiltOutput");
    fs::create_directory(working_materials() / "TestAll");
    const auto generatedProject{working_materials().parent_path() / "GeneratedProject"};

    const fs::path mainCpp{"TestAll/TestAllMain.cpp"}, mainCmake{"TestAll/CMakeLists.txt"};
    fs::copy_file(generatedProject / mainCpp,   working_materials() / mainCpp);
    fs::copy_file(generatedProject / mainCmake, working_materials() / mainCmake);
    check_equivalence(LINE("TestAllMain.cpp"),  working_materials() / mainCpp,   predictive_materials() / mainCpp);
    check_equivalence(LINE("CMakeLists.tt"), working_materials() / mainCmake, predictive_materials() / mainCmake);


    //=================== Rerun and do a dump ===================//

    run_and_check(LINE("Do a dump"), b, "RunPostUpdate", "dump");

    fs::create_directory(working_materials() / "Dump");
    fs::copy(generated_project() / "output" / "Recovery" / "Dump.txt", working_materials() / "Dump");
    check_equivalence(LINE("Dump File"), working_materials() / "Dump", predictive_materials() / "Dump");

    //=================== Rerun in the presence of an exception ===================//
    // Rename generated_project() / TestMaterials / Stuff / FooTest / WorkingCopy / RepresentativeCases,
    // in order to cause the check in FooTest.cpp to throw, thereby allowing the recovery
    // mode to be tested.

    const auto generatedWorkingCopy{generated_project() / "TestMaterials" / "Stuff" / "FooTest" / "WorkingCopy"};
    fs::copy(generatedWorkingCopy / "RepresentativeCases", generatedWorkingCopy / "RepresentativeCasesTemp", fs::copy_options::recursive);
    fs::remove_all(generatedWorkingCopy / "RepresentativeCases");

    run_and_check(LINE("Recovery mode"), b, "RunRecovery", "--recovery");

    fs::create_directory(working_materials() / "Recovery");
    fs::copy(generated_project() / "output" / "Recovery" / "Recovery.txt", working_materials() / "Recovery");
    check_equivalence(LINE("Recovery File"), working_materials() / "Recovery", predictive_materials() / "Recovery");

    //=================== Rerun in the presence of an exception mid-check ===================//
    // Rename generated_project() / TestMaterials / Stuff / FooTest / Prediction / RepresentativeCases,
    // in order to cause the check in FooTest.cpp to throw mid-check, thereby allowing the recovery
    // mode to be tested.

    const auto generatedPredictive{generated_project() / "TestMaterials" / "Stuff" / "FooTest" / "Prediction"};
    fs::copy(generatedPredictive / "RepresentativeCases", generatedPredictive / "RepresentativeCasesTemp", fs::copy_options::recursive);
    fs::remove_all(generatedPredictive / "RepresentativeCases");

    run_and_check(LINE("Recovery mode, throw mid-check"), b, "RunRecoveryMidCheck", "--recovery");

    fs::create_directory(working_materials() / "RecoveryMidCheck");
    fs::copy(generated_project() / "output" / "Recovery" / "Recovery.txt", working_materials() / "RecoveryMidCheck");
    check_equivalence(LINE("Recovery File"), working_materials() / "RecoveryMidCheck", predictive_materials() / "RecoveryMidCheck");
  }
}
