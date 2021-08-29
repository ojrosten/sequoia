////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/ProjectCreator.hpp"

#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/Streaming/Streaming.hpp"

namespace sequoia::testing
{
  using namespace runtime;
  namespace fs = std::filesystem;

  [[nodiscard]]
  shell_command git_first_cmd(const std::filesystem::path& root, const std::filesystem::path& output)
  {
    if(!output.empty())
    {
      read_modify_write(root / ".gitignore", [&](std::string& text) { text.append("\n\n").append(output.filename().string()); });
    }

    using app_mode = shell_command::append_mode;
    return cd_cmd(root)
      && shell_command {
      "Placing under version control...", "git init -b trunk", output
    }
    && shell_command{"", "git add . ", output, app_mode::yes}
    && shell_command{"", "git commit -m \"First commit\"", output, app_mode::yes};
  }

  [[nodiscard]]
  shell_command launch_cmd(const std::filesystem::path& root, const std::filesystem::path& buildDir)
  {
    if(root.empty()) return {};

    if constexpr(with_msvc_v)
    {
      const auto vs2019Dir{
        []() -> std::filesystem::path {
          const fs::path vs2019Path{"C:/Program Files (x86)/Microsoft Visual Studio/2019"};
          if(const auto enterprise{vs2019Path / "Enterprise"}; fs::exists(enterprise))
          {
            return enterprise;
          }
          else if(const auto pro{vs2019Path / "Professional"}; fs::exists(pro))
          {
            return pro;
          }
          else if(const auto community{vs2019Path / "Community"}; fs::exists(community))
          {
            return community;
          }

          return "";
        }()
      };

      if(!vs2019Dir.empty())
      {
        const auto devenv{vs2019Dir / "Common7" / "IDE" / "devenv"};

        const auto token{*(--root.end())};
        const auto sln{(buildDir / token).concat("Tests.sln")};

        return {"Attempting to open IDE...", std::string{"\""}.append(devenv.string()).append("\" ").append("/Run ").append(sln.string()), ""};
      }
    }

    return {};
  }
}