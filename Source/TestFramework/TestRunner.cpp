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

#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>
#include <optional>

namespace sequoia::testing
{
  using parsing::commandline::warning;

  [[nodiscard]]
  std::string report_time(const test_family::summary& s)
  {
    return report_time(s.log, s.execution_time);
  }

  [[nodiscard]]
  std::string report_file_issue(const std::filesystem::path& file, std::string_view description)
  {
    auto mess{std::string{"Unable to open file "}.append(file)};
    if(!description.empty()) mess.append(" ").append(description);
    mess.append("\n");

    return mess;
  }

  [[nodiscard]]
  std::string report_failed_read(const std::filesystem::path& file)
  {
    return report_file_issue(file, " for reading");
  }

  [[nodiscard]]
  std::string report_failed_write(const std::filesystem::path& file)
  {
    return report_file_issue(file, " for writing");
  }

  [[nodiscard]]
  std::string compare_files(const std::filesystem::path& file, const std::filesystem::path& prediction, const test_mode mode)
  {
    enum class file_comparison {failed, same, different};

    auto fcomp{file_comparison::failed};
    std::string info{};
    
    std::ifstream file1{file}, file2{prediction};
    if(!file1) info = warning(report_failed_read(file));
    if(!file2) info = warning(report_failed_read(prediction));
    
    if(file1 && file2)
    {
      std::stringstream buffer1{}, buffer2{};
      buffer1 << file1.rdbuf();
      buffer2 << file2.rdbuf();

      fcomp = (buffer1.str() == buffer2.str()) ? file_comparison::same : file_comparison::different;
    }

    switch(mode)
    {    
    case test_mode::false_positive:
      switch(fcomp)
      {
      case file_comparison::same:
        info = append_lines(warning("Contents of" ), file.string(), "spuriously comparing equal to", prediction.string()).append("\n");
        break;
      case file_comparison::different:
        info = "passed";
        break;
      case file_comparison::failed:
        info = warning("Unable to perform false-positive test");
        break;
      }
      break;
    default:
      switch(fcomp)
      {
      case file_comparison::same:
        info = "passed";
        break;
      case file_comparison::different:
        info = append_lines(warning("Contents of" ), file.string(), "no longer matches", prediction.string()).append("\n");
        break;
      case file_comparison::failed:
        info = warning("Unable to perform file comparison");
        break;
      }
      break;
    }

    return info;
  }

  std::filesystem::path host_directory::get([[maybe_unused]] std::string_view filename) const
  {
    namespace fs = std::filesystem;
    
    variant_visitor visitor{
      [](const fs::path& p) { return p; },
      [filename](const generator& data){

        if(auto sourcePath{data.sourceRepo.find(filename)}; !sourcePath.empty())
        {
          const auto dir{data.hostRepo / fs::relative(sourcePath, data.sourceRepo.root()).parent_path()};
          fs::create_directories(dir);
          
          return dir;
        }

        throw std::runtime_error{std::string{"Unable to locate file "}.append(filename).append(" in the source repository\n").append(data.sourceRepo.root())};
      }
    };

    return std::visit(visitor, m_Data);
  }

  //=========================================== nascent_test ===========================================//

