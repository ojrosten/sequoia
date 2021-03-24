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
    struct cmake_and_build_command
    {
      std::string cmd;
      std::filesystem::path cmake_output, build_output;
    };

    std::string& operator&&(std::string& lhs, std::string_view rhs)
    {
      return lhs.append(" && ").append(rhs);
    }

    [[nodiscard]]
    std::string operator&&(std::string_view lhs, std::string_view rhs)
    {
      std::string left{lhs};
      return left && rhs;
    }

    [[nodiscard]]
    cmake_and_build_command operator&&(std::string_view lhs, cmake_and_build_command rhs)
    {
      rhs.cmd = lhs && rhs.cmd;
      return rhs;
    }

    [[nodiscard]]
    cmake_and_build_command operator&&(cmake_and_build_command lhs, std::string_view rhs)
    {
      lhs.cmd = lhs.cmd && rhs;
      return lhs;
    }

    [[nodiscard]]
    std::string cd(const std::filesystem::path& dir)
    {
      return std::string{"cd "}.append(dir.string());
    }

    [[nodiscard]]
    std::string cmake_cmd(const std::filesystem::path& buildDir)
    {
      auto cmd{std::string{"cmake -S ."}.append(" -B \"").append(buildDir.string()).append("\" ")};

      if constexpr (has_msvc_v)
      {
        cmd.append("-G \"Visual Studio 16 2019\"");
      }
      else if constexpr (has_clang_v)
      {
        cmd.append("-D CMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++");
      }
      else if constexpr (has_gcc_v)
      {
        cmd.append("-D CMAKE_CXX_COMPILER=/usr/bin/g++");
      }

      return cmd;
    }

    [[nodiscard]]
    std::string build_cmd(const std::filesystem::path& buildDir)
    {
      auto cmd{cd(buildDir) && "cmake --build . --target TestMain"};
      if constexpr (has_msvc_v)
      {
        cmd.append(" --config Debug");
      }

      return cmd;
    }

    [[nodiscard]]
    std::string run_cmd()
    {
      if constexpr (has_msvc_v)
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
    std::string add_output_file(std::string cmd, const std::filesystem::path& file)
    {
      return !file.empty() ? cmd.append(" > ").append(file.string()) : cmd;
    }

    [[nodiscard]]
    std::string create_cmd()
    {
      return run_cmd().append(" create free_test Utilities.hpp"
                              " create regular_test \"other::functional::maybe<class T>\" \"std::optional<T>\""
                              " create move_only_test \"bar::baz::foo<maths::floating_point T>\" T"
                              " create regular_allocation_test container"
                              " create move_only_allocation_test house"
                              " create performance_test Container.hpp");
    }

    struct cmd_builder
    {
      std::filesystem::path mainDir, buildDir;

      [[nodiscard]]
      cmake_and_build_command cmake_and_build() const
      {
        return cmake_and_build("CMakeOutput.txt", "BuildOutput.txt");
      }

      [[nodiscard]]
      cmake_and_build_command create_cmake_build_run(const std::filesystem::path& output) const
      {
        return    cd(buildDir)
               && add_output_file(create_cmd(), output / "CreationOutput.txt")
               && cmake_and_build("CMakeOutput2.txt", "BuildOutput2.txt")
               && add_output_file(run_cmd(), output / "TestRunOutput.txt")
               && add_output_file(run_cmd().append(" select ../../../Tests/HouseAllocationTest.cpp")
                                           .append(" select Maybe/MaybeTest.cpp")
                                           .append(" select FooTest.cpp"),
                                  output / "SpecifiedSourceOutput.txt")
               && add_output_file(run_cmd().append(" select Plurgh.cpp"), output / "FailedSpecifiedSourceOutput.txt")
               && add_output_file(run_cmd().append(" test Foo"), output / "SpecifiedFamilyOutput.txt")
               && add_output_file(run_cmd().append(" -v"), output / "VerboseOutput.txt")
               && add_output_file(run_cmd().append(" --help"), output / "HelpOutput.txt");
      }

      [[nodiscard]]
      cmake_and_build_command rebuild_run(const std::filesystem::path& output) const
      {
        return {    cd(buildDir)
                 && cmake_and_build("CMakeOutput3.txt", "BuildOutput3.txt")
                 && add_output_file(run_cmd(), output / "TestRunOutput.txt")};
      }

      [[nodiscard]]
      std::string materials_update(const std::filesystem::path& output) const
      {
        return cd(buildDir) && add_output_file(run_cmd().append(" u"), output / "TestRunOutput.txt");
      }
    private:
      [[nodiscard]]
      cmake_and_build_command cmake_and_build(std::string_view cmakeOut, std::string_view buildOut) const
      {
        return {     cd(mainDir)
                  && add_output_file(cmake_cmd(buildDir), cmakeOut)
                  && add_output_file(build_cmd(buildDir), buildOut),
                  mainDir / cmakeOut,
                  buildDir / buildOut
               };
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

    auto fake{
      [&mat{auxiliary_materials()}]() {
        return mat / "FakeProject";
      }
    };
    
    const auto testMain{fake().append("TestSandbox").append("TestSandbox.cpp")};
    const auto includeTarget{fake().append("TestShared").append("SharedIncludes.hpp")};

    const repositories repos{fake()};

    auto generated{
      [&mat{working_materials()}]() {
        return mat.parent_path() / "GeneratedProject";
      }
    };

    commandline_arguments args{"", "init", "Oliver Jacob Rosten", generated().string()};

    std::stringstream outputStream{};
    test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, repos, outputStream};

    tr.execute();

    fs::create_directory(working_materials() / "InitOutput");
    if(std::ofstream file{working_materials() / "InitOutput" / "io.txt"})
    {
      file << outputStream.rdbuf();
    }

    check_equivalence(LINE(""), working_materials() / "InitOutput", predictive_materials() / "InitOutput");

    check(LINE("Command processor existance"), std::system(nullptr) > 0);

    const cmd_builder b{generated() / "TestAll", generated() / "build" / "CMade" / "TestAll"};

    const auto cmakeAndBuild{b.cmake_and_build()};
    std::system(cmakeAndBuild.cmd.c_str());
    check(LINE("First CMake output existance"), fs::exists(cmakeAndBuild.cmake_output));
    check(LINE("First build output existance"), fs::exists(cmakeAndBuild.build_output));

    fs::copy(fake() / "Source", generated() / "Source", fs::copy_options::recursive | fs::copy_options::skip_existing);
    fs::create_directory(working_materials() / "Output");

    const auto cmakeBuildRun{b.create_cmake_build_run(working_materials() / "Output")};
    std::system(cmakeBuildRun.cmd.c_str());

    check(LINE("Second CMake output existance"), fs::exists(cmakeBuildRun.cmake_output));
    check(LINE("Second build output existance"), fs::exists(cmakeBuildRun.build_output));

    check_equivalence(LINE(""), working_materials() / "Output", predictive_materials() / "Output");

    fs::copy(auxiliary_materials() / "TestMaterials", generated() / "TestMaterials", fs::copy_options::recursive | fs::copy_options::overwrite_existing);

    fs::copy(auxiliary_materials() / "FooTest.cpp", generated() / "Tests" / "Stuff", fs::copy_options::overwrite_existing);
    fs::last_write_time(generated() / "Tests" / "Stuff" / "FooTest.cpp", fs::file_time_type::clock::now());
    fs::create_directory(working_materials() / "RebuiltOutput");

    const auto rebuildRun{b.rebuild_run(working_materials() / "RebuiltOutput")};
    std::system(rebuildRun.cmd.c_str());
    check(LINE("Third CMake output existance"), fs::exists(rebuildRun.cmake_output));
    check(LINE("Third build output existance"), fs::exists(rebuildRun.build_output));

    check_equivalence(LINE(""), working_materials() / "RebuiltOutput", predictive_materials() / "RebuiltOutput");

    fs::copy(generated() / "TestMaterials", working_materials() / "OriginalTestMaterials", fs::copy_options::recursive);
    check_equivalence(LINE(""), working_materials() / "OriginalTestMaterials", predictive_materials() / "OriginalTestMaterials");
    fs::create_directory(working_materials() / "RunWithUpdateOutput");

    std::system(b.materials_update(working_materials() / "RunWithUpdateOutput").c_str());
    fs::copy(generated() / "TestMaterials", working_materials() / "UpdatedTestMaterials", fs::copy_options::recursive);
    check_equivalence(LINE(""), working_materials() / "UpdatedTestMaterials", predictive_materials() / "UpdatedTestMaterials");
  }
}
