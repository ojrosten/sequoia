////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for ShellCommands.hpp
*/

#include "sequoia/Runtime/ShellCommands.hpp"

#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TextProcessing/Patterns.hpp"

#include <iostream>

namespace sequoia::runtime
{
  namespace fs = std::filesystem;

  namespace
  {
    [[nodiscard]]
    std::string cmake_extractor(const std::optional<std::filesystem::path>& parentBuildDir,
                                const std::filesystem::path& buildDir,
                                std::string_view pattern)
    {
      const auto cacheDir{parentBuildDir.has_value() ? parentBuildDir.value() : buildDir};

      const fs::path cmakeCache{cacheDir / "CMakeCache.txt"};
      if(!fs::exists(cmakeCache))
        throw std::runtime_error{"Unable to find CMakeCache.txt in " + cmakeCache.generic_string()};

      if(const auto optText{read_to_string(cmakeCache)})
      {
        constexpr auto npos{std::string::npos};
        const auto& text{optText.value()};
        const auto positions{find_sandwiched_text(text, pattern, "\n")};
        if((positions.first == npos) || (positions.second == npos))
          throw std::runtime_error{"Unable to determine Visual Studio version from " + cmakeCache.generic_string()};

        const auto generator{text.substr(positions.first, positions.second - positions.first)};
        return std::string{"-G \""}.append(generator).append("\"");
      }
      else
      {
        throw std::runtime_error{"Unable to read from " + cmakeCache.generic_string()};
      }
    }
  }

  shell_command::shell_command(std::string cmd, const std::filesystem::path& output, append_mode app)
    : m_Command{std::move(cmd)}
  {
    if(!output.empty())
    {
      if(!m_Command.empty() && std::isdigit(m_Command.back()))
        m_Command.append(" ");

      m_Command.append(app == append_mode::no ? "> " : ">> ");
      m_Command.append(output.string()).append(" 2>&1");
    }
  }

  shell_command::shell_command(std::string_view preamble, std::string cmd, const std::filesystem::path& output, append_mode app)
  {
    const auto pre{
      [&]() -> shell_command {
        if(!preamble.empty())
        {
          const std::string newline{with_msvc_v ? "echo/" : "echo"};
          return shell_command{newline, output, app}
              && shell_command{std::string{"echo "}.append(preamble), output, append_mode::yes}
              && shell_command{newline, output, append_mode::yes};
        }

        return {};
      }()
    };

    *this = pre && shell_command{std::move(cmd), output, !pre.empty() ? append_mode::yes : app};
  }

  [[nodiscard]]
  shell_command operator&&(const shell_command& lhs, const shell_command& rhs)
  {
    return rhs.empty() ? lhs :
           lhs.empty() ? rhs :
                         std::string{lhs.m_Command}.append("&&").append(rhs.m_Command);
  }

  void invoke(const shell_command& cmd)
  {
    std::cout << std::flush;
    if(cmd.m_Command.data()) std::system(cmd.m_Command.data());
  }

  [[nodiscard]]
  shell_command cd_cmd(const std::filesystem::path& dir)
  {
    return std::string{"cd "}.append(dir.string());
  }

  [[nodiscard]]
  shell_command cmake_cmd(const std::optional<std::filesystem::path>& parentBuildDir,
                          const std::filesystem::path& buildDir,
                          const std::filesystem::path& output)
  {
    auto cmd{std::string{"cmake -S ."}.append(" -B \"").append(buildDir.string()).append("\" ")};

    if constexpr(with_msvc_v)
    {
      cmd.append(cmake_extractor(parentBuildDir, buildDir, "CMAKE_GENERATOR:INTERNAL="));
    }
    else if constexpr(with_clang_v)
    {
      cmd.append("-D CMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++");
    }
    else if constexpr(with_gcc_v)
    {
      cmd.append("-D CMAKE_CXX_COMPILER=/usr/local/Cellar/gcc/11.2.0_3/bin/g++-11");
    }

    return {"Running CMake...", cmd, output};
  }

  [[nodiscard]]
  shell_command build_cmd(const std::filesystem::path& buildDir, const std::filesystem::path& output)
  {
    const auto cmd{
      [&output]() -> shell_command {
        std::string str{"cmake --build . --target TestAll"};
        if constexpr(with_msvc_v)
        {
#ifdef CMAKE_INTDIR
          str.append(" --config ").append(std::string{CMAKE_INTDIR});
#else
          std::cerr << parsing::commandline::warning("Unable to find preprocessor definition for CMAKE_INTDIR");
#endif
        }
        else
        {
          str.append(" -- -j4");
        }

        return {"Building...", str, output};
      }()
    };

    return cd_cmd(buildDir) && cmd;
  }
}
