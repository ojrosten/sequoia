////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerEndToEndFreeTest.hpp"
#include "TestRunner.hpp"

#include "CommandLineArgumentsTestingUtilities.hpp"

namespace sequoia::testing
{
  namespace
  {
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
      auto cmd{cd(buildDir)};
      if constexpr (has_msvc_v)
      {
        cmd && "\"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\MSBuild\\Current\\Bin\\MSbuild.exe\" MyProject.sln";
      }
      else
      {
        cmd && "make";
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
                              " create performance_test Container.hpp");
    }
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
    auto fake{
      [&mat{working_materials()}]() {
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

    if(std::ofstream file{fake() / "output" / "io.txt"})
    {
      file << outputStream.rdbuf();
    }

    check_equivalence(LINE(""), working_materials() / "FakeProject", predictive_materials() / "FakeProject");

    check(LINE("Command processor existance"), std::system(nullptr) > 0);

    const auto mainDir{generated() / "TestAll"};
    const auto buildDir{generated() / "build"/ "CMade" / "TestAll"};

    auto cmake_and_build{
      [mainDir,buildDir](std::string_view cmakeOut, std::string_view buildOut) {
        return    cd(mainDir)
               && add_output_file(cmake_cmd(buildDir), cmakeOut)
               && add_output_file(build_cmd(buildDir), buildOut);
      }
    };

    namespace fs = std::filesystem;

    std::system(cmake_and_build("CMakeOutput.txt", "BuildOutput.txt").c_str());
    check(LINE("First CMake output existance"), fs::exists(mainDir / "CMakeOutput.txt"));
    check(LINE("First build output existance"), fs::exists(buildDir / "BuildOutput.txt"));

    fs::copy(fake() / "Source", generated() / "Source", fs::copy_options::recursive | fs::copy_options::skip_existing);
    fs::create_directory(working_materials() / "Output");

    const auto cmd{cd(buildDir) && add_output_file(create_cmd(), working_materials() / "Output" / "CreationOutput.txt")
                                && cmake_and_build("CMakeOutput2.txt", "BuildOutput2.txt")
                                && add_output_file(run_cmd(), working_materials() / "Output" / "TestRunOutput.txt")};

    std::system(cmd.c_str());

    check(LINE("Second CMake output existance"), fs::exists(mainDir / "CMakeOutput2.txt"));
    check(LINE("Second build output existance"), fs::exists(buildDir / "BuildOutput2.txt"));

    check_equivalence(LINE(""), working_materials() / "Output", predictive_materials() / "Output");
  }
}