  nascent_test::nascent_test(std::string_view testType, std::string_view qualifiedName, std::vector<std::string> equivalentTypes, const host_directory& hostDir, std::string_view overriddenFamily, std::string_view overriddenClassHeader)
    : m_QualifiedClassName{qualifiedName}
    , m_TestType{testType}
    , m_EquivalentTypes{std::move(equivalentTypes)}
  {
    constexpr auto npos{std::string::npos};

    auto start{npos};
    if(auto pos{m_QualifiedClassName.rfind("::")}; pos != npos)
    {
      if(pos < m_QualifiedClassName.length() - 2)
      {
        start = pos+2;
        m_RawClassName = m_QualifiedClassName.substr(start);
      }
    }
    else
    {      
      m_RawClassName = m_QualifiedClassName;
      start = 0;
    }

    m_TemplateData = generate_template_data(m_RawClassName);
    if(!m_TemplateData.empty())
    {
      if(auto pos{m_RawClassName.find('<')}; pos != npos)
      {
        m_RawClassName.erase(pos);

        if(start != npos)
        {
          m_QualifiedClassName.erase(start + pos);

          std::string args{"<"};
          std::for_each(m_TemplateData.cbegin(), m_TemplateData.cend(),
            [&args](const template_spec& d) { args.append(d.name).append(","); }
          );

          args.back() = '>';

          m_QualifiedClassName.append(args);
        }
      }
    }

    m_ClassHeader = !overriddenClassHeader.empty() ? overriddenClassHeader : to_camel_case(m_RawClassName).append(".hpp");
    if(!overriddenFamily.empty())
    {
      m_Family = overriddenFamily;
    }
    else
    {
      m_Family = m_RawClassName;
      replace_all(m_Family, "_", " ");
    }

    m_HostDirectory = hostDir.get(m_ClassHeader);
  }

