////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for TestRunner.hpp.
*/

#include "sequoia/TestFramework/TestRunner.hpp"
#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/TestFramework/Summary.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#ifdef _MSC_VER
  #include <format>
#endif 

namespace sequoia::testing
{
  namespace
  {
    std::string& to_spaces(std::string& text, std::string_view spacing)
    {
      if(spacing != "\t")
      {
        constexpr auto npos{std::string::npos};
        std::string::size_type tabPos{};
        while((tabPos = text.find('\t', tabPos)) != npos)
        {
          text.replace(tabPos, 1, spacing);
          tabPos += spacing.size();
        }
      }

      return text;
    }

    void process_namespace(std::string& text, std::string_view nameSpace)
    {
      if(nameSpace.empty())
      {
        replace_all(text, {{"namespace\n", ""}, {"?{\n", ""}, {"?}\n", ""}});
        constexpr auto npos{std::string::npos};
        std::string::size_type endLine{};
        while((endLine = text.find('\n', endLine)) != npos)
        {
          if((++endLine < text.size()) && (text[endLine] == '\t'))
          {
            text.erase(endLine, 1);
          }
        }
      }
      else
      {
        replace_all(text, {{"namespace", std::string{"namespace "}.append(nameSpace)}, {"?{", "{"}, {"?}", "}"}});
      }
    }

    void set_proj_name(std::string& text, const std::filesystem::path& projRoot)
    {
      if(projRoot.empty()) return;

      const auto name{(--projRoot.end())->generic_string()};
      const std::string myProj{"MyProject"}, projName{replace_all(name, " ", "_")};
      replace_all(text, myProj, projName);
    }

    void check_indent(const indentation& ind)
    {
      if(std::string_view{ind}.find_first_not_of("\t ") != std::string::npos)
        throw std::runtime_error{"Code indent must comprise only spaces or tabs"};
    }
  }

  void invoke(const shell_command& cmd)
  {
    std::cout << std::flush;
    if(cmd.m_Command.data()) std::system(cmd.m_Command.data());
  }

