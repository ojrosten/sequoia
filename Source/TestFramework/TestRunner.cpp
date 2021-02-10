////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for TestRunner.hpp.
*/

#include "TestRunner.hpp"
#include "CommandLineArguments.hpp"
#include "Summary.hpp"
#include "FileEditors.hpp"

namespace sequoia::testing
{ 
  using namespace parsing::commandline;

  [[nodiscard]]
  std::string report_time(const test_family::summary& s)
  {
    return report_time(s.log, s.execution_time);
  }

  [[nodiscard]]
  std::filesystem::path host_directory::get([[maybe_unused]] const std::filesystem::path& filename) const
  {
    namespace fs = std::filesystem;
    
    variant_visitor visitor{
      [](const fs::path& p) { return p; },
      [&filename](const generator& data){
        const auto sourcePath{
          [&](){
            if(const auto path{data.sourceRepo.find(filename)}; !path.empty())
              return path;

            const auto ext{filename.extension()};
            const auto alternative{std::filesystem::path{filename}.replace_extension(ext == ".hpp" ? ".h" : ".hpp")};
            if(const auto path{data.sourceRepo.find(alternative)}; !path.empty())
              return path;

            throw std::runtime_error{std::string{"Unable to locate file "}
                                   .append(filename.generic_string()).append(" or ")
                                   .append(alternative.generic_string()).append(" in the source repository\n")
                                   .append(fs::relative(data.sourceRepo.root(), data.hostRepo).generic_string())};
          }()
        };
        
        const auto dir{data.hostRepo / fs::relative(sourcePath, data.sourceRepo.root()).parent_path()};
        fs::create_directories(dir);

        return dir;
      }
    };

    return std::visit(visitor, m_Data);
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

      const auto now{std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())};
      const auto year{std::to_string(1900 + std::localtime(&now)->tm_year)};

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

  void nascent_test_base::finalize_family()
  {
    if(m_Family.empty())
    {
      m_Family = m_CamelName;
      replace_all(m_Family, "_", " ");
    }
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

      if(std::ofstream ofile{outputFile})
      {
        ofile << text;
      }
      else
      {
        throw std::runtime_error{report_failed_write(outputFile)};
      }
    }

