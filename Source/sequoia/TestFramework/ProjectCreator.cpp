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
#include "sequoia/TestFramework/ProjectPaths.hpp"
#include "sequoia/TestFramework/TestRunnerUtilities.hpp"

#include "sequoia/FileSystem/FileSystem.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TextProcessing/Patterns.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

namespace sequoia::testing
{
  using namespace runtime;
  namespace fs = std::filesystem;

  namespace
  {
    constexpr auto npos{std::string::npos};

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

    void set_proj_name(std::string& text, const fs::path& projRoot)
    {
      if(projRoot.empty()) return;

      const auto name{(--projRoot.end())->generic_string()};
      const std::string myProj{"MyProject"}, projName{replace_all(name, " ", "_")};
      replace_all(text, myProj, projName);
    }
  }

  void check_indent(const indentation& ind)
  {
    if(std::string_view{ind}.find_first_not_of("\t ") != npos)
      throw std::runtime_error{"Code indent must comprise only spaces or tabs"};
  }

  void generate_test_main(std::string_view copyright, const fs::path& newProjRoot, indentation codeIndent)
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

    read_modify_write(newProjRoot / main_paths::default_main_cpp_from_root(), modifier);
  }

  void generate_build_system_files(const fs::path& parentProjRoot, const fs::path& newProjRoot)
  {
    if(newProjRoot.empty())
      throw std::logic_error{"Pre-condition violated: path should not be empty"};

    const fs::path relCmakeLocation{main_paths::default_cmake_from_root()};

    auto setBuildSysPath{
      [&parentProjRoot,&newProjRoot](std::string& text) {
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
              text.replace(left, count, relPath.generic_string());
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

    read_modify_write(source_paths::cmake_lists(newProjRoot), [setBuildSysPath, &newProjRoot](std::string& text) {
        setBuildSysPath(text);
        set_proj_name(text, newProjRoot);
      }
    );

    read_modify_write(auxiliary_paths::project_template(newProjRoot) / relCmakeLocation, [setBuildSysPath](std::string& text) {
        setBuildSysPath(text);
      }
    );
  }

  void init_projects(const project_paths& parentProjectPaths, const std::vector<project_data>& projects, std::ostream& stream)
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

      const auto& parentProjRoot{parentProjectPaths.project_root()};
      fs::create_directories(data.project_root);
      fs::copy(parentProjectPaths.aux_paths().project_template(), data.project_root, fs::copy_options::recursive | fs::copy_options::skip_existing);
      fs::create_directory(source_paths(data.project_root).project());
      fs::copy(parentProjectPaths.aux_paths().repo(), auxiliary_paths::repo(data.project_root), fs::copy_options::recursive | fs::copy_options::skip_existing);

      generate_test_main(data.copyright, data.project_root, data.code_indent);
      generate_build_system_files(parentProjRoot, data.project_root);

      if(data.do_build != build_invocation::no)
      {
        const main_paths main{data.project_root / main_paths::default_main_cpp_from_root()};
        const build_paths build{data.project_root, main};

        invoke(cd_cmd(main.dir())
            && cmake_cmd(parentProjectPaths.build(), build, data.output)
            && build_cmd(build, data.output)
            && git_first_cmd(data.project_root, data.output)
            && (data.do_build == build_invocation::launch_ide ? launch_cmd(parentProjectPaths, data.project_root, build.cmade_dir()) : shell_command{})
        );
      }
    }
  }

  [[nodiscard]]
  shell_command git_first_cmd(const fs::path& root, const fs::path& output)
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
  shell_command launch_cmd(const project_paths& parentProjectPaths, const fs::path& root, const fs::path& buildDir)
  {
    if(root.empty()) return {};

    if constexpr(with_msvc_v)
    {
      const auto cmakeCache{parentProjectPaths.build().cmake_cache()};

      if(const auto optText{read_to_string(cmakeCache)})
      {
        const auto [first, last]{find_sandwiched_text(optText.value(), "CMAKE_GENERATOR_INSTANCE:INTERNAL=", "\n")};
        if((first != npos) && (last != npos))
        {
          const auto devenv{fs::path(optText->substr(first, last - first)).append("Common7/IDE/devenv.exe")};
          if(fs::exists(devenv))
          {
            const auto token{back(root)};
            const auto sln{(buildDir / token).concat("Tests.sln")};

            return {"Attempting to open IDE...", std::string{"\""}.append(devenv.string()).append("\" ").append("/Run ").append(sln.string()), ""};
          }
        }
      }
    }

    return {};
  }
}