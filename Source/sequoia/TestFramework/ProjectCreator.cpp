////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for ProjectCreator.hpp
 */

#include "sequoia/TestFramework/ProjectCreator.hpp"
#include "sequoia/TestFramework/FileSystem.hpp"
#include "sequoia/TestFramework/TestRunnerUtilities.hpp"

#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

namespace sequoia::testing
{
  using namespace runtime;
  namespace fs = std::filesystem;

  namespace
  {
    [[nodiscard]]
    bool is_appropriate_root(const fs::path& root)
    {
      if(fs::exists(root))
      {
        for(auto& entry : fs::directory_iterator(root))
        {
          const auto& f{entry.path().filename()};
          if((f != ".DS_Store") && (f != ".keep")) return false;
        }
      }

      return true;
    }

    void set_proj_name(std::string& text, const std::filesystem::path& projRoot)
    {
      if(projRoot.empty()) return;

      const auto name{(--projRoot.end())->generic_string()};
      const std::string myProj{"MyProject"}, projName{replace_all(name, " ", "_")};
      replace_all(text, myProj, projName);
    }
  }

  void check_indent(const indentation& ind)
  {
    if(std::string_view{ind}.find_first_not_of("\t ") != std::string::npos)
      throw std::runtime_error{"Code indent must comprise only spaces or tabs"};
  }

  void generate_test_main(std::string_view copyright, const std::filesystem::path& newProjRoot, indentation codeIndent)
  {
    auto modifier{
      [copyright, codeIndent](std::string& text) {

        set_top_copyright(text, copyright);

        tabs_to_spacing(text, codeIndent);
        replace(text, "Oliver J. Rosten", copyright);

        const auto indentReplacement{
          [&codeIndent]() {
            std::string replacement;
            for(auto c : std::string_view{codeIndent})
            {
              if(c == '\t') replacement.append("\\t");
              else          replacement.push_back(c);
            }

            return indentation{replacement};
          }
        };

        replace(text, "\\t", indentReplacement());
      }
    };

    read_modify_write(newProjRoot / "TestAll" / "TestAllMain.cpp", modifier);
  }

  void generate_build_system_files(const std::filesystem::path& parentProjRoot, const std::filesystem::path& newProjRoot)
  {
    if(newProjRoot.empty())
      throw std::logic_error{"Pre-condition violated: path should not be empty"};

    const std::filesystem::path relCmakeLocation{"TestAll/CMakeLists.txt"};

    auto setBuildSysPath{
      [&parentProjRoot,&newProjRoot](std::string& text) {
        constexpr auto npos{std::string::npos};
        std::string_view token{"/build_system"};
        if(const auto pos{text.find(token)}; pos != npos)
        {
          std::string_view pattern{"BuildSystem "};
          if(auto left{text.rfind(pattern)}; left != npos)
          {
            left += pattern.size();
            if(pos >= left)
            {
              const auto count{pos - left + token.size()};
              const auto relPath{fs::relative(parentProjRoot / "TestAll" / text.substr(left, count), newProjRoot / "TestAll")};
              text.replace(left, count, relPath.lexically_normal().generic_string());
            }
          }
        }
      }
    };

    read_modify_write(newProjRoot / relCmakeLocation, [setBuildSysPath, &newProjRoot](std::string& text) {
        setBuildSysPath(text);
        set_proj_name(text, newProjRoot);
      }
    );

    read_modify_write(newProjRoot / "Source" / "CMakeLists.txt", [setBuildSysPath, &newProjRoot](std::string& text) {
        setBuildSysPath(text);
        set_proj_name(text, newProjRoot);
      }
    );

    read_modify_write(project_template_path(newProjRoot) / relCmakeLocation, [setBuildSysPath](std::string& text) {
        setBuildSysPath(text);
      }
    );
  }

  void init_projects(const std::filesystem::path& parentProjRoot, const std::vector<project_data>& projects, std::ostream& stream)
  {
    stream << "Initializing Project(s)....\n\n";

    for(const auto& data : projects)
    {
      if(data.project_root.empty())
        throw std::runtime_error{"Project path should not be empty\n"};

      if(!data.project_root.is_absolute())
        throw std::runtime_error{std::string{"Project path '"}.append(data.project_root.generic_string()).append("' should be absolute\n")};

      if(!is_appropriate_root(data.project_root))
        throw std::runtime_error{std::string{"Project location '"}.append(data.project_root.generic_string()).append("' is in use\n")};

      const auto name{(--data.project_root.end())->generic_string()};
      if(name.empty())
        throw std::runtime_error{"Project name, deduced as the last token of path, is empty\n"};

      if(std::find_if(name.cbegin(), name.cend(), [](char c) { return !std::isalnum(c) || (c == '_') || (c == '-'); }) != name.cend())
      {
        throw std::runtime_error{std::string{"Please ensure the project name '"}
          .append(name)
          .append("' consists of just alpha-numeric characters, underscores and dashes\n")};
      }

      check_indent(data.code_indent);

      report(stream, "Creating new project at location:", data.project_root.generic_string());

      fs::create_directories(data.project_root);
      fs::copy(project_template_path(parentProjRoot), data.project_root, fs::copy_options::recursive | fs::copy_options::skip_existing);
      fs::create_directory(project_paths::source_path(data.project_root));
      fs::copy(aux_files_path(parentProjRoot), aux_files_path(data.project_root), fs::copy_options::recursive | fs::copy_options::skip_existing);

      generate_test_main(data.copyright, data.project_root, data.code_indent);
      generate_build_system_files(parentProjRoot, data.project_root);

      if(data.do_build != build_invocation::no)
      {
        const auto mainDir{data.project_root / "TestAll"};
        const auto buildDir{project_paths::cmade_build_dir(data.project_root, mainDir)};

        using namespace runtime;
        invoke(cd_cmd(mainDir)
            && cmake_cmd(working_path_v, buildDir, data.output)
            && build_cmd(buildDir, data.output)
            && git_first_cmd(data.project_root, data.output)
            && (data.do_build == build_invocation::launch_ide ? launch_cmd(data.project_root, buildDir) : shell_command{})
        );
      }
    }
  }

  [[nodiscard]]
  shell_command git_first_cmd(const std::filesystem::path& root, const std::filesystem::path& output)
  {
    if(!output.empty())
    {
      read_modify_write(root / ".gitignore", [&](std::string& text) { text.append("\n\n").append(output.filename().string()); });
    }

    using app_mode = shell_command::append_mode;
    return cd_cmd(root)
        && shell_command{"Placing under version control...", "git init -b trunk", output}
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