    return {outputFile, true};
  }

  //=========================================== nascent_semantics_test ===========================================//

  void nascent_semantics_test::finalize()
  {
    constexpr auto npos{std::string::npos};

    auto start{npos};
    auto templatePos{m_QualifiedName.find('<')};
    
    if(auto pos{m_QualifiedName.rfind("::", templatePos)}; pos != npos)
    {
      if(pos < m_QualifiedName.length() - 2)
      {
        start = pos+2;
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

    nascent_test_base::finalize_family();
  }

  [[nodiscard]]
  auto nascent_semantics_test::create_file(std::string_view copyright, const std::filesystem::path& codeTemplatesDir, std::string_view nameEnding) const -> file_data
  {
    auto transformer{[this](std::string& text) { transform_file(text); }};
    return nascent_test_base::create_file(codeTemplatesDir, copyright, "MyClass", nameEnding, transformer);
  }

  [[nodiscard]]
  std::vector<std::string> nascent_semantics_test::translation_units() const
  {
    return { {std::string{forename()}.append("_false_positive_test(\"False Positive Test\")")},
             {std::string{forename()}.append("_test(\"Unit Test\")")} };
  }

  void nascent_semantics_test::transform_file(std::string& text) const
  {
    if(!m_EquivalentTypes.empty())
    {
      std::string args{};

      const auto num{m_EquivalentTypes.size()};
      for(std::size_t i{}; i < num; ++i)
      {
        const auto& type{m_EquivalentTypes[i]};
        if(!type.empty())
        {
          const bool normal{(type.back() != '*') && (type.back() != '&')};
          if(normal) args.append("const ");

          args.append(type);

          if(normal) args.append("&");
            
          args.append(" prediction");

          if(num > 1) args.append("_").append(std::to_string(i));
          if(i < num -1) args.append(", ");
        }
      }

      replace_all(text, "?predictions", args);
    }
    else
    {
      constexpr auto npos{std::string::npos};
      const auto start{text.rfind("template<?>")};
      const auto finish{text.rfind("};")};
      if((start != npos) && (finish != npos))
      {
        text.erase(start, finish + 2 - start);
      }
    }

    if(!m_TemplateData.empty())
    {
      std::string spec{"<"};
      for(const auto& d : m_TemplateData)
      {
        spec.append(d.species).append(" ").append(d.symbol).append(", ");
      }

      spec.erase(spec.size() - 1);
      spec.back() = '>';
      spec.append("\n ");

      replace_all(text, "<?>", spec);
    }
    else
    {
      replace_all(text, "template<?> ", "");
    }

    replace_all(text, {{"::?_class", m_QualifiedName},
                       {"?_class", forename()},
                       {"?Class", camel_name()},
                       {"?Test", to_camel_case(test_type()).append("Test")},
                       {"?Class.hpp", header().string()},
                       {"?", test_type()}});

  }

  //=========================================== nascent_behavioural_test ===========================================//

  void nascent_behavioural_test::finalize()
  {
    camel_name(header().filename().replace_extension().string());

    nascent_test_base::finalize_family();

    camel_name(std::string{camel_name()}.append(capitalize(test_type())));    
    forename(to_snake_case(camel_name()));
  }

  [[nodiscard]]
  auto nascent_behavioural_test::create_file(std::string_view copyright, const std::filesystem::path& codeTemplatesDir, std::string_view nameEnding) const -> file_data
  {
    auto transformer{[this](std::string& text) { transform_file(text); }};
    return nascent_test_base::create_file(codeTemplatesDir, copyright, "MyBehavioural", nameEnding, transformer);
  }

  [[nodiscard]]
  std::vector<std::string> nascent_behavioural_test::translation_units() const
  {
    return { {std::string{forename()}.append("_test(\"").append(to_camel_case(test_type())).append(" Test\")")} };
  }

  void nascent_behavioural_test::transform_file(std::string& text) const
  {
    replace_all(text, {{"?_behavioural", forename()},
                       {"?Behavioural", camel_name()},
                       {"?", test_type()}});
  }

  //=========================================== nascent_allocation_test ===========================================//

  void nascent_allocation_test::finalize()
  {
    camel_name(forename());
    if(header().empty()) header(std::filesystem::path{camel_name()}.concat(".hpp"));

    nascent_test_base::finalize_family();
  }

  [[nodiscard]]
  auto nascent_allocation_test::create_file(std::string_view copyright, const std::filesystem::path& codeTemplatesDir, std::string_view nameEnding) const -> file_data
  {
    auto transformer{[this](std::string& text) { transform_file(text); }};
    return nascent_test_base::create_file(codeTemplatesDir, copyright, "MyClass", nameEnding, transformer);
  }

  [[nodiscard]]
  std::vector<std::string> nascent_allocation_test::translation_units() const
  {
    return { {std::string{forename()}.append("_allocation_test(\"Allocation Test\")")} };
  }

  void nascent_allocation_test::transform_file(std::string& text) const
  {
    replace_all(text, {{"?_class", forename()},
                       {"?Class", camel_name()},
                       {"?", test_type()}});
  }

  //=========================================== test_runner ===========================================//

  void test_runner::test_creator::operator()(const parsing::commandline::param_list& args)
  {
    auto& nascentTests{runner.m_NascentTests};

    static creation_factory factory{{"semantic", "allocation", "behavioural"}, runner.m_TestRepo, runner.m_SourceSearchTree};
    auto nascent{factory.create(genus)};

    std::visit(
        variant_visitor{
          [&args,&species{species}](nascent_semantics_test& nascent){
            nascent.test_type(species);
            nascent.qualified_name(args[0]);
            nascent.add_equivalent_type(args[1]);
          },
          [&args,&species{species}](nascent_allocation_test& nascent){
            nascent.test_type(species);
            nascent.forename(args[0]);
          },
          [&args,&species{species}](nascent_behavioural_test& nascent){
            nascent.test_type(species);
            nascent.header(args[0]);
          }
        },
        nascent);

    nascentTests.emplace_back(std::move(nascent));
  }

  test_runner::test_runner(int argc, char** argv, std::string_view copyright, std::filesystem::path testMain, std::filesystem::path hashIncludeTarget, repositories repos, std::ostream& stream)
    : m_Copyright{copyright}
    , m_SourceSearchTree{std::move(repos.source)}
    , m_ProjectRoot{project_root(argc, argv)}
    , m_TestMain{std::move(testMain)}
    , m_HashIncludeTarget{std::move(hashIncludeTarget)}
    , m_TestRepo{std::move(repos.tests)}
    , m_TestMaterialsRepo{std::move(repos.test_materials)}
    , m_OutputDir{std::move(repos.output)}
    , m_Stream{stream}
  {
    throw_unless_regular_file(m_TestMain, "\nTry ensuring that the application is run from the appropriate directory");
    throw_unless_regular_file(m_HashIncludeTarget);
    throw_unless_directory(m_TestRepo);

    process_args(argc, argv);

    namespace fs = std::filesystem;
    fs::create_directory(m_OutputDir);
    fs::create_directory(diagnostics_output_path(m_OutputDir));
    fs::create_directory(test_summaries_path(m_OutputDir));
  }

  void test_runner::process_args(int argc, char** argv)
  {
    const option hostOption{"--host-directory", {"-h"}, {"host_directory"},
        [this](const param_list& args){
          if(m_NascentTests.empty())
            throw std::logic_error{"Unable to find nascent test"};

          std::visit(variant_visitor{[&args](auto& nascent){ nascent.host_dir(args[0]);}}, m_NascentTests.back());
        } 
    };

    const option familyOption{"--family", {"-f"}, {"family"},
        [this](const param_list& args){
          if(m_NascentTests.empty())
            throw std::logic_error{"Unable to find nascent test"};

          std::visit(variant_visitor{[&args](auto& nascent){ nascent.family(args[0]);}}, m_NascentTests.back());
        }
    };

    const option equivOption{"--equivalent-type", {"-e"}, {"equivalent_type"},
        [this](const param_list& args){
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

    const option headerOption{"--class-header", {"-ch"}, {"class_header"},
        [this](const param_list& args){
          if(m_NascentTests.empty())
            throw std::logic_error{"Unable to find nascent test"};

          std::visit(variant_visitor{[&args](auto& nascent){ nascent.header(args[0]); }}, m_NascentTests.back());
        }                              
    };

    const std::vector<option> semanticsOptions{equivOption, hostOption, familyOption, headerOption};

    const std::vector<option> allocationOptions{hostOption, familyOption, headerOption};
    
    const auto help{
      parse_invoke_depth_first(argc, argv,
                { {"test", {"t"}, {"test_family_name"},
                    [this](const param_list& args) { m_SelectedFamilies.emplace(args.front(), false); }
                  },
                  {"source", {"s"}, {"source_file_name"},
                    [this](const param_list& args) { m_SelectedSources.emplace(args.front(), false); }
                  },
                  {"create", {"c"}, {}, [](const param_list&) {},
                   { {"regular_test", {"regular"}, {"qualified::class_name<class T>", "equivalent_type"},
                      test_creator{"semantic", "regular", *this}, semanticsOptions
                     },
                     {"move_only_test", {"move_only"}, {"qualified::class_name<class T>", "equivalent_type"},
                      test_creator{"semantic", "move_only", *this}, semanticsOptions
                     },
                     {"regular_allocation_test", {"regular_allocation", "allocation_test"}, {"raw_class_name"},
                      test_creator{"allocation", "regular_allocation", *this}, allocationOptions
                     },
                     {"move_only_allocation_test", {"move_only_allocation"}, {"raw_class_name"},
                      test_creator{"allocation", "move_only_allocation", *this}, allocationOptions
                     },
                     {"free_test", {"free"}, {"header"},
                       test_creator{"behavioural", "free", *this}, {hostOption, familyOption}
                     },
                     {"performance_test", {"performance"}, {"header"},
                       test_creator{"behavioural", "performance", *this}, {hostOption, familyOption}
                     }
                   },
                   [this](const param_list&) { std::visit(variant_visitor{[](auto& nascent){ nascent.finalize();}}, m_NascentTests.back()); }
                  },
                  {"init", {"i"}, {"copyright", "path"},
                    [this](const param_list& args) {
                      init_project(args[0], args[1]);
                    }
                  },                  
                  {"update-materials", {"um"}, {},
                    [this](const param_list&) {
                      if(m_UpdateMode != update_mode::hard)
                        m_UpdateMode = update_mode::soft;
                    },
                   { {{"--hard"}, {}, {},  [this](const param_list&) { m_UpdateMode = update_mode::hard; }} }
                  },
                  {"--async", {"-a"}, {},
                    [this](const param_list&) {
                      if(m_ConcurrencyMode == concurrency_mode::serial)
                        m_ConcurrencyMode = concurrency_mode::family;
                    }
                  },
                  {"--async-depth", {"-ad"}, {"depth [0-2]"},
                    [this](const param_list& args) {
                      const int i{std::clamp(std::stoi(args.front()), 0, 2)};
                      m_ConcurrencyMode = static_cast<concurrency_mode>(i);
                    }
                  },
                  {"--verbose",  {"-v"}, {}, [this](const param_list&) { m_OutputMode |= output_mode::verbose; }},
                  {"--recovery", {"-r"}, {},
                    [recoveryDir{recovery_path(m_OutputDir)}] (const param_list&) {
                      std::filesystem::create_directory(recoveryDir);
                      output_manager::recovery_file(recoveryDir / "Recovery.txt");
                      std::filesystem::remove(output_manager::recovery_file());
                    }
                  },
                  {"--dump", {}, {},
                    [recoveryDir{recovery_path(m_OutputDir)}] (const param_list&) {
                      std::filesystem::create_directory(recoveryDir);
                      output_manager::dump_file(recoveryDir / "Dump.txt");
                      std::filesystem::remove(output_manager::dump_file());
                    }
                  }
                },
                [](std::string_view){})
        };

    if(!help.empty())
    {
      m_OutputMode |= output_mode::help;
      m_Stream << help;
    }
    else
    {    
      check_argument_consistency();
    }
  }

  void test_runner::check_argument_consistency() const
  {
    using parsing::commandline::error;

    if(concurrent_execution() && (!output_manager::recovery_file().empty() || !output_manager::dump_file().empty()))
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
    case concurrency_mode::deep:
      return "Deep";
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

    m_Stream << output;

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
      [&stream{m_Stream}](const auto& tests, std::string_view type) {
        for(const auto& test : tests)
        {
          if(!test.second)
          {
            stream << warning(std::string{"Test "}.append(type)
                              .append(" '")
                              .append(convert(test.first))
                              .append("' not found\n"));
          }
        }
      }
    };

    check(m_SelectedFamilies, "Family");
    check(m_SelectedSources, "File");
  }

  [[nodiscard]]
  auto test_runner::find_filename(const std::filesystem::path& filename) -> source_map::iterator
  {
    auto found{m_SelectedSources.find(filename)};
    if(found != m_SelectedSources.end())
      return found;
    
    if(!m_TestRepo.empty())
    {
      return std::find_if(m_SelectedSources.begin(), m_SelectedSources.end(),
                          [&filename, repo{m_TestRepo}](const auto& element){
                     const auto& source{element.first};

                     return rebase_from(source, repo) == rebase_from(filename, repo);
                   });
    }
    
    return m_SelectedSources.end();
  }

  void test_runner::execute()
  {
    namespace fs = std::filesystem;

    if(!mode(output_mode::help))
    {
      report("Creating files...\n", create_files());

      run_tests();
    }
  }

  [[nodiscard]]
  std::string test_runner::create_files() const
  {
    std::string mess{};
    for(const auto& nascentVessel : m_NascentTests)
    {
      variant_visitor visitor{
        [&mess,this](const auto& nascent){
          for(const auto& stub : nascent.stubs())
          {
            append_lines(mess, create_file(nascent, stub));
          }

          add_to_family(m_TestMain, nascent.family(), nascent.translation_units());
        }
      };

      std::visit(visitor, nascentVessel);
    }

    return mess;
  }

  template<class Nascent>
  [[nodiscard]]
  std::string test_runner::create_file(const Nascent& nascent, std::string_view stub) const
  {
    namespace fs = std::filesystem;
    auto stringify{[root{m_ProjectRoot}] (const fs::path file) { return fs::relative(file, root).generic_string();  }};

    const auto[outputFile, created]{nascent.create_file(m_Copyright, code_templates_path(m_ProjectRoot), stub)};

    if(created)
    {
      if(const auto filename{outputFile.filename()}; outputFile.extension() == ".hpp")
      {
        if(const auto str{outputFile.string()}; str.find("Utilities.hpp") == std::string::npos)
        {
          add_include(m_HashIncludeTarget, filename.string());
        }
      }

      return std::string{"\""}.append(stringify(outputFile)).append("\"");
    }
    else
    {
      return warning(stringify(outputFile).append(" already exists, so not created\n"));
    }
  }

  void test_runner::report(std::string_view prefix, std::string_view message)
  {
    if(!message.empty())
    {
      m_Stream << prefix << '\n';
      m_Stream << message << "\n\n";
    }
  }

  void test_runner::run_tests()
  {
    using namespace std::chrono;
    const auto time{steady_clock::now()};

    if(!m_Families.empty() && (m_NascentTests.empty() || !m_SelectedFamilies.empty()))
    {
      m_Stream << "\nRunning tests...\n\n";
      log_summary summary{};
      if(!concurrent_execution())
      {
        for(auto& family : m_Families)
        {
          m_Stream << family.name() << ":\n";
          summary += process_family(family.execute(m_UpdateMode, m_ConcurrencyMode)).log;
        }
      }
      else
      {
        m_Stream << "\n\t--Using asynchronous execution, level: " << stringify(m_ConcurrencyMode) << "\n\n";
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
          m_Stream << res.first << ":\n";
          summary += process_family(res.second.get()).log;
        }
      }
      m_Stream <<  "\n-----------Grand Totals-----------\n";
      m_Stream << summarize(summary, steady_clock::now() - time, summary_detail::absent_checks | summary_detail::timings, indentation{"\t"},  no_indent);
    }

    check_for_missing_tests();
  }

  void test_runner::init_project(std::string_view copyright, const std::filesystem::path& path) const
  {
    namespace fs = std::filesystem;

    fs::create_directories(path);
    fs::copy(project_template_path(m_ProjectRoot), path, fs::copy_options::recursive | fs::copy_options::skip_existing);
    fs::copy(aux_files_path(m_ProjectRoot), aux_files_path(path), fs::copy_options::recursive | fs::copy_options::skip_existing);

    generate_test_main(copyright, path);
    generate_make_file(path);
    generate_make_file(project_template_path(path));
  }

  void test_runner::generate_test_main(std::string_view copyright, const std::filesystem::path& path) const
  {
    const auto file{path/"TestAll"/"TestMain.cpp"}; 
    std::string text{};
    if(std::ifstream ifile{file})
    {
      std::stringstream buffer{};
      buffer << ifile.rdbuf();
      text = buffer.str();
    }
    else
    {
      throw std::runtime_error{report_failed_read(file)};
    }

    set_top_copyright(text, copyright);

    std::string_view myCopyright{"Oliver J. Rosten"};
    if(auto pos{text.find(myCopyright)}; pos != std::string::npos)
    {
      text.replace(pos, myCopyright.size(), copyright);
    }

    if(std::ofstream ofile{file})
    {
      ofile << text;
    }
    else
    {
      throw std::runtime_error{report_failed_write(file)};
    }
  }

  void test_runner::generate_make_file(const std::filesystem::path& path) const
  {
    const auto file{path/"TestAll"/"makefile"}; 
    std::string text{};
    if(std::ifstream ifile{file})
    {
      std::stringstream buffer{};
      buffer << ifile.rdbuf();
      text = buffer.str();
    }
    else
    {
      throw std::runtime_error{report_failed_read(file)};
    }

    constexpr auto npos{std::string::npos};
    if(auto rootPos{text.find("???")}; rootPos != npos)
    {
      namespace fs = std::filesystem;
      text.replace(rootPos, 3, m_ProjectRoot.generic_string());
    }
    else
    {
      throw std::runtime_error{"Unable to locate makefile root definition"};
    }

    if(std::ofstream ofile{file})
    {
      ofile << text;
    }
    else
    {
      throw std::runtime_error{report_failed_write(file)};
    }
  }
}
