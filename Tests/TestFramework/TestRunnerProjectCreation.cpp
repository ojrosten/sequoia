////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerProjectCreation.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"
#include "CommandLineArgumentsTestingUtilities.hpp"

#include <cstdlib>

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
  }

  [[nodiscard]]
  std::string_view test_runner_project_creation::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_runner_project_creation::run_tests()
  {
    test_project_creation();
  }

  void test_runner_project_creation::test_project_creation()
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

    // The first argument is set to ensure that the project root is deduced as
    // Sequoia, in order that the aux_files are correctly located

    commandline_arguments args{"../../build/foo/bar", "init", "Oliver Jacob Rosten", generated().string()};

    std::stringstream outputStream{};
    test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, repos, outputStream};

    tr.execute();

    if(std::ofstream file{fake() / "output" / "io.txt"})
    {
      file << outputStream.rdbuf();
    }

    check_equivalence(LINE(""), generated(), predictive_materials() / "GeneratedProject");
    check_equivalence(LINE(""), fake(), predictive_materials() / "FakeProject");

    const auto buildDir{generated() / "build/CMade/TestAll"};
    const auto cmake{
      [&buildDir]() {
        auto cmd{std::string{"cmake -S ."}.append(" -B \"").append(buildDir.string()).append("\"")};

        if constexpr (has_msvc_v)
        {
          cmd.append(" -G \"Visual Studio 16 2019\"");
        }
        else if constexpr (has_clang_v)
        {
          cmd.append("-D CMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++");
        }
        else if constexpr (has_gcc_v)
        {
          cmd.append("-D CMAKE_CXX_COMPILER=/usr/bin/g++");
        }
        else
        {
          static_assert(dependent_false<int>::value, "Unrecognized compiler");
        }

        return cmd.append(" > CMakeOutput.txt");
      }()
    };

    const auto build{
      [&buildDir] () {
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
      }()
    };

    check(LINE("Command processor existance"), std::system(nullptr) > 0);

    std::system((cd(generated() / "TestAll") && cmake && build).c_str());
  }
}