  [[nodiscard]]
  auto nascent_test::generate_template_data(std::string_view str) -> std::vector<template_spec>
  {
    std::vector<template_spec> decomposition{};
    
    constexpr auto npos{std::string::npos};
    if(auto openPos{str.find('<')}; openPos != npos)
    {
      if(auto closePos{str.rfind('>')}; closePos != npos)
      {
        if((closePos < openPos) || (closePos - openPos < 2))
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

  [[nodiscard]]
  auto nascent_test::generate_template_spec(std::string_view str) -> template_spec
  {    
    constexpr auto npos{std::string::npos};
    const auto endOfLastToken{str.find_last_not_of(" .")};
    if(endOfLastToken == npos)
      throw std::runtime_error(std::string{str}.append(" Unable to locate end of final token"));
    
    const auto beforeLastToken{str.substr(0, endOfLastToken).rfind(' ')};
    if(beforeLastToken == npos)
      throw std::runtime_error(std::string{str}.append(" Unable to locate start of final token"));

    const auto lastTokenSize{endOfLastToken - beforeLastToken};
    const auto endOfLastTemplateSpec{str.substr(0, str.size() - lastTokenSize).find_last_not_of(" .")};
    if(endOfLastTemplateSpec == npos)
      return {"", std::string{str}};
    
    return {std::string{str.substr(0, endOfLastTemplateSpec + 1)}, std::string{str.substr(beforeLastToken + 1, lastTokenSize)}};
  }

  [[nodiscard]]
  std::filesystem::path nascent_test::create_file(std::string_view copyright, std::string_view partName, const std::filesystem::copy_options options) const
  {
    namespace fs = std::filesystem;
    
    const auto outputFile{(m_HostDirectory / to_camel_case(m_RawClassName)) += partName};

    if(((options & fs::copy_options::skip_existing) == fs::copy_options::skip_existing) && fs::exists(outputFile))
    {
      return warning(outputFile.string()).append(" already exists, so not created\n");
    }

    auto makePath{
      [partName](){
        return aux_path("UnitTestCodeTemplates").append("CodeTemplates").append("MyClass").concat(partName);
      }
    };

    testing::create_file(makePath(), outputFile, [this, copyright](const fs::path& file) { transform_file(file, copyright); });
    return outputFile;
  }

  [[nodiscard]]
  std::string nascent_test::compare_files(std::string_view partName) const
  {
    namespace fs = std::filesystem;

    const auto className{to_camel_case(m_RawClassName).append(partName)};

    const auto file{self_diag_output_path("UnitTestCreationDiagnostics").append(className)};
    const auto prediction{aux_path("UnitTestCodeTemplates").append("ReferenceExamples").append(className)};

    return testing::compare_files(file, prediction, test_mode::standard);
  }

  void nascent_test::transform_file(const std::filesystem::path& file, std::string_view copyright) const
  {
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
        
    if(!text.empty())
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
        throw std::runtime_error("Unable to locate Copyright information");
      }

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
          if(!d.parameter.empty()) spec.append(d.parameter).append(" ").append(d.name).append(",");
        }

        spec.back() = '>';
        spec.append("\n ");

        replace_all(text, "<?>", spec);
      }
      else
      {
        replace_all(text, "template<?> ", "");
      }

      replace_all(text, "::?_class", m_QualifiedClassName);
      replace_all(text, "?_class", m_RawClassName);
      replace_all(text, "?_test", m_TestType);
      replace_all(text, "?Test", to_camel_case(m_TestType));
      replace_all(text, "?Class.hpp", m_ClassHeader);
      replace_all(text, "?Class", to_camel_case(m_RawClassName));

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

  //=========================================== test_runner ===========================================//

  test_runner::test_runner(int argc, char** argv, std::string_view copyright, std::filesystem::path testMain, std::filesystem::path hashIncludeTarget, std::filesystem::path testRepo, std::filesystem::path testMaterialsRepo, std::filesystem::path sourceRepo)
    : m_Copyright{copyright}
    , m_TestMain{std::move(testMain)}
    , m_HashIncludeTarget{std::move(hashIncludeTarget)}
    , m_TestRepo{std::move(testRepo)}
    , m_MaterialsSearchTree{testMaterialsRepo}
    , m_SourceSearchTree{std::move(sourceRepo)}
  {
    throw_unless_regular_file(m_TestMain, "\nEnsure the application is run from the appropriate directory");
    throw_unless_regular_file(m_HashIncludeTarget);    
    throw_unless_directory(m_TestRepo);

    process_args(argc, argv);

    if(m_Pause)
    {
      std::cout << "Please hit enter to continue...\n";
      while(std::cin.get() != '\n'){}
    }

    check_argument_consistency();
    //clean_temporary_output();
    //run_diagnostics();
  }

  void test_runner::process_args(int argc, char** argv)
  {    
    using namespace parsing::commandline;

    std::vector<std::string> equivalentTypes{};
    host_directory host{m_TestRepo, m_SourceSearchTree};
    std::string family{}, classHeader{};
    
    invoke_late(argc, argv,
                { {"test", {"t"}, {"test_family_name"},
                   [this](const param_list& args) {
                     m_SelectedFamilies.emplace(args.front(), false); }
                  },
                  {"source", {"s"}, {"source_file_name"},
                   [this](const param_list& args) {
                     m_SelectedSources.emplace(args.front(), false); }
                  },
                  {"create", {"c"}, { "test type", "qualified::class_name<class T>", "equivalent type" },
                   [this,&equivalentTypes,&host,&family,&classHeader](const param_list& args) {
                       equivalentTypes.push_back(args[2]);
                       std::reverse(equivalentTypes.begin(), equivalentTypes.end());
                       m_NascentTests.push_back(nascent_test{args[0], args[1], equivalentTypes, host, family, classHeader});
                       
                       equivalentTypes.clear();
                       host = host_directory{m_TestRepo, m_SourceSearchTree};
                       family.clear();
                       classHeader.clear();
                     },
                     { {"--equivalent-type", {"-e"}, {"equivalent type"},
                         [&equivalentTypes](const param_list& args){
                           equivalentTypes.push_back(args[0]);
                         }
                       },
                       {"--host-directory", {"-h"}, {"host directory"},
                           [&host](const param_list& args){ host = host_directory{args[0]};}
                       },
                       {"--family", {"-f"}, {"family"},
                           [&family](const param_list& args){ family = args[0]; }
                       },
                       {"--class-header", {"-ch"}, {"class header"},
                           [&classHeader](const param_list& args){ classHeader = args[0]; }
                       }
                     }
                  },
                  {"--async", {"-a"}, {},
                   [this](const param_list&) {
                     if(m_ConcurrencyMode == concurrency_mode::serial)
                       m_ConcurrencyMode = concurrency_mode::family; }
                  },
                  {"--async-depth", {"-ad"}, {"depth [0-2]"},
                   [this](const param_list& args) {
                     const int i{std::clamp(std::stoi(args.front()), 0, 2)};
                     m_ConcurrencyMode = static_cast<concurrency_mode>(i); }
                  },
                  {"--verbose",  {"-v"}, {}, [this](const param_list&) { m_Verbose    = true; }},          
                  {"--nofiles",  {"-n"}, {}, [this](const param_list&) { m_OutputMode = output_mode::write_files; }},
                  {"--pause",    {"-p"}, {}, [this](const param_list&) { m_Pause      = true;  }},
                  {"--recovery", {"-r"}, {},
                   [] (const param_list&) {
                     output_manager::recovery_file(output_path("Recovery").append("Recovery.txt")); }}
                });
  }

  void test_runner::check_argument_consistency() const
  {
    using parsing::commandline::error;

    if(concurrent_execution() && !output_manager::recovery_file().empty())
      throw std::runtime_error{error("Can't run asynchronously in recovery mode\n")};
  }

  void test_runner::clean_temporary_output()
  {
    namespace fs = std::filesystem;

    std::cout << "Cleaning temporary files...\n\n";

    fs::remove_all(self_diag_output_path(""));
    fs::create_directory(self_diag_output_path(""));
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
  }

  void test_runner::run_diagnostics()
  {    
    std::cout << "Running self-diagnostics...\n";

    false_positive_check();    
    test_file_editing();
    test_creation();
    test_full_creation();
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
    for(const auto& s : results.logs)
    {
      if(m_Verbose) output += summarize(s, log_verbosity::failure_messages, tab, tab);
      familySummary.log += s;
    }
          
    if(m_Verbose)
    {
      output.insert(0, report_time(familySummary));
    }
    else
    {
      output += summarize(familySummary, log_verbosity::failure_messages, tab, no_indent);
    }

    std::cout << output;

    return familySummary;
  }

  void test_runner::check_for_missing_tests() const
  {
    auto check{
      [](const selection_map& tests, std::string_view type) {
        for(const auto& test : tests)
        {
          if(!test.second)
          {
            std::cout << warning(std::string{"Test "}.append(type).append(" '").append(test.first).append("' not found\n"));
          }
        }
      }
    };

    check(m_SelectedFamilies, "Family");
    check(m_SelectedSources, "File");
  }
                       
  void test_runner::execute()
  {
    namespace fs = std::filesystem;

    report("Creating files...\n", create_files(m_NascentTests.cbegin(), m_NascentTests.cend(), fs::copy_options::skip_existing));

    run_tests();
  }

  template<class Iter, class Fn>
  [[nodiscard]]
  std::string test_runner::process_nascent_tests(Iter beginNascentTests, Iter endNascentTests, Fn fn)
  {
    if(std::distance(beginNascentTests, endNascentTests))
    {
      std::string mess{};
      while(beginNascentTests != endNascentTests)
      {
        const auto& data{*beginNascentTests};
        for(const auto& stub : st_TestNameStubs)
        {
          append_lines(mess, fn(data, stub));
        }

        ++beginNascentTests;
      }

      return mess;
    }

    return "";
  }

  template<class Iter>
  [[nodiscard]]
  std::string test_runner::create_files(Iter beginNascentTests, Iter endNascentTests, const std::filesystem::copy_options options) const
  {
    auto action{
                [options,&copyright{m_Copyright},&target{m_HashIncludeTarget}](const nascent_test& data, std::string_view stub){
        const auto filePath{data.create_file(copyright, stub, options)};
        
        if(const auto filename{filePath.filename()};
           (filename.extension() == ".hpp") && (filename.string().find("Utilities") == std::string::npos))
        {
          add_include(target, filename.string());
        }
        
        return std::string{"\""}.append(filePath).append("\"");
      }
    };
    
    return process_nascent_tests(beginNascentTests, endNascentTests, action);
  }
  
  template<class Iter>
  [[nodiscard]]
  std::string test_runner::compare_files(Iter beginNascentTests, Iter endNascentTests)
  {
    auto action{
      [](const nascent_test& data, std::string_view stub){
        return data.compare_files(stub);
      }
    };

    return process_nascent_tests(beginNascentTests, endNascentTests, action);    
  }

  void test_runner::report(std::string_view prefix, std::string_view message)
  {
    if(!message.empty())
    {
      sentinel block_1{*this};
      std::cout << block_1.indent(prefix) << '\n';

      sentinel block_2{*this};
      std::cout << block_2.indent(message) << "\n\n";
    }
  }

  template<class Iter, class Filter>
    requires invocable<Filter, test_runner::messages>
  void test_runner::report(std::string_view prefix, Iter begin, Iter end, Filter filter)
  {
    sentinel block_1{*this};
    std::cout << block_1.indent(prefix) << '\n';

    sentinel block_2{*this};
    std::for_each(begin, end, [&block_2, filter](const messages& mess){
                                std::cout << block_2.indent(filter(mess)) << '\n';
                              });

    std::cout << '\n';
  }

  void test_runner::false_positive_check()
  {
    static_assert(st_TestNameStubs.size() > 1, "Insufficient data for false-positive test");

    sentinel block_0{*this};
    std::cout << block_0.indent("Running false-positive tests...\n");

    auto partPath{
      [](){ return aux_path("UnitTestCodeTemplates").append("CodeTemplates").append("MyClass"); }
    };

    const auto file1{partPath().concat(st_TestNameStubs[0])};
    const auto file2{partPath().concat(st_TestNameStubs[1])};

    report("File comparison:", testing::compare_files(file1, file2, test_mode::false_positive));
  }

  void test_runner::test_file_editing()
  {
    namespace fs = std::filesystem;

    sentinel block_0{*this};
    
    std::cout << block_0.indent("Running file editing diagnostics...\n");

    fs::create_directory(self_diag_output_path("FileEditingOutput"));

    const std::array<messages, 3> mess{
      test_file_editing("Includes.hpp",
                        [](const fs::path& sandboxFile){
                          add_include(sandboxFile, "Bar.hpp");
                        }
      ),
      test_file_editing("FakeMain.cpp",
                        [](const fs::path& sandboxFile){
                          add_to_family(sandboxFile, "New Family",
                                        {{"fake_false_positive_test{\"False Positive Test\"}"}, {"fake_test{\"Unit Test\"}"}});
                        }
      ),
      test_file_editing("FakeMain2.cpp",
                        [](const fs::path& sandboxFile){
                          add_to_family(sandboxFile, "CommandLine Arguments",
                                        {{"commandline_arguments_false_positive_test{\"False Positive Test\"}"}});
                        }
      )
    };

    report("Files built:", mess.cbegin(), mess.cend(), [](const messages& m) { return m.creation;});
    report("Comparisons against reference files:", mess.cbegin(), mess.cend(), [](const messages& m) { return m.comparison;});
  }

  void test_runner::test_creation(std::string_view qualifiedName, std::vector<std::string> equivalentTypes)
  {
    namespace fs = std::filesystem;

    const std::array<nascent_test, 1>
      diagnosticFiles{nascent_test{"regular_test", qualifiedName, std::move(equivalentTypes), self_diag_output_path("UnitTestCreationDiagnostics")}};

    report("Files built:", create_files(diagnosticFiles.cbegin(), diagnosticFiles.cend(), fs::copy_options::overwrite_existing));
    report("Comparisons against reference files:", compare_files(diagnosticFiles.cbegin(), diagnosticFiles.cend()));
  }

  void test_runner::test_creation()
  {
    /*sentinel block_0{*this};
    
    std::cout << block_0.indent("Running test creation tool diagnostics...\n");

    std::filesystem::create_directory(self_diag_output_path("UnitTestCreationDiagnostics"));

    test_creation("utilities::iterator", {"int*"});
    test_creation("bar::baz::foo<class T>", {"T"});*/
  }

  void test_runner::test_full_creation()
  {
    /*
    namespace fs = std::filesystem;

    sentinel block_0{*this};
    
    std::cout << block_0.indent("Running full test creation diagnostics...\n");

    fs::copy(aux_path("FakeProject"), self_diag_output_path("FakeProject"), std::filesystem::copy_options::recursive);
    
    const auto testMain{self_diag_output_path("FakeProject").append("TestSandbox").append("TestSandbox.cpp")};
    const auto includeTarget{self_diag_output_path("FakeProject").append("TestShared").append("SharedIncludes.hpp")};
    const auto testRepo{self_diag_output_path("FakeProject").append("Tests")};
    const auto sourceRepo{self_diag_output_path("FakeProject").append("Source")};

    using namespace parsing::commandline;
    commandline_arguments args{"", "create", "ordered_test", "other::functional::maybe<class T>", "std::optional<T>"
                                 , "create", "ordered_test", "utilities::iterator", "int*"
                                 , "create", "move_only_test", "bar::baz::foo<class T>", "T"
                                 , "create", "regular_test", "other::couple<class S, class T>", "S", "-e", "T",
                                      "-h", (testRepo / "Partners").string(), "-f", "partners", "-ch", "Couple.hpp"};
    
    test_runner tr{args.size(), args.get(), "Oliver J. Rosten", testMain, includeTarget, testRepo, sourceRepo, suppress_diagnostics{}};

    tr.execute();
    */
  }

  template<class Fn>
    requires invocable<Fn, std::filesystem::path>
  [[nodiscard]]
  auto test_runner::test_file_editing(std::string_view fileName, Fn action) -> messages
  {
    namespace fs = std::filesystem;

    messages m{};

    const auto source{aux_path("FileEditingTestMaterials").append("BeforeEditing").append(fileName)};
    const auto target{self_diag_output_path("FileEditingOutput").append(fileName)};

    create_file(source, target, action);
    m.creation = target.string();

    const auto prediction{aux_path("FileEditingTestMaterials").append("AfterEditing").append(fileName)};

    m.comparison = testing::compare_files(target, prediction, test_mode::standard);

    return m;
  }

  void test_runner::run_tests()
  {
    using namespace std::chrono;
    const auto time{steady_clock::now()};

    if(!m_Families.empty() && (m_NascentTests.empty() || !m_SelectedFamilies.empty()))
    {
      std::cout << "Running tests...\n";
      log_summary summary{};
      if(!concurrent_execution())
      {
        for(auto& family : m_Families)
        {
          std::cout << family.name() << ":\n";
          summary += process_family(family.execute(m_OutputMode, m_ConcurrencyMode)).log;
        }
      }
      else
      {
        std::cout << "\n\t--Using asynchronous execution, level: " << stringify(m_ConcurrencyMode) << "\n\n";
        std::vector<std::pair<std::string, std::future<test_family::results>>> results{};
        results.reserve(m_Families.size());

        for(auto& family : m_Families)
        {
          results.emplace_back(family.name(),
            std::async([&family, omode{m_OutputMode}, cmode{m_ConcurrencyMode}](){
                return family.execute(omode, cmode); }));
        }

        for(auto& res : results)
        {
          std::cout << res.first << ":\n";
          summary += process_family(res.second.get()).log;
        }
      }
      std::cout <<  "\n-----------Grand Totals-----------\n";
      std::cout << summarize(summary, steady_clock::now() - time, log_verbosity::absent_checks, indentation{"\t"},  no_indent);
    }

    check_for_missing_tests();
  }
}
