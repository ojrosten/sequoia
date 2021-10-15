////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for TestCreator.hpp"
 */

#include "sequoia/TestFramework/TestCreator.hpp"

#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TestFramework/TestRunnerUtilities.hpp"

#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/Runtime/ShellCommands.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <array>
#include <chrono>
#include <stdexcept>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
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

  void cmake_nascent_tests(const std::filesystem::path& mainCppDir, const std::filesystem::path& buildDir, std::ostream& stream)
  {
    using namespace runtime;

    if(fs::exists(mainCppDir) && fs::exists(buildDir))
    {
      stream << "\n";
      invoke(cd_cmd(mainCppDir) && cmake_cmd(buildDir, {}));
    }
  }


  //=========================================== nascent_test_base ===========================================//

  void nascent_test_base::camel_name(std::string name) { m_CamelName = to_camel_case(std::move(name)); }

  [[nodiscard]]
  std::filesystem::path nascent_test_base::build_source_path(const std::filesystem::path& filename) const
  {
    if(filename.empty())
      throw std::runtime_error{"Header name is empty"};

    if(const auto path{find_in_tree(m_Paths.source(), filename)}; !path.empty())
      return path;

    for(auto e : st_HeaderExtensions)
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

  template<std::invocable<std::string&> FileTransformer>
  [[nodiscard]]
  std::string nascent_test_base::create_file(std::string_view nameStub, std::string_view nameEnding, FileTransformer transformer) const
  {
    const auto outputFile{(host_dir() / camel_name()) += nameEnding};
    auto stringify{[root{m_Paths.project_root()}] (const fs::path file) { return fs::relative(file, root).generic_string();  }};

    if(fs::exists(outputFile))
    {
      using namespace parsing::commandline;
      return warning(stringify(outputFile).append(" already exists, so not created\n"));
    }

    const auto inputFile{(code_templates_path(m_Paths.project_root()) / nameStub).concat(nameEnding)};

    fs::copy_file(inputFile, outputFile, fs::copy_options::overwrite_existing);
    if(auto contents{read_to_string(outputFile)})
    {
      if(std::string& text{contents.value()}; !text.empty())
      {
        set_top_copyright(text, m_Copyright);
        transformer(text);

        write_to_file(outputFile, text);
      }
    }
    else
    {
      throw std::runtime_error{report_failed_read(outputFile)};
    }

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

    return stringify(outputFile);
  }

  template<invocable_r<std::filesystem::path, std::filesystem::path> WhenAbsent, std::size_t N, std::invocable<std::string&> FileTransformer>
  void nascent_test_base::finalize(WhenAbsent fn,
                                   const std::array<std::string_view, N>& stubs,
                                   const std::vector<std::string>& constructors,
                                   std::string_view nameStub,
                                   FileTransformer transformer)
  {
    stream() << "Creating files for new test:\n";

    const auto srcPath{[this, fn]() {
        auto pth{build_source_path(m_Header)};
        if(pth.empty() && (m_SourceOption == gen_source_option::yes))
          pth = fn(m_Header);

        if(pth.empty())
          on_source_path_error();

        return pth;
      }()
    };

    finalize_header(srcPath);

    for(const auto& stub : stubs)
    {
      stream() << std::quoted(create_file(nameStub, stub, transformer)) << '\n';
    }

    add_to_family(m_Paths.main_cpp(), family(), m_CodeIndent, constructors);

    stream() << '\n';
  }

  void nascent_test_base::finalize_family(std::string_view fallbackIngredient)
  {
    if(m_Family.empty())
    {
      m_Family = fallbackIngredient;
      camel_to_words(m_Family);
    }
  }

  void nascent_test_base::finalize_header(const std::filesystem::path& sourcePath)
  {
    const auto relSourcePath{fs::relative(sourcePath, m_Paths.source())};
    const auto dir{(m_Paths.tests() / relSourcePath).parent_path()};
    fs::create_directories(dir);

    m_HostDir = dir;
    m_HeaderPath = fs::relative(sourcePath, m_Paths.source_root());
  }

  void nascent_test_base::on_source_path_error() const
  {
    auto mess{std::string{"Unable to locate file "}.append(m_Header.generic_string())};
    for(auto e : st_HeaderExtensions)
    {
      if(e != m_Header.extension())
      {
        const auto alternative{fs::path{m_Header}.replace_extension(e)};
        mess.append(" or ").append(alternative.generic_string());
      }
    }

    mess.append(" in the source repository\n").append(fs::relative(m_Paths.source(), m_Paths.tests()).generic_string());

    throw std::runtime_error{mess};
  }

  void nascent_test_base::set_cpp(const std::filesystem::path& headerPath, std::string_view copyright, std::string_view nameSpace)
  {
    const auto srcPath{fs::path{headerPath}.replace_extension("cpp")};

    const auto sourceRoot{paths().source_root()};
    stream() << std::quoted(fs::relative(srcPath, paths().project_root()) .generic_string()) << '\n';
    fs::copy_file(source_templates_path(paths().project_root()) / "MyCpp.cpp", srcPath);

    auto setCppText{
        [&](std::string& text) {
          set_top_copyright(text, copyright);
          process_namespace(text, nameSpace);
          replace_all(text, "?.hpp", rebase_from(headerPath, sourceRoot).generic_string());
          tabs_to_spacing(text, code_indent());
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

  void nascent_semantics_test::finalize()
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
    finalize_family(camel_name());
    if(header().empty()) header(std::filesystem::path{camel_name()}.concat(".hpp"));

    nascent_test_base::finalize([this, &nameSpace](const fs::path& filename) { return when_header_absent(filename, nameSpace); },
                                stubs(),
                                constructors(),
                                "MyClass",
                                [this](std::string& text) { transform_file(text); });
  }

  [[nodiscard]]
  std::filesystem::path nascent_semantics_test::when_header_absent(const std::filesystem::path& filename, const std::string& nameSpace)
  {
    const auto headerTemplate{std::string{"My"}.append(capitalize(to_camel_case(test_type()))).append("Class.hpp")};

    const auto headerPath{filename.is_absolute() ? filename : paths().source() / rebase_from(m_SourceDir / filename, paths().source())};

    stream() << std::quoted(fs::relative(headerPath, paths().project_root()).generic_string()) << '\n';
    fs::create_directories(headerPath.parent_path());
    fs::copy_file(source_templates_path(paths().project_root()) / headerTemplate, headerPath);

    read_modify_write(headerPath, [this, &nameSpace](std::string& text) { set_header_text(text, copyright(), nameSpace); });

    if(m_TemplateData.empty())
    {
      set_cpp(headerPath, copyright(), nameSpace);
    }

    return headerPath;
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

    tabs_to_spacing(text, code_indent());

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

    tabs_to_spacing(text, code_indent());
  }

  //=========================================== nascent_behavioural_test ===========================================//

  void nascent_behavioural_test::finalize()
  {
    const auto fallbackFamily{capitalize(forename().empty() ? header().filename().replace_extension().string() : forename())};
    finalize_family(fallbackFamily);

    if(forename().empty())
      forename(to_snake_case(fallbackFamily).append("_").append(test_type()));

    camel_name(forename());

    nascent_test_base::finalize([this](const fs::path& filename) { return when_header_absent(filename); },
                                stubs(),
                                constructors(),
                                "MyBehavioural",
                                [this](std::string& text) { transform_file(text); });
  }

  [[nodiscard]]
  std::filesystem::path nascent_behavioural_test::when_header_absent(const std::filesystem::path& filename)
  {
    const auto headerPath{filename.is_absolute() ? filename : paths().source() / rebase_from(filename, paths().source())};

    stream() << fs::relative(headerPath, paths().project_root()).generic_string() << '\n';
    fs::create_directories(headerPath.parent_path());
    fs::copy_file(source_templates_path(paths().project_root()) / "MyFreeFunctions.hpp", headerPath);

    read_modify_write(headerPath, [&nameSpace = m_Namespace](std::string& text) { process_namespace(text, nameSpace); });

    set_cpp(headerPath, copyright(), m_Namespace);

    return headerPath;
  }

  [[nodiscard]]
  std::vector<std::string> nascent_behavioural_test::constructors() const
  {
    return { {std::string{forename()}.append("_test(\"").append(to_camel_case(test_type())).append(" Test\")")} };
  }

  void nascent_behavioural_test::transform_file(std::string& text) const
  {
    tabs_to_spacing(text, code_indent());

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
    finalize_family(camel_name());
    if(header().empty()) header(std::filesystem::path{camel_name()}.concat(".hpp"));

    nascent_test_base::finalize([](const fs::path& p) { return p; },
                                stubs(),
                                constructors(),
                                "MyClass",
                                [this](std::string& text) { transform_file(text); });
  }

  [[nodiscard]]
  std::vector<std::string> nascent_allocation_test::constructors() const
  {
    return { {std::string{forename()}.append("_allocation_test(\"Allocation Test\")")} };
  }

  void nascent_allocation_test::transform_file(std::string& text) const
  {
    tabs_to_spacing(text, code_indent());

    replace_all(text, {{"?_class", forename()},
                       {"?Class", camel_name()},
                       {"?Allocation", to_camel_case(test_type())},
                       {"?_allocation", test_type()}});

    if (test_type() == "move_only_allocation")
    {
      replace_all(text, "bool PropagateCopy, ", "");
    }
  }
}