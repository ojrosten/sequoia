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

      return cmd.append(" > CMakeOutput.txt");
    }

    [[nodiscard]]
    std::string build_cmd(const std::filesystem::path& buildDir)
    {
      auto cmd{cd(buildDir.string())};
      if constexpr (has_msvc_v)
      {
        cmd && "\"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\MSBuild\\Current\\Bin\\MSbuild.exe\" MyProject.sln";
      }
      else
      {
        cmd && "make";
      }

      return cmd.append("> BuildOutput.txt");
    }

    [[nodiscard]]
    std::string create_cmd()
    {
      std::string cmd{
        []() {
          if constexpr (has_msvc_v)
          {
#ifdef CMAKE_INTDIR
            return std::string{CMAKE_INTDIR}.append("\\TestMain.exe");
#elif
            throw std::logic_error{"Unable to find preprocessor definition for CMAKE_INTDIR"};
#endif
          }
          else
          {
            return "./TestMain";
          }
        }()
      };

      return cmd.append(" > CreateOutput.txt");
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
        return mat / "GeneratedProject";
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

    check_equivalence(LINE(""), generated(), predictive_materials() / "GeneratedProject");
    check_equivalence(LINE(""), fake(), predictive_materials() / "FakeProject");

    check(LINE("Command processor existance"), std::system(nullptr) > 0);

    const auto buildDir{generated() / "build/CMade/TestAll"};
    std::system(     (cd(generated() / "TestAll")
                  && cmake_cmd(buildDir)
                  && build_cmd(buildDir)
                  && create_cmd()).c_str());
  }
}