  shell_command::shell_command(std::string cmd, const std::filesystem::path& output, append_mode app)
    : m_Command{std::move(cmd)}
  {
    if(!output.empty())
    {
      m_Command.append(app == append_mode::no ? " > " : " >> ");
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
  shell_command cd_cmd(const std::filesystem::path& dir)
  {
    return std::string{"cd "}.append(dir.string());
  }

  [[nodiscard]]
  shell_command cmake_cmd(const std::filesystem::path& buildDir, const std::filesystem::path& output)
  {
    auto cmd{std::string{"cmake -S ."}.append(" -B \"").append(buildDir.string()).append("\" ")};

    if constexpr(with_msvc_v)
    {
      cmd.append("-G \"Visual Studio 16 2019\"");
    }
    else if constexpr(with_clang_v)
    {
      cmd.append("-D CMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++");
    }
    else if constexpr(with_gcc_v)
    {
      cmd.append("-D CMAKE_CXX_COMPILER=/usr/bin/g++");
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

  [[nodiscard]]
  shell_command git_first_cmd(const std::filesystem::path& root, const std::filesystem::path& output)
  {
    if(!output.empty())
    {
      namespace fs = std::filesystem;
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
      namespace fs = std::filesystem;

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

  [[nodiscard]]
  bool handle_as_ref(std::string_view type)
  {
    if(type.empty())
      throw std::logic_error{"Equivalent type is unspecified"};

    const auto startPos{type.find_first_not_of(' ')};
    if(startPos == std::string::npos)
      throw std::logic_error{"Equivalent type is unspecified"};

    if((type.back() == '*') || (type.back() == '&')) return false;

    const auto endPos{type.find_first_of(' ', startPos)};
    auto token{std::string_view{type}.substr(startPos, endPos - startPos)};

    constexpr std::array<std::string_view, 9> funTypes{"int", "float", "double", "bool", "char", "short", "long", "signed", "unsigned"};
    for(auto t : funTypes)
    {
      if(const auto pos{token.find(t)}; pos != std::string::npos)
      {
        if(token.size() == t.size()) return false;

        return (t.size() < token.size()) && (token[t.size()] != ' ');
      }
    }

    constexpr std::array<std::string_view, 10> types{"std::size_t", "size_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "int8_t", "int16_t", "int32_t", "int64_t"};
    for(auto t : types)
    {
      if(type == t) return false;
    }

    return true;
  }

  [[nodiscard]]
  std::string report_time(const test_family::summary& s)
  {
    return report_time(s.log, s.execution_time);
  }

  [[nodiscard]]
  std::string to_string(const template_data& data)
  {
    if(data.empty()) return "";

    std::string str{"<"};
    for(const auto& d : data)
    {
      str.append(d.species).append(" ").append(d.symbol).append(", ");
    }
    str.replace(str.size() - 2, 2, ">");

    return str;
  }

  [[nodiscard]]
  template_spec generate_template_spec(std::string_view str)
  {
    constexpr auto npos{std::string::npos};
    const auto endOfLastToken{str.find_last_not_of(" .")};
    if(endOfLastToken == npos) return {};

    auto mess{
      [str](std::string_view details){
        return std::string{"<"}.append(str).append(">: ").append(details);
      }
    };

    const auto beforeLastToken{str.substr(0, endOfLastToken).rfind(' ')};
    if(beforeLastToken == npos)
      throw std::runtime_error{mess(" Unable to locate species/symbol pair")};

    const auto lastTokenSize{endOfLastToken - beforeLastToken};
    const auto endOfLastTemplateSpec{str.substr(0, endOfLastToken + 1 - lastTokenSize).find_last_not_of(" ")};
    if(endOfLastTemplateSpec == npos)
      throw std::runtime_error{mess(" Unable to locate species/symbol pair")};

    const auto first{
      [str, end{endOfLastTemplateSpec}](){
        auto pos{str.substr(0, end).rfind(' ')};
        if((pos != npos) && (str.size() > pos + 1) && (str[pos + 1] == '.'))
          pos = str.substr(0, pos).rfind(' ');

        return pos;
      }()
    };

    std::string::size_type pos{first == npos ? 0 : first + 1};

    return {std::string{str.substr(pos, endOfLastTemplateSpec + 1 - pos)}, std::string{str.substr(beforeLastToken + 1, lastTokenSize)}};
  }

  [[nodiscard]]
  template_data generate_template_data(std::string_view str)
  {
    std::vector<template_spec> decomposition{};

    constexpr auto npos{std::string::npos};
    if(auto openPos{str.find('<')}; openPos != npos)
    {
      if(auto closePos{str.rfind('>')}; closePos != npos)
      {
        if(closePos < openPos)
          throw std::runtime_error{std::string{str}.append(": unable to parse template")};

        auto start{openPos + 1};
        auto next{npos};
        while((next = str.find(',', start)) != npos)
        {
          std::string_view v{&str.data()[start], next - start};

          decomposition.push_back(generate_template_spec(v));

          start = next + 1;
        }

        std::string_view v{&str.data()[start], closePos - start};
        decomposition.push_back(generate_template_spec(v));
      }
      else
      {
        throw std::runtime_error{std::string{str}.append(": < not matched by >")};
      }
    }

    return decomposition;
  }

  void set_top_copyright(std::string& text, std::string_view copyright)
  {
    constexpr auto npos{std::string::npos};
    if(auto copyrightPos{text.find("Copyright")}; copyrightPos != npos)
    {
      auto left{text.rfind("//", copyrightPos)};
      auto right{text.find("//", copyrightPos)};
      if((left == npos) || (right == npos))
      {
        throw std::runtime_error("Unable to find boundaries of copyright message");
      }

      // TO DO: prefer the MSC code once supported by other compilers
      const auto year{
        []() -> std::string {
#ifdef _MSC_VER
          using namespace std::chrono;
          return std::format("{}", year_month_day{floor<days>(system_clock::now())}.year());
#else
          const auto now{std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())};
          return std::to_string(1900 + std::localtime(&now)->tm_year);
#endif
        }()
      };

      const auto newCopyright{std::string{"Copyright "}.append(copyright).append(" ").append(year).append(".")};

      const auto reservedSpace{right - left - 2};
      const auto requiredSpace{newCopyright.size()};
      const auto remainingSpace{reservedSpace > requiredSpace ? reservedSpace - requiredSpace : std::string::size_type{}};
      const auto rightSpace(remainingSpace / 2);
      const auto leftSpace(remainingSpace - rightSpace);

      const auto replacement{std::string(leftSpace, ' ').append(newCopyright).append(std::string(rightSpace, ' '))};

      text.replace(left + 2, reservedSpace, replacement);
    }
    else
    {
      throw std::runtime_error{"Unable to locate Copyright information"};
    }
  }

  //=========================================== nascent_test_base ===========================================//

  void nascent_test_base::camel_name(std::string name) { m_CamelName = to_camel_case(std::move(name)); }

  [[nodiscard]]
  std::filesystem::path nascent_test_base::build_source_path(const std::filesystem::path& filename,
    const std::vector<std::string_view>& extensions) const
  {
    if(filename.empty())
      throw std::runtime_error{"Header name is empty"};

    if(const auto path{find_in_tree(m_Paths.source(), filename)}; !path.empty())
      return path;

    for(auto e : extensions)
    {
      if(e != filename.extension())
      {
        const auto alternative{std::filesystem::path{filename}.replace_extension(e)};
        if(const auto path{find_in_tree(m_Paths.source(), alternative)}; !path.empty())
          return path;
      }
    }

    return {};
  }

  template<invocable_r<std::filesystem::path, std::filesystem::path> WhenAbsent>
  void nascent_test_base::finalize(WhenAbsent fn)
  {
    finalize_family();

    const auto srcPath{[this, fn]() {
        const std::vector<std::string_view> extensions{".hpp", ".h"};
        auto pth{build_source_path(m_Header, extensions)};
        if(pth.empty() && (m_SourceOption == gen_source_option::yes))
          pth = fn(m_Header);

        if(pth.empty())
          on_source_path_error(extensions);

        return pth;
      }()
    };

    finalize_header(srcPath);
  }

  void nascent_test_base::finalize_family()
  {
    if(m_Family.empty())
    {
      m_Family = m_CamelName;
      replace_all(m_Family, "_", " ");
    }
  }

  void nascent_test_base::finalize_header(const std::filesystem::path& sourcePath)
  {
    namespace fs = std::filesystem;

    const auto relSourcePath{fs::relative(sourcePath, m_Paths.source())};
    const auto dir{(m_Paths.tests() / relSourcePath).parent_path()};
    fs::create_directories(dir);

    m_HostDir = dir;
    m_HeaderPath = fs::relative(sourcePath, m_Paths.source_root());
  }

  void nascent_test_base::on_source_path_error(const std::vector<std::string_view>& extensions) const
  {
    namespace fs = std::filesystem;

    auto mess{std::string{"Unable to locate file "}.append(m_Header.generic_string()).append(" or ")};
    for(auto e : extensions)
    {
      if(e != m_Header.extension())
      {
        const auto alternative{fs::path{m_Header}.replace_extension(e)};
        mess.append(alternative.generic_string());
      }
    }

    mess.append(" in the source repository\n").append(fs::relative(m_Paths.source(), m_Paths.tests()).generic_string());

    throw std::runtime_error{mess};
  }

  template<invocable<std::string&> FileTransformer>
  [[nodiscard]]
  auto nascent_test_base::create_file(const std::filesystem::path& codeTemplatesDir, std::string_view copyright, std::string_view inputNameStub, std::string_view nameEnding, FileTransformer transformer) const -> file_data
  {
    namespace fs = std::filesystem;

    const auto outputFile{(host_dir() / camel_name()) += nameEnding};

    if(fs::exists(outputFile))
    {
      return {outputFile, false};
    }

    const auto inputFile{(codeTemplatesDir / inputNameStub).concat(nameEnding)};

    fs::copy_file(inputFile, outputFile, fs::copy_options::overwrite_existing);
    if(std::string text{read_to_string(outputFile)}; !text.empty())
    {
      set_top_copyright(text, copyright);
      transformer(text);

      write_to_file(outputFile, text);
    }

    return {outputFile, true};
  }

  void nascent_test_base::set_cpp(const std::filesystem::path& headerPath, std::string_view copyright, std::string_view nameSpace) const
  {
    namespace fs = std::filesystem;
    const auto srcPath{fs::path{headerPath}.replace_extension("cpp")};

    const auto sourceRoot{paths().source_root()};
    fs::copy_file(source_templates_path(paths().project_root()) / "MyCpp.cpp", srcPath);

    auto setCppText{
        [&](std::string& text) {
          set_top_copyright(text, copyright);
          process_namespace(text, nameSpace);
          replace_all(text, "?.hpp", rebase_from(headerPath, sourceRoot).generic_string());
          to_spaces(text, code_indent());
        }
    };

    read_modify_write(srcPath, setCppText);

    add_to_cmake(sourceRoot, sourceRoot, srcPath, "set(", ")\n", "");

    read_modify_write(paths().main_cpp_dir() / "CMakeLists.txt", [&root=paths().project_root()](std::string& text) {
        replace_all(text, "#!", "");
      }
    );
  }

  //=========================================== nascent_semantics_test ===========================================//

  void nascent_semantics_test::finalize(std::string_view copyright)
  {
    constexpr auto npos{std::string::npos};

    auto start{npos};
    auto templatePos{m_QualifiedName.find('<')};
    std::string nameSpace{};

    if(auto pos{m_QualifiedName.rfind("::", templatePos)}; pos != npos)
    {
      if(pos < m_QualifiedName.length() - 2)
      {
        start = pos+2;
        nameSpace = m_QualifiedName.substr(0, pos);
        forename(m_QualifiedName.substr(start));
      }
    }
    else
    {
      forename(m_QualifiedName);
      start = 0;
    }

    m_TemplateData = generate_template_data(forename());
    if(!m_TemplateData.empty())
    {
      if(auto pos{forename().find('<')}; pos != npos)
      {
        forename(std::string{forename()}.erase(pos));

        if(start != npos)
        {
          m_QualifiedName.erase(start + pos);

          std::string args{"<"};
          std::for_each(m_TemplateData.cbegin(), m_TemplateData.cend(),
            [&args](const template_spec& d) {
              args.append(d.symbol);
              if(!d.species.empty() && (d.species.back() == '.'))
                args.append("...");

              args.append(", ");
            }
          );

          args.erase(args.size() - 1);
          args.back() = '>';

          m_QualifiedName.append(args);
        }
      }
    }

    camel_name(forename());
    if(header().empty()) header(std::filesystem::path{camel_name()}.concat(".hpp"));

    namespace fs = std::filesystem;
    nascent_test_base::finalize([&nameSpace,this,copyright](const fs::path& filename) {

      const auto headerTemplate{std::string{"My"}.append(capitalize(to_camel_case(test_type()))).append("Class.hpp")};

      const auto headerPath{filename.is_absolute() ? filename : paths().source() / rebase_from(m_SourceDir / filename, paths().source())};
      fs::create_directories(headerPath.parent_path());
      fs::copy_file(source_templates_path(paths().project_root()) / headerTemplate, headerPath);

      read_modify_write(headerPath, [this, &nameSpace, copyright](std::string& text) { set_header_text(text, copyright, nameSpace); });

      if(m_TemplateData.empty())
      {
        set_cpp(headerPath, copyright, nameSpace);
      }

      return headerPath;
    });
  }

  [[nodiscard]]
  auto nascent_semantics_test::create_file(std::string_view copyright, const std::filesystem::path& codeTemplatesDir, std::string_view nameEnding) const -> file_data
  {
    auto transformer{[this](std::string& text) { transform_file(text); }};
    return nascent_test_base::create_file(codeTemplatesDir, copyright, "MyClass", nameEnding, transformer);
  }

  [[nodiscard]]
  std::vector<std::string> nascent_semantics_test::constructors() const
  {
    return { {std::string{forename()}.append("_false_positive_test(\"False Positive Test\")")},
             {std::string{forename()}.append("_test(\"Unit Test\")")} };
  }

  void nascent_semantics_test::transform_file(std::string& text) const
  {
    constexpr auto npos{std::string::npos};
    constexpr std::string_view regPattern{"$Regular"}, movPattern{"$Move"}, endPattern{"$\n"};
    if(auto start{text.find(regPattern)}; start != npos)
    {
      if(auto middle{text.find(movPattern, start + regPattern.size())}; middle != npos)
      {
        if(auto end{text.find(endPattern, middle + movPattern.size())}; end != npos)
        {
          if(test_type() == "regular")
          {
            text.erase(middle, end + endPattern.size() - middle);
            text.erase(start, regPattern.size());
          }
          else
          {
            text.erase(end, endPattern.size());
            text.erase(start, middle + movPattern.size() - start);
          }
        }
      }
    }

    to_spaces(text, code_indent());

    if(!m_EquivalentTypes.empty())
    {
      const auto num{m_EquivalentTypes.size()};
      const auto prediction{
        [num](const std::size_t i, std::string_view sep) {
          std::string p{"prediction"};
          if(num > 1) p.append("_").append(std::to_string(i));
          if((i < num - 1) && !sep.empty()) p.append(sep).append(" ");
          return p;
        }
      };

      std::string args{};
      for(std::size_t i{}; i < num; ++i)
      {
        const auto& type{m_EquivalentTypes[i]};
        if(!type.empty())
        {
          constexpr std::string_view pattern{"const "};
          if(std::string_view{type}.substr(0, pattern.size()) != pattern)
          {
            args.append("const ");
          }

          args.append(type);

          if(handle_as_ref(type)) args.append("&");
          args.append(" ");

          args.append(prediction(i, ","));
        }
      }

      replace_all(text, "?args", args);
      replace_all(text, "?predictions", prediction(0, ""));
    }
    else
    {
      const auto start{text.rfind("template<?>")};
      const auto finish{text.rfind("};")};
      if((start != npos) && (finish != npos))
      {
        text.erase(start, finish + 2 - start);
      }
    }

    if(!m_TemplateData.empty())
    {
      replace_all(text, "<?> ", to_string(m_TemplateData).append("\n").append(code_indent()));
    }
    else
    {
      replace_all(text, "<?>", "<>");
    }

    namespace fs = std::filesystem;
    replace_all(text, {{"::?_class", m_QualifiedName},
                       {"?_class", forename()},
                       {"?Class.hpp", header_path().generic_string()},
                       {"?Class", camel_name()},
                       {"?Test", to_camel_case(test_type()).append("Test")},
                       {"?", test_type()}});
  }

  void nascent_semantics_test::set_header_text(std::string& text, std::string_view copyright, std::string_view nameSpace) const
  {
    set_top_copyright(text, copyright);
    process_namespace(text, nameSpace);
    replace_all(text, "?type", forename());
    if(m_TemplateData.empty())
    {
      replace_all(text, "\ttemplate<?>\n", "");
    }
    else
    {
      const auto templateSpec{std::string{"template"}.append(to_string(m_TemplateData))};
      replace_all(text, "template<?>", templateSpec);
    }

    to_spaces(text, code_indent());
  }

  //=========================================== nascent_behavioural_test ===========================================//

  void nascent_behavioural_test::finalize(std::string_view copyright)
  {
    camel_name(forename().empty() ? header().filename().replace_extension().string() : forename());

    namespace fs = std::filesystem;
    nascent_test_base::finalize([this,copyright](const fs::path& filename) {

      const auto headerPath{filename.is_absolute() ? filename : paths().source() / rebase_from(filename, paths().source())};
      fs::create_directories(headerPath.parent_path());
      fs::copy_file(source_templates_path(paths().project_root()) / "MyFreeFunctions.hpp", headerPath);

      read_modify_write(headerPath, [&nameSpace=m_Namespace](std::string& text) { process_namespace(text, nameSpace); });

      set_cpp(headerPath, copyright, m_Namespace);

      return headerPath;
    });

    camel_name(std::string{camel_name()}.append(capitalize(test_type())));

    if(forename().empty())
      forename(to_snake_case(camel_name()));
  }

  [[nodiscard]]
  auto nascent_behavioural_test::create_file(std::string_view copyright, const std::filesystem::path& codeTemplatesDir, std::string_view nameEnding) const -> file_data
  {
    auto transformer{[this](std::string& text) { transform_file(text); }};
    return nascent_test_base::create_file(codeTemplatesDir, copyright, "MyBehavioural", nameEnding, transformer);
  }

  [[nodiscard]]
  std::vector<std::string> nascent_behavioural_test::constructors() const
  {
    return { {std::string{forename()}.append("_test(\"").append(to_camel_case(test_type())).append(" Test\")")} };
  }

  void nascent_behavioural_test::transform_file(std::string& text) const
  {
    to_spaces(text, code_indent());

    replace_all(text, {{"?_behavioural", forename()},
                       {"?Behavioural", camel_name()},
                       {"?Test", to_camel_case(test_type()).append("Test")},
                       {"?Header.hpp", header_path().generic_string()},
                       {"?", test_type()}});
  }

  //=========================================== nascent_allocation_test ===========================================//

  void nascent_allocation_test::finalize()
  {
    camel_name(forename());
    if(header().empty()) header(std::filesystem::path{camel_name()}.concat(".hpp"));

    namespace fs = std::filesystem;
    nascent_test_base::finalize([](const fs::path& p) { return p; });
  }

  [[nodiscard]]
  auto nascent_allocation_test::create_file(std::string_view copyright, const std::filesystem::path& codeTemplatesDir,
                                            std::string_view nameEnding) const -> file_data
  {
    auto transformer{[this](std::string& text) { transform_file(text); }};
    return nascent_test_base::create_file(codeTemplatesDir, copyright, "MyClass", nameEnding, transformer);
  }

  [[nodiscard]]
  std::vector<std::string> nascent_allocation_test::constructors() const
  {
    return { {std::string{forename()}.append("_allocation_test(\"Allocation Test\")")} };
  }

  void nascent_allocation_test::transform_file(std::string& text) const
  {
    to_spaces(text, code_indent());

    replace_all(text, {{"?_class", forename()},
                       {"?Class", camel_name()},
                       {"?Allocation", to_camel_case(test_type())},
                       {"?_allocation", test_type()}});

    if (test_type() == "move_only_allocation")
    {
      replace_all(text, "bool PropagateCopy, ", "");
    }
  }

  //=========================================== test_runner ===========================================//

  void test_runner::nascent_test_data::operator()(const parsing::commandline::arg_list& args)
  {
    auto& nascentTests{runner.m_NascentTests};

    static creation_factory factory{{"semantic", "allocation", "behavioural"}, runner.m_Paths, runner.m_CodeIndent};
    auto nascent{factory.create(genus)};

    std::visit(
        variant_visitor{
          [&args,&species=species](nascent_semantics_test& nascent){
            nascent.test_type(species);
            nascent.qualified_name(args[0]);
            nascent.add_equivalent_type(args[1]);
          },
          [&args,&species=species](nascent_allocation_test& nascent){
            nascent.test_type(species);
            nascent.forename(args[0]);
          },
          [&args,&species=species](nascent_behavioural_test& nascent){
            nascent.test_type(species);
            nascent.header(args[0]);
          }
        },
        nascent);

    nascentTests.emplace_back(std::move(nascent));
  }

  test_runner::test_runner(int argc, char** argv, std::string copyright, project_paths paths, std::string codeIndent, std::ostream& stream)
    : m_Copyright{std::move(copyright)}
    , m_Paths{std::move(paths)}
    , m_CodeIndent{std::move(codeIndent)}
    , m_Stream{&stream}
  {
    check_indent(m_CodeIndent);

    process_args(argc, argv);

    namespace fs = std::filesystem;
    fs::create_directory(m_Paths.output());
    fs::create_directory(diagnostics_output_path(m_Paths.output()));
    fs::create_directory(test_summaries_path(m_Paths.output()));
  }

  void test_runner::process_args(int argc, char** argv)
  {

    using namespace parsing::commandline;

    const option familyOption{"--family", {"-f"}, {"family"},
      [this](const arg_list& args){
        if(m_NascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(variant_visitor{[&args](auto& nascent){ nascent.family(args[0]);}}, m_NascentTests.back());
      }
    };

    const option equivOption{"--equivalent-type", {"-e"}, {"equivalent_type"},
      [this](const arg_list& args){
        if(m_NascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        auto visitor{
          variant_visitor{
            [&args](nascent_semantics_test& nascent){ nascent.add_equivalent_type(args[0]); },
            [](auto&){}
          }
        };

        std::visit(visitor, m_NascentTests.back());
      }
    };

    const option headerOption{"--class-header", {"-ch"}, {"header of class to test"},
      [this](const arg_list& args){
        if(m_NascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(variant_visitor{[&args](auto& nascent){ nascent.header(args[0]); }}, m_NascentTests.back());
      }
    };

    const option nameOption{"--forename", {"-name"}, {"forename"},
      [this](const arg_list& args){
        if(m_NascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        std::visit(variant_visitor{[&args](auto& nascent){ nascent.forename(args[0]); }}, m_NascentTests.back());
      }
    };

    const option genFreeSourceOption{"gen-source", {"g"}, {"namespace"},
      [this](const arg_list& args) {
        if(m_NascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        using src_opt = nascent_test_base::gen_source_option;

        auto visitor{
          variant_visitor{
            [&args](nascent_behavioural_test& nascent) {
              nascent.generate_source_files(src_opt::yes);
              if(args[0] != "::") nascent.set_namespace(args[0]);
            },
            [](auto&) {}
          }
        };

        std::visit(visitor, m_NascentTests.back());
      }
    };

    const option genSemanticsSourceOption{"gen-source", {"g"}, {"source dir"},
      [this](const arg_list& args) {
        if(m_NascentTests.empty())
          throw std::logic_error{"Unable to find nascent test"};

        using src_opt = nascent_test_base::gen_source_option;

        auto visitor{
          variant_visitor{
            [&args](nascent_semantics_test& nascent) {
              nascent.generate_source_files(src_opt::yes);
              nascent.source_dir(args[0]);
            },
            [](auto&) {}
          }
        };

        std::visit(visitor, m_NascentTests.back());
      }
    };

    const std::vector<option> semanticsOptions{equivOption, familyOption, headerOption, genSemanticsSourceOption};

    const std::vector<option> allocationOptions{familyOption, headerOption};

    namespace fs = std::filesystem;
    const auto help{
      parse_invoke_depth_first(argc, argv,
                { {"test", {"t"}, {"test family name"},
                    [this](const arg_list& args) { m_SelectedFamilies.emplace(args.front(), false); }
                  },
                  {"select", {"s"}, {"source file name"},
                    [this](const arg_list& args) {
                      m_SelectedSources.emplace_back(fs::path{args.front()}.lexically_normal(), false);
                    }
                  },
                  {"create", {"c"}, {}, [](const arg_list&) {},
                   { {"regular_test", {"regular"}, {"qualified::class_name<class T>", "equivalent type"},
                      nascent_test_data{"semantic", "regular", *this}, semanticsOptions
                     },
                     {"move_only_test", {"move_only"}, {"qualified::class_name<class T>", "equivalent type"},
                      nascent_test_data{"semantic", "move_only", *this}, semanticsOptions
                     },
                     {"regular_allocation_test", {"regular_allocation", "allocation_test"}, {"raw class name"},
                      nascent_test_data{"allocation", "regular_allocation", *this}, allocationOptions
                     },
                     {"move_only_allocation_test", {"move_only_allocation"}, {"raw class name"},
                      nascent_test_data{"allocation", "move_only_allocation", *this}, allocationOptions
                     },
                     {"free_test", {"free"}, {"header"},
                      nascent_test_data{"behavioural", "free", *this}, {familyOption, nameOption, genFreeSourceOption}
                     },
                     {"performance_test", {"performance"}, {"header"},
                       nascent_test_data{"behavioural", "performance", *this}, {familyOption}
                     }
                   },
                   [this](const arg_list&) {
                      if(!m_NascentTests.empty())
                      {
                        variant_visitor visitor{
                          [copyright=m_Copyright](nascent_behavioural_test& nascent) { nascent.finalize(copyright); },
                          [copyright=m_Copyright](nascent_semantics_test& nascent) { nascent.finalize(copyright); },
                          [](auto& nascent) { nascent.finalize(); }
                        };
                        std::visit(visitor, m_NascentTests.back());
                      }
                    }
                  },
                  {"init", {"i"}, {"copyright owner", "path ending with project name", "code indent"},
                    [this](const arg_list& args) {
                      const auto ind{
                        [](std::string arg) {
                          
                          replace_all(arg, "\\t", "\t");
                          return indentation{arg};
                        }
                      };

                      m_NascentProjects.push_back(project_data{args[0], args[1], ind(args[2])});
                    },
                    { {"--no-build", {}, {}, [this](const arg_list&) { m_NascentProjects.back().do_build = build_invocation::no; }},
                      {"--to-files",  {}, {"filename (A file of this name will appear in multiple directories)"}, 
                        [this](const arg_list& args) { m_NascentProjects.back().output = args[0]; }},
                      {"--no-ide", {}, {}, [this](const arg_list&) {
                          auto& build{m_NascentProjects.back().do_build};
                          if(build == build_invocation::launch_ide) build = build_invocation::yes;
                        }
                      }
                    }
                  },
                  {"update-materials", {"u"}, {},
                    [this](const arg_list&) { m_UpdateMode = update_mode::soft; }
                  },
                  {"--async", {"-a"}, {},
                    [this](const arg_list&) {
                      if(m_ConcurrencyMode == concurrency_mode::serial)
                        m_ConcurrencyMode = concurrency_mode::family;
                    }
                  },
                  {"--async-depth", {"-ad"}, {"depth [0,1]"},
                    [this](const arg_list& args) {
                      const int i{std::clamp(std::stoi(args.front()), 0, 1)};
                      m_ConcurrencyMode = static_cast<concurrency_mode>(i);
                    }
                  },
                  {"--verbose",  {"-v"}, {}, [this](const arg_list&) { m_OutputMode |= output_mode::verbose; }},
                  {"--recovery", {"-r"}, {},
                    [this,recoveryDir{recovery_path(m_Paths.output())}] (const arg_list&) {
                      std::filesystem::create_directory(recoveryDir);
                      m_Recovery.recovery_file = recoveryDir / "Recovery.txt";
                      std::filesystem::remove(m_Recovery.recovery_file);
                    }
                  },
                  {"--dump", {}, {},
                    [this, recoveryDir{recovery_path(m_Paths.output())}] (const arg_list&) {
                      std::filesystem::create_directory(recoveryDir);
                      m_Recovery.dump_file = recoveryDir / "Dump.txt";
                      std::filesystem::remove(m_Recovery.dump_file);
                    }
                  }
                },
                [](std::string_view){})
        };

    if(!help.empty())
    {
      m_OutputMode |= output_mode::help;
      stream() << help;
    }
    else
    {
      check_argument_consistency();
    }
  }

  void test_runner::check_argument_consistency() const
  {
    using parsing::commandline::error;

    if(concurrent_execution() && (!m_Recovery.recovery_file.empty() || !m_Recovery.dump_file.empty()))
      throw std::runtime_error{error("Can't run asynchronously in recovery/dump mode\n")};
  }

  [[nodiscard]]
  std::string test_runner::stringify(concurrency_mode mode)
  {
    switch(mode)
    {
    case concurrency_mode::serial:
      return "Serial";
    case concurrency_mode::family:
      return "Family";
    case concurrency_mode::test:
      return "Test";
    }

    throw std::logic_error{"Unknown option for concurrency_mode"};
  }

  bool test_runner::mark_family(std::string_view name)
  {
    if(m_SelectedFamilies.empty()) return true;

    auto i{m_SelectedFamilies.find(name)};
    if(i != m_SelectedFamilies.end())
    {
      i->second = true;
      return true;
    }

    return false;
  }

  [[nodiscard]]
  test_family::summary test_runner::process_family(const test_family::results& results)
  {
    test_family::summary familySummary{results.execution_time};
    std::string output{};
    const auto detail{summary_detail::failure_messages | summary_detail::timings};
    for(const auto& s : results.logs)
    {
      if(mode(output_mode::verbose)) output += summarize(s, detail, tab, tab);
      familySummary.log += s;
    }

    if(mode(output_mode::verbose))
    {
      output.insert(0, report_time(familySummary));
    }
    else
    {
      output += summarize(familySummary, detail, tab, no_indent);
    }

    stream() << output;

    return familySummary;
  }

  namespace
  {
    const std::string& convert(const std::string& s) { return s; }
    std::string convert(const std::filesystem::path& p) { return p.generic_string(); }
  }

  void test_runner::check_for_missing_tests()
  {
    auto check{
      [&stream=stream()](const auto& tests, std::string_view type, auto fn) {
        for(const auto& test : tests)
        {
          if(!test.second)
          {
            using namespace parsing::commandline;
            stream << warning(std::string{"Test "}.append(type)
                              .append(" '")
                              .append(convert(test.first))
                              .append("' not found\n")
                              .append(fn(test.first)));
          }
        }
      }
    };

    check(m_SelectedFamilies, "Family", [](const std::string& name) -> std::string {
        if(auto pos{name.rfind('.')}; pos < std::string::npos)
        {
          return "--If trying to select a source file use 'select' rather than 'test'\n";
        }

        return "";
      }
    );

    check(m_SelectedSources, "File", [](const std::filesystem::path& p) -> std::string {
        if(!p.has_extension())
        {
          return "--If trying to test a family use 'test' rather than 'select'\n";
        }
  
        return "";
      }
    );
  }

  [[nodiscard]]
  auto test_runner::find_filename(const std::filesystem::path& filename) -> source_list::iterator
  {
    return std::find_if(m_SelectedSources.begin(), m_SelectedSources.end(),
                 [&filename, &repo=m_Paths.tests(), &root=m_Paths.project_root()](const auto& element){
                   const auto& source{element.first};

                   if(filename == source) return true;

                   if(filename.is_absolute())
                   {
                     if(source.is_absolute()) return false;

                     if(source.is_relative())
                     {
                       if(rebase_from(filename, root) == rebase_from(source, working_path()))
                         return true;
                     }
                   }

                   // filename is relative to where compilation was performed which
                   // cannot be known here. Therefore fallback to assuming the 'selected sources'
                   // live in the test repository

                   if(!repo.empty())
                   {
                     if(rebase_from(source, repo) == rebase_from(filename, repo))
                       return true;

                     if(const auto path{find_in_tree(repo, source)}; !path.empty())
                     {
                       if(rebase_from(path, repo) == rebase_from(filename, repo))
                         return true;
                     }
                   }

                   return false;
                 });
  }

  void test_runner::execute([[maybe_unused]] timer_resolution r)
  {
    namespace fs = std::filesystem;

    if(!mode(output_mode::help))
    {
      init_projects();
      create_tests();
      run_tests();
    }
  }

  void test_runner::create_tests()
  {
    if(!m_NascentTests.empty())
    {
      stream() << "Creating files...";

      for(const auto& nascentVessel : m_NascentTests)
      {
        variant_visitor visitor{
          [this](const auto& nascent) {
            std::string mess{"\n"};
            for(const auto& stub : nascent.stubs())
            {
              append_lines(mess, create_file(nascent, stub));
            }

            add_to_family(m_Paths.main_cpp(), nascent.family(), m_CodeIndent, nascent.constructors());

            stream() << mess;
          }
        };

        std::visit(visitor, nascentVessel);
      }

      namespace fs = std::filesystem;
      if(fs::exists(m_Paths.main_cpp_dir()) && fs::exists(m_Paths.cmade_build_dir()))
      {
        stream() << "\n";
        invoke(cd_cmd(m_Paths.main_cpp_dir()) && cmake_cmd(m_Paths.cmade_build_dir(), {}));
      }
    }
  }

  template<class Nascent>
  [[nodiscard]]
  std::string test_runner::create_file(const Nascent& nascent, std::string_view stub) const
  {
    namespace fs = std::filesystem;
    auto stringify{[root{m_Paths.project_root()}] (const fs::path file) { return fs::relative(file, root).generic_string();  }};

    const auto[outputFile, created]{nascent.create_file(m_Copyright, code_templates_path(m_Paths.project_root()), stub)};

    if(created)
    {
      if(outputFile.extension() == ".hpp")
      {
        if(const auto str{outputFile.string()}; str.find("Utilities.hpp") == std::string::npos)
        {
          add_include(m_Paths.include_target(), fs::relative(outputFile, m_Paths.tests()).generic_string());
        }
      }
      else if(outputFile.extension() == ".cpp")
      {
        add_to_cmake(m_Paths.main_cpp_dir(), m_Paths.tests(), outputFile, "target_sources(", ")\n", "${TestDir}/");
      }

      return std::string{"\""}.append(stringify(outputFile)).append("\"");
    }
    else
    {
      using namespace parsing::commandline;
      return warning(stringify(outputFile).append(" already exists, so not created\n"));
    }
  }

  void test_runner::report(std::string_view prefix, std::string_view message)
  {
    if(!message.empty())
    {
      stream() << prefix << '\n';
      stream() << message << "\n\n";
    }
  }

  void test_runner::run_tests()
  {
    using namespace std::chrono;
    const auto time{steady_clock::now()};

    const bool selected{!m_SelectedFamilies.empty() || !m_SelectedSources.empty()};
    if((m_NascentTests.empty() && m_NascentProjects.empty()) || selected)
    {
      if(!m_Families.empty())
      {
        stream() << "\nRunning tests...\n\n";
        log_summary summary{};
        if(!concurrent_execution())
        {
          for(auto& family : m_Families)
          {
            stream() << family.name() << ":\n";
            summary += process_family(family.execute(m_UpdateMode, m_ConcurrencyMode)).log;
          }
        }
        else
        {
          stream() << "\n\t--Using asynchronous execution, level: " << stringify(m_ConcurrencyMode) << "\n\n";
          std::vector<std::pair<std::string, std::future<test_family::results>>> results{};
          results.reserve(m_Families.size());

          for(auto& family : m_Families)
          {
            results.emplace_back(family.name(),
              std::async([&family, umode{m_UpdateMode}, cmode{m_ConcurrencyMode}](){
              return family.execute(umode, cmode); }));
          }

          for(auto& res : results)
          {
            stream() << res.first << ":\n";
            summary += process_family(res.second.get()).log;
          }
        }
        stream() << "\n-----------Grand Totals-----------\n";
        stream() << summarize(summary, steady_clock::now() - time, summary_detail::absent_checks | summary_detail::timings, indentation{"\t"}, no_indent);
      }
      else if(!selected)
      {
        stream() << "Nothing to do; try creating some tests!\nRun with --help to see options\n";
      }
    }

    check_for_missing_tests();
  }

  void test_runner::init_projects()
  {
    namespace fs = std::filesystem;

    if(!m_NascentProjects.empty())
    {
      stream() << "Initializing Project(s)....\n\n";
    }

    for(const auto& data : m_NascentProjects)
    {
      if(data.project_root.empty())
        throw std::runtime_error{"Project path should not be empty\n"};

      if(!data.project_root.is_absolute())
        throw std::runtime_error{std::string{"Project path '"}.append(data.project_root.generic_string()).append("' should be absolute\n")};

      if(fs::exists(data.project_root))
        throw std::runtime_error{std::string{"Project location '"}.append(data.project_root.generic_string()).append("' already exists\n")};

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

      report("Creating new project at location:", data.project_root.generic_string());

      fs::create_directories(data.project_root);
      fs::copy(project_template_path(m_Paths.project_root()), data.project_root, fs::copy_options::recursive | fs::copy_options::skip_existing);
      fs::create_directory(project_paths::source_path(data.project_root));
      fs::copy(aux_files_path(m_Paths.project_root()), aux_files_path(data.project_root), fs::copy_options::recursive | fs::copy_options::skip_existing);

      generate_test_main(data.copyright, data.project_root, data.code_indent);
      generate_build_system_files(data.project_root);

      if(data.do_build != build_invocation::no)
      {
        const auto mainDir{data.project_root / "TestAll"};
        const auto buildDir{project_paths::cmade_build_dir(data.project_root, mainDir)};

        invoke(cd_cmd(mainDir)
            && cmake_cmd(buildDir, data.output)
            && build_cmd(buildDir, data.output)
            && git_first_cmd(data.project_root, data.output)
            && (data.do_build == build_invocation::launch_ide ? launch_cmd(data.project_root, buildDir) : shell_command{})
        );
      }
    }
  }

  void test_runner::generate_test_main(std::string_view copyright, const std::filesystem::path& projRoot, indentation codeIndent) const
  {
    auto modifier{
      [copyright, codeIndent](std::string& text) {

        set_top_copyright(text, copyright);

        to_spaces(text, codeIndent);
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

    read_modify_write(projRoot / "TestAll" / "TestAllMain.cpp", modifier);
  }

  void test_runner::generate_build_system_files(const std::filesystem::path& projRoot) const
  {
    if(projRoot.empty())
      throw std::logic_error{"Pre-condition violated: path should not be empty"};

    namespace fs = std::filesystem;

    const std::filesystem::path relCmakeLocation{"TestAll/CMakeLists.txt"};

    auto setBuildSysPath{
      [&parentProjRoot=m_Paths.project_root(),&projRoot](std::string& text) {
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
              const auto relPath{fs::relative(parentProjRoot / "TestAll" / text.substr(left, count), projRoot / "TestAll")};
              text.replace(left, count, relPath.lexically_normal().generic_string());
            }
          }
        }
      }
    };

    read_modify_write(projRoot / relCmakeLocation, [setBuildSysPath, &projRoot](std::string& text) {
        setBuildSysPath(text);
        set_proj_name(text, projRoot);
      }
    );

    read_modify_write(projRoot / "Source" / "CMakeLists.txt", [&projRoot](std::string& text) {
        set_proj_name(text, projRoot);
      }
    );

    read_modify_write(project_template_path(projRoot) / relCmakeLocation, [setBuildSysPath](std::string& text) {
        setBuildSysPath(text);
      }
    );
  }
}
