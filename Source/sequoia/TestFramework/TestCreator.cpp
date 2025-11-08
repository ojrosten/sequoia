////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for TestCreator.hpp
 */

#include "sequoia/TestFramework/TestCreator.hpp"

#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"
#include "sequoia/TestFramework/TestRunnerUtilities.hpp"

#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/TestFramework/Commands.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <array>
#include <chrono>
#include <stdexcept>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  constexpr auto npos{std::string::npos};

  namespace
  {
    void process_namespace(std::string& text, std::string_view nameSpace)
    {
      if(nameSpace.empty())
      {
        replace_all(text, replacement{"namespace\n", ""}, replacement{"?{\n", ""}, replacement{"?}\n", ""});
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
        replace_all(text, replacement{"namespace", std::string{"namespace "}.append(nameSpace)}, replacement{"?{", "{"}, replacement{"?}", "}"});
      }
    }

    void process_copyright_and_namespace(std::string& text, std::string_view copyright, std::string_view nameSpace)
    {
      set_top_copyright(text, copyright);
      process_namespace(text, nameSpace);
    }

    [[nodiscard]]
    std::string to_surname(nascent_test_flavour f)
    {
      switch(f)
      {
      case nascent_test_flavour::standard:
        return "test";
      case nascent_test_flavour::framework_diagnostics:
        return "diagnostics";
      }

      throw std::logic_error{"Unrecognized option for nascent_test_flavour"};
    }

    template<class Nascent>
    [[nodiscard]]
    std::vector<std::string> to_stubs(const Nascent& nascent)
    {
      switch(nascent.flavour())
      {
      case nascent_test_flavour::standard:
        return nascent.stubs();
      case nascent_test_flavour::framework_diagnostics:
        return nascent.framework_diagnostics_stubs();
      }

      throw std::logic_error{"Unrecognized option for nascent_test_flavour"};
    }

    template<std::invocable<fs::path> Amender, invocable_r<fs::path, main_paths> PathGenerator>
    void ammend_file(const project_paths& projPaths, Amender f, PathGenerator g)
    {
      f(g(projPaths.main()));
      for(const auto& mainCpp : projPaths.ancillary_main_cpps())
      {
        f(g(mainCpp));
      }
    }
  }

  [[nodiscard]]
  bool handle_as_ref(std::string_view type)
  {
    if(type.empty())
      throw std::logic_error{"Equivalent type is unspecified"};

    const auto startPos{type.find_first_not_of(' ')};
    if(startPos == npos)
      throw std::logic_error{"Equivalent type is unspecified"};

    if((type.back() == '*') || (type.back() == '&')) return false;

    const auto endPos{type.find_first_of(' ', startPos)};
    auto token{std::string_view{type}.substr(startPos, endPos - startPos)};

    constexpr std::array<std::string_view, 9> funTypes{"int", "float", "double", "bool", "char", "short", "long", "signed", "unsigned"};
    for(auto t : funTypes)
    {
      if(const auto pos{token.find(t)}; pos != npos)
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

  void cmake_nascent_tests(const project_paths& projPaths, std::ostream& stream)
  {
    using namespace runtime;

    auto cmake{
      [&stream](const main_paths& main, const build_paths& buildPaths) {
        if(fs::exists(main.dir()) && fs::exists(buildPaths.cmake_cache_dir()))
        {
          stream << "\n";
          invoke(cd_cmd(main.dir()) && cmake_cmd(buildPaths, {}));
        }
      }
    };

    cmake(projPaths.main(), projPaths.build());
  }


  //=========================================== nascent_test_base ===========================================//

  void nascent_test_base::camel_name(std::string name) { m_CamelName = to_camel_case(std::move(name)); }

  [[nodiscard]]
  std::vector<std::string> nascent_test_base::framework_diagnostics_stubs()
  {
    return {"Diagnostics.hpp", "Diagnostics.cpp"};
  };

  [[nodiscard]]
  std::filesystem::path nascent_test_base::build_source_path(const std::filesystem::path& filename) const
  {
    if(filename.empty())
      throw std::runtime_error{"Header name is empty"};

    if(const auto path{find_in_tree(m_Paths.source().repo(), filename)}; !path.empty())
      return path;

    for(auto e : st_HeaderExtensions)
    {
      if(e != filename.extension())
      {
        const auto alternative{std::filesystem::path{filename}.replace_extension(e)};
        if(const auto path{find_in_tree(m_Paths.source().repo(), alternative)}; !path.empty())
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
      return warning(stringify(outputFile).append(" already exists, so not created"));
    }

    const auto inputFile{(m_Paths.aux_paths().test_templates() / nameStub).concat(nameEnding)};

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
      if(const auto str{outputFile.string()}; str.find("Utilities.hpp") == npos)
      {
        add_include(m_Paths.main().common_includes(), fs::relative(outputFile, m_Paths.tests().repo()).generic_string());
      }
    }
    else if(outputFile.extension() == ".cpp")
    {
      auto addToCMake{
        [this, outputFile](const fs::path& mainCMake) {
          add_to_cmake(mainCMake, m_Paths.tests().repo(), outputFile, "target_sources(", ")\n", "${TestDir}/");
        }
      };

      ammend_file(m_Paths, addToCMake, [](const main_paths& info) { return info.cmake_lists(); });
    }

    return std::string{"\""}.append(stringify(outputFile)).append("\"");
  }

  template<invocable_r<std::filesystem::path, std::filesystem::path> WhenAbsent, std::invocable<std::string&> FileTransformer>
  void nascent_test_base::finalize(WhenAbsent fn,
                                   const std::vector<std::string>& stubs,
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
      stream() << create_file(nameStub, stub, transformer) << '\n';
    }

    auto addToSuite{
      [this, &constructors](const fs::path& mainCpp) {
        add_to_suite(mainCpp, suite(), m_CodeIndent, constructors);
      }
    };

    ammend_file(m_Paths, addToSuite, [](const main_paths& info) { return info.file(); });

    stream() << '\n';
  }

  void nascent_test_base::finalize_suite(std::string_view fallbackIngredient)
  {
    if(m_Suite.empty())
    {
      m_Suite = fallbackIngredient;
      camel_to_words(m_Suite);
    }
  }

  void nascent_test_base::finalize_header(const std::filesystem::path& sourcePath)
  {
    const auto relSourcePath{fs::relative(sourcePath, m_Paths.source().project())};
    const auto dir{(m_Paths.tests().repo() / relSourcePath).parent_path()};
    fs::create_directories(dir);

    m_HostDir = dir;
    m_HeaderPath = fs::relative(sourcePath, m_Paths.source().repo());
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

    mess.append(" in the source repository\n").append(fs::relative(m_Paths.source().repo(), m_Paths.tests().repo()).generic_string());

    throw std::runtime_error{mess};
  }

  void nascent_test_base::set_cpp(const std::filesystem::path& headerPath, std::string_view nameSpace)
  {
    const auto srcPath{fs::path{headerPath}.replace_extension("cpp")};

    stream() << std::quoted(fs::relative(srcPath, paths().project_root()) .generic_string()) << '\n';
    fs::copy_file(paths().aux_paths().source_templates() / "MyCpp.cpp", srcPath);

    auto setCppText{
        [&, copyright{copyright()}](std::string& text) {
          process_copyright_and_namespace(text, copyright, nameSpace);
          replace_all(text, "?.hpp", rebase_from(headerPath, paths().source().repo()).generic_string());
          tabs_to_spacing(text, code_indent());
        }
    };

    read_modify_write(srcPath, setCppText);

    add_to_cmake(paths().source().cmake_lists(), paths().source().project(), srcPath, "set(SourceList", ")\n", "");

    read_modify_write(paths().main().cmake_lists(), [&root = paths().project_root()](std::string& text) {
        replace_all(text, "#!", "");
      }
    );
  }

  void nascent_test_base::make_common_replacements(std::string& text) const
  {
    
    replace_all(text, replacement{"?::testing", std::format("{}::testing", project_namespace())},
                      replacement{"using namespace sequoia::testing;", project_namespace() == "sequoia" ? "" : "using namespace sequoia::testing;\n\n\t"},
                      replacement{"?forename", forename()},
                      replacement{"?surname", surname()});

    tabs_to_spacing(text, code_indent());
  }

  //=========================================== nascent_semantics_test ===========================================//

  [[nodiscard]]
  std::vector<std::string> nascent_semantics_test::stubs()
  {
    return {"TestingUtilities.hpp",
            "TestingDiagnostics.hpp",
            "TestingDiagnostics.cpp",
            "Test.hpp",
            "Test.cpp"};
  };

  void nascent_semantics_test::finalize()
  {
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
          std::ranges::for_each(m_TemplateData,
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

    if(surname().empty()) surname(to_surname(flavour()));

    camel_name(forename());
    finalize_suite(camel_name());
    if(header().empty()) header(std::filesystem::path{camel_name()}.concat(".hpp"));

    nascent_test_base::finalize([this, &nameSpace](const fs::path& filename) { return when_header_absent(filename, nameSpace); },
                                to_stubs(*this),
                                constructors(),
                                "MyClass",
                                [this](std::string& text) { transform_file(text); });
  }

  [[nodiscard]]
  std::filesystem::path nascent_semantics_test::when_header_absent(const std::filesystem::path& filename, const std::string& nameSpace)
  {
    const auto headerTemplate{std::string{"My"}.append(capitalize(to_camel_case(test_type()))).append("Class.hpp")};

    const auto headerPath{filename.is_absolute() ? filename : paths().source().project() / rebase_from(m_SourceDir / filename, paths().source().project())};

    stream() << std::quoted(fs::relative(headerPath, paths().project_root()).generic_string()) << '\n';
    fs::create_directories(headerPath.parent_path());
    fs::copy_file(paths().aux_paths().source_templates() / headerTemplate, headerPath);

    read_modify_write(headerPath, [this, &nameSpace](std::string& text) { set_header_text(text, copyright(), nameSpace); });

    if(m_TemplateData.empty())
    {
      set_cpp(headerPath, nameSpace);
    }

    return headerPath;
  }

  [[nodiscard]]
  std::vector<std::string> nascent_semantics_test::constructors() const
  {
    return { {std::string{forename()}.append("_false_negative_").append(surname()).append("{\"False Negative Test\"}")},
             {std::string{forename()}.append("_").append(surname()).append("{\"Unit Test\"}")}};
  }

  void nascent_semantics_test::transform_file(std::string& text) const
  {
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

    make_common_replacements(text);

    replace_all(text, replacement{"::?_class", m_QualifiedName},
                      replacement{"?Class.hpp", header_path().generic_string()},
                      replacement{"?Class", camel_name()},
                      replacement{"?Test", to_camel_case(test_type()).append("Test")},
                      replacement{"?", test_type()});
  }

  void nascent_semantics_test::set_header_text(std::string& text, std::string_view copyright, std::string_view nameSpace) const
  {
    process_copyright_and_namespace(text, copyright, nameSpace);
    replace_all(text, "?type", forename());
    if(m_TemplateData.empty())
    {
      replace_all(text, "\ttemplate<?>\n", "");
      replace_all(text, "template<?>\n", "");
    }
    else
    {
      const auto templateSpec{std::string{"template"}.append(to_string(m_TemplateData))};
      replace_all(text, "template<?>", templateSpec);
    }

    tabs_to_spacing(text, code_indent());
  }

  //=========================================== nascent_allocation_test ===========================================//

  [[nodiscard]]
  std::vector<std::string> nascent_allocation_test::stubs()
  {
    return {"AllocationTest.hpp",
            "AllocationTest.cpp"};
  };

  void nascent_allocation_test::finalize()
  {
    if(surname().empty()) surname(std::string{"allocation_"}.append(to_surname(flavour())));
    camel_name(forename());
    finalize_suite(camel_name());
    if(header().empty()) header(std::filesystem::path{camel_name()}.concat(".hpp"));

    nascent_test_base::finalize([](const fs::path& p) { return p; },
                                to_stubs(*this),
                                constructors(),
                                "MyClass",
                                [this](std::string& text) { transform_file(text); });
  }

  [[nodiscard]]
  std::vector<std::string> nascent_allocation_test::constructors() const
  {
    return { {std::string{forename()}.append("_").append(surname()).append("{\"Allocation Test\"}")} };
  }

  void nascent_allocation_test::transform_file(std::string& text) const
  {
    tabs_to_spacing(text, code_indent());

    make_common_replacements(text);

    replace_all(text, replacement{"?Class", camel_name()},
                      replacement{"?Allocation", to_camel_case(test_type())},
                      replacement{"?_allocation", test_type()});

    if (test_type() == "move_only_allocation")
    {
      replace_all(text, "bool PropagateCopy, ", "");
    }
  }

  //=========================================== nascent_behavioural_test ===========================================//

  [[nodiscard]]
  std::vector<std::string> nascent_behavioural_test::stubs()
  {
    return {"Test.hpp", "Test.cpp"};
  };

  void nascent_behavioural_test::finalize()
  {
    const auto fallbackSuite{capitalize(forename().empty() ? header().filename().replace_extension().string() : forename())};
    finalize_suite(fallbackSuite);

    if(forename().empty()) forename(to_snake_case(fallbackSuite));

    if(surname().empty()) surname(std::string{test_type()}.append("_").append(to_surname(flavour())));

    camel_name(std::string{forename()}.append("_").append(test_type()));

    nascent_test_base::finalize([this](const fs::path& filename) { return when_header_absent(filename); },
                                to_stubs(*this),
                                constructors(),
                                "MyBehavioural",
                                [this](std::string& text) { transform_file(text); });
  }

  [[nodiscard]]
  std::filesystem::path nascent_behavioural_test::when_header_absent(const std::filesystem::path& filename)
  {
    const auto headerPath{filename.is_absolute() ? filename : paths().source().project() / rebase_from(filename, paths().source().project())};

    stream() << fs::relative(headerPath, paths().project_root()).generic_string() << '\n';
    fs::create_directories(headerPath.parent_path());
    fs::copy_file(paths().aux_paths().source_templates() / "MyFreeFunctions.hpp", headerPath);

    read_modify_write(headerPath, [&nameSpace = m_Namespace, copyright{copyright()}](std::string& text) {
        process_copyright_and_namespace(text, copyright, nameSpace);
      }
    );

    set_cpp(headerPath, m_Namespace);

    return headerPath;
  }

  [[nodiscard]]
  std::vector<std::string> nascent_behavioural_test::constructors() const
  {
    auto makeClassName{
      [this](std::string_view middlename) -> std::string {
        auto testClass{std::string{forename()}.append("_")};
        if(!middlename.empty()) testClass.append(middlename).append("_");
        return testClass.append(surname());
      }
    };

    auto make{
      [makeClassName](std::string_view middlename) -> std::string {
        const auto testClass{makeClassName(middlename)};
        const auto testName{to_camel_case(testClass, " ")};

        return std::string{testClass}.append("{\"").append(testName).append("\"}");
      }
    };

    switch(flavour())
    {
    case nascent_test_flavour::standard:
      return { make("") };
    case nascent_test_flavour::framework_diagnostics:
      return { make("false_positive"), make("false_negative")};
    }

    throw std::logic_error{"Unrecognized option for nascent_test_flavour"};
  }

  void nascent_behavioural_test::transform_file(std::string& text) const
  {
    make_common_replacements(text);

    replace_all(text, replacement{"?Behavioural", camel_name()},
                      replacement{"?Test", to_camel_case(test_type()).append("Test")},
                      replacement{"?Header.hpp", header_path().generic_string()},
                      replacement{"?", test_type()});
  }
}
