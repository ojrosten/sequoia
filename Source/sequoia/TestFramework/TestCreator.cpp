////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/TestCreator.hpp"

#include "sequoia/TestFramework/CMakeCache.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"
#include "sequoia/TestFramework/FileSystemUtilities.hpp"
#include "sequoia/TestFramework/TestRunnerUtilities.hpp"

#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/TestFramework/Commands.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <array>
#include <chrono>
#include <format>
#include <stdexcept>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  constexpr auto npos{std::string::npos};

  namespace
  {
    /** \brief Whether `name` can name a namespace or a class: letters, digits and underscores, not led by a digit */
    [[nodiscard]]
    bool is_identifier(std::string_view name)
    {
      auto isIdentifierChar{[](char c) { return std::isalnum(static_cast<unsigned char>(c)) || (c == '_'); }};

      return !name.empty()
          && !std::isdigit(static_cast<unsigned char>(name.front()))
          && std::ranges::all_of(name, isIdentifierChar);
    }
  }

  [[nodiscard]]
  std::string project_namespace_for(const std::filesystem::path& sourceProject)
  {
    if(!fs::is_directory(sourceProject))
      throw std::runtime_error{
              std::format(
                "Unable to locate the project's purported source directory, {}.\n"
                "The source directory is either deduced from the name of the directory into which the project\n"
                "is checked out, or overridden by specifying a (different) `source_folder` in the project_paths\n"
                "customizer. A worktree, or a checkout named anything but the project, needs `source_folder`\n"
                "to be appropriately set.\n",
                sourceProject.generic_string()
              )
            };

    const auto name{back(sourceProject).string()};

    if(!is_identifier(name))
      throw std::runtime_error{
              std::format(
                "The project's namespace is taken from its source directory, {}, which in this case is not a\n"
                "permissible namespace name. Please supply `source_folder` in the project_paths customizer.\n",
                name
              )
            };

    return name;
  }

  namespace
  {
    /** \brief Wraps a string in quotation marks, escaping nothing.

        `std::quoted` would do here, and this spelling exists only to stay identical to
        `modules-native`, where it cannot: libstdc++'s module std exports the manipulator
        but not the `operator<<` for the `std::__detail::_Quoted_string` it returns, so
        under `import std` the manipulator is visible and unusable. Fixed upstream in gcc 16.1.

        This is not a general substitute. `std::quoted` escapes `"` and `\`; this escapes
        nothing, and would be wrong for a string containing either. It is exact for what
        both call sites pass - a project-relative `generic_string()` - and the generated
        `io.txt` is byte-identical either way, which was checked rather than assumed.
     */
    [[nodiscard]]
    std::string quote_without_escapes(std::string_view relativePath)
    {
      return std::format("\"{}\"", relativePath);
    }

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

    template<std::invocable<fs::path> Amender, invocable_exact_r<fs::path, main_paths> PathGenerator>
    void ammend_file(const project_paths& projPaths, Amender f, PathGenerator g)
    {
      f(g(projPaths.main()));
      for(const auto& mainCpp : projPaths.ancillary_main_cpps())
      {
        f(g(mainCpp));
      }
    }

    /** \brief Whether `dir` is `root` or lies beneath it, once symbolic links are resolved; both must exist. */
    [[nodiscard]]
    bool lies_within(const fs::path& dir, const fs::path& root)
    {
      const auto canonicalRoot{fs::canonical(root)};
      return std::ranges::mismatch(canonicalRoot, fs::canonical(dir)).in1 == canonicalRoot.end();
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

  std::string cmake_nascent_tests(const project_paths& projPaths)
  {
    using namespace runtime;

    const auto& build{projPaths.build()};
    if(!fs::exists(build.cmake_cache_dir())) return {};

    // A build tree copied with its checkout still names the source directory of the original.
    const auto sourceDir{cmake_cache{build}.source_dir()};
    if(!fs::is_directory(sourceDir) || !lies_within(sourceDir, projPaths.project_root()))
      throw std::runtime_error{
              std::format("CMake not run: the build tree was configured from {},\n"
                          "which is not a directory within this project, {}\n",
                          sourceDir.generic_string(),
                          projPaths.project_root().generic_string())
            };

    // Removed first, so that a run which writes nothing cannot hand back the previous run's output.
    const auto outputPath{build.cmake_cache_dir() / "CMakeOutput.txt"};
    fs::remove(outputPath);
    throw_unless_succeeded(invoke(cd_cmd(sourceDir) && cmake_cmd(build, outputPath)),
                           "Running CMake on the new tests",
                           std::format("The new tests' files and registrations are in place.\n"
                                       "CMake's output is {}.\n"
                                       "Once the cause is fixed, run `{}` from {}",
                                       where_written(sourceDir, outputPath),
                                       cmake_invocation(build),
                                       sourceDir.generic_string()));

    return read_to_string(outputPath, std::ios_base::in).value_or(std::string{});
  }


  //=========================================== nascent_test_base ===========================================//

  void nascent_test_base::set_type_name(std::string_view name)
  {
    m_TypeFileStem = to_camel_case(name);
    if(m_Header.empty())
      m_Header = fs::path{m_TypeFileStem}.concat(".hpp");
  }

  [[nodiscard]]
  std::vector<std::string> nascent_test_base::framework_diagnostics_stubs()
  {
    return {"Diagnostics.hpp", "Diagnostics.cpp"};
  }

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

  [[nodiscard]]
  std::string nascent_test_base::test_name() const
  {
    return m_FullName.value_or(std::format("{}_{}", m_Forename, m_Surname));
  }

  [[nodiscard]]
  std::string nascent_test_base::test_file_stem() const
  {
    return to_camel_case(test_name());
  }

  [[nodiscard]]
  std::string nascent_test_base::testing_utilities_include() const
  {
    const auto withinHostDir{m_TestingUtilities.lexically_relative(m_HostDir)};
    const bool isWithin{!withinHostDir.empty() && (*withinHostDir.begin() != "..")};

    return (isWithin ? withinHostDir : m_TestingUtilities.lexically_relative(m_Paths.tests().repo())).generic_string();
  }

  /** The name is normalised, and an absolute one made relative to the tests repository, so that
      `./Stuff/X.hpp`, `Stuff/../Maths/X.hpp` and a full path are all found. It then names every regular
      file beneath the repository whose path ends with it, and must name exactly one: a directory of the
      same name is not a candidate.
   */
  void nascent_test_base::locate_testing_utilities()
  {
    const auto& repo{m_Paths.tests().repo()};
    const auto repoName{fs::relative(repo, m_Paths.project_root()).generic_string()};
    const auto sought{m_TestingUtilities.lexically_normal()};

    auto failure{
      [&sought, &repoName](std::string_view problem) {
        return std::runtime_error{std::format("The testing utilities {} {} the tests repository {}",
                                              sought.generic_string(),
                                              problem,
                                              repoName)};
      }
    };

    const auto withinRepo{sought.is_absolute() ? sought.lexically_relative(repo) : sought};
    if(withinRepo.empty() || (*withinRepo.begin() == ".."))
      throw failure("do not lie beneath");

    const auto suffix{withinRepo.generic_string()};
    auto endsWithSought{
      [&repo, &suffix](const fs::directory_entry& entry) {
        const auto relative{entry.path().lexically_relative(repo).generic_string()};
        return entry.is_regular_file() && ((relative == suffix) || relative.ends_with("/" + suffix));
      }
    };

    auto candidates{
         fs::recursive_directory_iterator{repo}
       | std::views::filter(endsWithSought)
       | std::views::transform([](const fs::directory_entry& entry) { return entry.path(); })
       | std::ranges::to<std::vector>()
    };

    if(candidates.empty())
      throw failure("cannot be found in");

    if(candidates.size() > 1)
    {
      std::ranges::sort(candidates);
      auto relativeToRepo{[&repo](const fs::path& p) { return p.lexically_relative(repo).generic_string(); }};
      throw std::runtime_error{
        std::format("The testing utilities {} are ambiguous in the tests repository {}; they may be any of\n{}",
                    sought.generic_string(),
                    repoName,
                    candidates | std::views::transform(relativeToRepo) | std::views::join_with('\n')
                               | std::ranges::to<std::string>())
      };
    }

    m_TestingUtilities = candidates.front();
  }

  void nascent_test_base::check_full_name(std::span<const fs::path> companionFiles,
                                          std::span<const fs::path> ownFiles) const
  {
    const auto& name{m_FullName.value()};

    if(!is_identifier(name))
      throw std::runtime_error{std::format("--fullname '{}' is not an identifier, so cannot name a test class", name)};

    const auto registration{std::format("register_test<{}>", name)};
    auto registers{
      [&registration](const fs::path& mainCpp) {
        const auto text{read_to_string(mainCpp, std::ios_base::in)};
        if(!text)
          throw std::runtime_error{report_failed_read(mainCpp)};

        return text->contains(registration);
      }
    };

    auto registersIn{[&registers](const main_paths& main) { return registers(main.file()); }};
    if(registersIn(m_Paths.main()) || std::ranges::any_of(m_Paths.ancillary_main_cpps(), registersIn))
      throw std::runtime_error{std::format("--fullname {} names a test which is already registered", name)};

    auto sameIgnoringCase{
      [](const fs::path& lhs, const fs::path& rhs) {
        auto lower{[](char c) { return std::tolower(static_cast<unsigned char>(c)); }};
        return std::ranges::equal(lhs.filename().string(), rhs.filename().string(), {}, lower, lower);
      }
    };

    // Lexically, since resolving the path would spell it as the file already present does.
    auto relativeToRoot{
      [this](const fs::path& p) { return p.lexically_relative(m_Paths.project_root()).generic_string(); }
    };

    for(const auto& own : ownFiles)
    {
      auto collides{[&own, &sameIgnoringCase](const fs::path& other) { return sameIgnoringCase(own, other); }};

      if(const auto companion{std::ranges::find_if(companionFiles, collides)}; companion != companionFiles.end())
        throw std::runtime_error{
          std::format("--fullname {} would name the test's file {}, which is also the file of the type under test {}",
                      name,
                      relativeToRoot(own),
                      relativeToRoot(*companion))
        };

      if(fs::is_directory(own.parent_path()))
      {
        auto entries{fs::directory_iterator{own.parent_path()}
                       | std::views::transform([](const fs::directory_entry& entry) { return entry.path(); })};

        if(const auto existing{std::ranges::find_if(entries, collides)}; existing != entries.end())
          throw std::runtime_error{
            std::format("--fullname {} would name the test's file {}, but {} is already present",
                        name,
                        relativeToRoot(own),
                        relativeToRoot(*existing))
          };
      }
    }
  }

  template<std::invocable<std::string&> FileTransformer>
  [[nodiscard]]
  std::string nascent_test_base::create_file(std::string_view nameStub,
                                             std::string_view nameEnding,
                                             const fs::path& outputFile,
                                             add_to_common_includes include,
                                             FileTransformer transformer) const
  {
    auto stringify{[root{m_Paths.project_root()}] (const fs::path file) { return fs::relative(file, root).generic_string();  }};

    if(fs::exists(outputFile))
    {
      using namespace parsing::commandline;
      return warning(stringify(outputFile).append(" already exists, so not created"));
    }

    const auto inputFile{(m_Paths.aux_paths().test_templates() / nameStub).concat(nameEnding)};

    fs::copy_file(inputFile, outputFile, fs::copy_options::overwrite_existing);
    if(auto contents{read_to_string(outputFile, std::ios_base::in)})
    {
      if(std::string& text{contents.value()}; !text.empty())
      {
        set_top_copyright(text, m_Copyright);
        transformer(text);

        write_to_file(outputFile, text, std::ios_base::out);
      }
    }
    else
    {
      throw std::runtime_error{report_failed_read(outputFile)};
    }

    if(outputFile.extension() == ".hpp")
    {
      if(include == add_to_common_includes::yes)
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

  template<invocable_exact_r<std::filesystem::path, std::filesystem::path> WhereAbsent,
           std::invocable<std::filesystem::path> Generator,
           std::invocable<std::string&> FileTransformer>
  void nascent_test_base::finalize(WhereAbsent whereAbsent,
                                   Generator generate,
                                   const std::vector<companion_stub>& companionStubs,
                                   const std::vector<std::string>& ownStubs,
                                   const std::vector<std::string>& testClasses,
                                   std::string_view nameStub,
                                   FileTransformer transformer)
  {
    if(!m_TestingUtilities.empty())
      locate_testing_utilities();

    stream() << "Creating files for new test:\n";

    const auto existingSource{build_source_path(m_Header)};
    const bool generateSource{existingSource.empty() && (m_SourceOption == gen_source_option::yes)};
    const auto srcPath{generateSource ? whereAbsent(m_Header) : existingSource};
    if(srcPath.empty())
      on_source_path_error();

    finalize_header(srcPath);

    auto companionFile{
      [this](const companion_stub& stub) { return (host_dir() / type_file_stem()) += stub.ending; }
    };

    auto ownFile{
      [this](const std::string& stub) { return (host_dir() / test_file_stem()) += fs::path{stub}.extension(); }
    };

    const auto companionFiles{companionStubs | std::views::transform(companionFile) | std::ranges::to<std::vector>()};
    const auto ownFiles{ownStubs | std::views::transform(ownFile) | std::ranges::to<std::vector>()};

    // Everything is checked before anything is written.
    if(m_FullName)
      check_full_name(companionFiles, ownFiles);

    if(generateSource)
      generate(srcPath);

    fs::create_directories(host_dir());

    for(const auto& [stub, file] : std::views::zip(companionStubs, companionFiles))
    {
      stream() << create_file(nameStub, stub.ending, file, stub.include, transformer) << '\n';
    }

    for(const auto& [stub, file] : std::views::zip(ownStubs, ownFiles))
    {
      stream() << create_file(nameStub, stub, file, add_to_common_includes::yes, transformer) << '\n';
    }

    auto registerTests{
      [this, &testClasses](const fs::path& mainCpp) {
        add_test_registrations(mainCpp, m_CodeIndent, testClasses);
      }
    };

    ammend_file(m_Paths, registerTests, [](const main_paths& info) { return info.file(); });

    stream() << '\n';
  }

  void nascent_test_base::finalize_header(const std::filesystem::path& sourcePath)
  {
    const auto relSourcePath{fs::relative(sourcePath, m_Paths.source().project())};
    m_HostDir = (m_Paths.tests().repo() / relSourcePath).parent_path();
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

    stream() << quote_without_escapes(fs::relative(srcPath, paths().project_root()).generic_string()) << '\n';
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
    replace_all(text, replacement{"?test_name", test_name()},
                      replacement{"?TestFile", test_file_stem()},
                      replacement{"?::testing", std::format("{}::testing", project_namespace())},
                      replacement{"using namespace sequoia::testing;", project_namespace() == "sequoia" ? "" : "using namespace sequoia::testing;\n\n\t"},
                      replacement{"?forename", forename()},
                      replacement{"?surname", surname()});

    tabs_to_spacing(text, code_indent());
  }

  //=========================================== nascent_semantics_test ===========================================//

  [[nodiscard]]
  std::vector<std::string> nascent_semantics_test::stubs()
  {
    return {"Test.hpp", "Test.cpp"};
  }

  [[nodiscard]]
  std::vector<companion_stub> nascent_semantics_test::companion_stubs()
  {
    return {{"TestingUtilities.hpp",   add_to_common_includes::no},
            {"TestingDiagnostics.hpp", add_to_common_includes::yes},
            {"TestingDiagnostics.cpp", add_to_common_includes::no}};
  }

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

    if(surname().empty())
      surname(to_surname(flavour()));

    set_type_name(forename());

    // Testing utilities named on the commandline hold the value_tester, and its false-negative
    // diagnostics belong with it, so neither companion is generated.
    nascent_test_base::finalize([this](const fs::path& filename) { return where_header_absent(filename); },
                                [this, &nameSpace](const fs::path& headerPath) {
                                  generate_header(headerPath, nameSpace);
                                },
                                testing_utilities().empty() ? companion_stubs() : std::vector<companion_stub>{},
                                to_stubs(*this),
                                test_classes(),
                                "MyClass",
                                [this](std::string& text) { transform_file(text); });
  }

  [[nodiscard]]
  std::filesystem::path nascent_semantics_test::where_header_absent(const std::filesystem::path& filename) const
  {
    const auto& project{paths().source().project()};
    return filename.is_absolute() ? filename : project / rebase_from(m_SourceDir / filename, project);
  }

  void nascent_semantics_test::generate_header(const std::filesystem::path& headerPath, const std::string& nameSpace)
  {
    const auto headerTemplate{std::string{"My"}.append(capitalize(to_camel_case(test_type()))).append("Class.hpp")};

    stream() << quote_without_escapes(fs::relative(headerPath, paths().project_root()).generic_string()) << '\n';
    fs::create_directories(headerPath.parent_path());
    fs::copy_file(paths().aux_paths().source_templates() / headerTemplate, headerPath);

    read_modify_write(headerPath, [this, &nameSpace](std::string& text) { set_header_text(text, copyright(), nameSpace); });

    if(m_TemplateData.empty())
    {
      set_cpp(headerPath, nameSpace);
    }
  }

  [[nodiscard]]
  std::vector<std::string> nascent_semantics_test::test_classes() const
  {
    if(!testing_utilities().empty())
      return { test_name() };

    return { std::format("{}_false_negative_{}", forename(), surname()), test_name() };
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

    constexpr std::string_view generatedUtilities{"#include \"?ClassTestingUtilities.hpp\""};
    if(!testing_utilities().empty())
      replace_all(text,
                  generatedUtilities,
                  std::format("#include \"{}\"\n\n#include \"sequoia/TestFramework/?TestCore.hpp\"",
                              testing_utilities_include()));

    replace_all(text, replacement{"::?_class", m_QualifiedName},
                      replacement{"?Class.hpp", header_path().generic_string()},
                      replacement{"?Class", type_file_stem()},
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
    if(surname().empty())
      surname(std::string{"allocation_"}.append(to_surname(flavour())));
    set_type_name(forename());

    // An allocation test takes no --gen-source, so its header is never generated.
    nascent_test_base::finalize([](const fs::path& p) { return p; },
                                [](const fs::path&) {},
                                {},
                                to_stubs(*this),
                                test_classes(),
                                "MyClass",
                                [this](std::string& text) { transform_file(text); });
  }

  [[nodiscard]]
  std::vector<std::string> nascent_allocation_test::test_classes() const
  {
    return { test_name() };
  }

  void nascent_allocation_test::transform_file(std::string& text) const
  {
    tabs_to_spacing(text, code_indent());

    make_common_replacements(text);

    constexpr std::string_view testCore{"#include \"sequoia/TestFramework/?AllocationTestCore.hpp\""};
    if(!testing_utilities().empty())
      replace_all(text, testCore, std::format("#include \"{}\"\n\n{}", testing_utilities_include(), testCore));

    replace_all(text, replacement{"?Allocation", to_camel_case(test_type())},
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
    if(full_name())
    {
      if(!forename().empty())
        throw std::runtime_error{"--forename and --fullname both name the test class: give one or the other"};

      if(flavour() == nascent_test_flavour::framework_diagnostics)
        throw std::runtime_error{"--fullname names one test class, but --framework-diagnostics creates two"};
    }

    const auto fallbackSuite{capitalize(forename().empty() ? header().filename().replace_extension().string() : forename())};

    if(forename().empty())
      forename(to_snake_case(fallbackSuite));

    if(surname().empty())
      surname(std::string{test_type()}.append("_").append(to_surname(flavour())));

    nascent_test_base::finalize([this](const fs::path& filename) { return where_header_absent(filename); },
                                [this](const fs::path& headerPath) { generate_header(headerPath); },
                                {},
                                to_stubs(*this),
                                test_classes(),
                                "MyBehavioural",
                                [this](std::string& text) { transform_file(text); });
  }

  [[nodiscard]]
  std::filesystem::path nascent_behavioural_test::where_header_absent(const std::filesystem::path& filename) const
  {
    const auto& project{paths().source().project()};
    return filename.is_absolute() ? filename : project / rebase_from(filename, project);
  }

  void nascent_behavioural_test::generate_header(const std::filesystem::path& headerPath)
  {
    stream() << fs::relative(headerPath, paths().project_root()).generic_string() << '\n';
    fs::create_directories(headerPath.parent_path());
    fs::copy_file(paths().aux_paths().source_templates() / "MyFreeFunctions.hpp", headerPath);

    read_modify_write(headerPath, [&nameSpace = m_Namespace, copyright{copyright()}](std::string& text) {
        process_copyright_and_namespace(text, copyright, nameSpace);
      }
    );

    set_cpp(headerPath, m_Namespace);
  }

  [[nodiscard]]
  std::vector<std::string> nascent_behavioural_test::test_classes() const
  {
    auto diagnostics{
      [this](std::string_view polarity) { return std::format("{}_{}_{}", forename(), polarity, surname()); }
    };

    switch(flavour())
    {
    case nascent_test_flavour::standard:
      return { test_name() };
    case nascent_test_flavour::framework_diagnostics:
      return { diagnostics("false_positive"), diagnostics("false_negative")};
    }

    throw std::logic_error{"Unrecognized option for nascent_test_flavour"};
  }

  void nascent_behavioural_test::transform_file(std::string& text) const
  {
    make_common_replacements(text);

    replace_all(text, replacement{"?Test", to_camel_case(test_type()).append("Test")},
                      replacement{"?Header.hpp", header_path().generic_string()},
                      replacement{"?", test_type()});
  }
}
