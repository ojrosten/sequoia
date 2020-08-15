////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
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
  std::string compare_files(const std::filesystem::path& file, const std::filesystem::path& prediction, const test_mode mode, std::string_view indent)
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
        info = warning("Contents of" );
        append_indented(info, file.string(), indent);
        append_indented(info, "spuriously comparing equal to", indent);
        append_indented(info, prediction.string(), indent);
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
        info = warning("Contents of" );
        append_indented(info, file.string(), indent);
        append_indented(info, "no longer matches", indent);
        append_indented(info, prediction.string(), indent);
        break;
      case file_comparison::failed:
        info = warning("Unable to perform file comparison");
        break;
      }
      break;
    }

    return info;
  }

  //=========================================== nascent_test ===========================================//

  nascent_test::nascent_test(std::filesystem::path dir, std::string family, std::string qualifiedName)
    : m_Directory{std::move(dir)}
    , m_Family{std::move(family)}
    , m_QualifiedClassName{std::move(qualifiedName)}
  {          
    if(auto pos{m_QualifiedClassName.rfind("::")}; pos != std::string::npos)
    {
      if(pos < m_QualifiedClassName.length() - 2)
      {            
        m_ClassName = m_QualifiedClassName.substr(pos+2);
      }
    }
    else
    {
      m_ClassName = m_QualifiedClassName;
    }
  }

  [[nodiscard]]
  std::string nascent_test::create_file(std::string_view partName, const std::filesystem::copy_options options) const
  {
    namespace fs = std::filesystem;
    
    const auto outputFile{(m_Directory / to_camel_case(m_ClassName)) += partName};

    if(((options & fs::copy_options::skip_existing) == fs::copy_options::skip_existing) && fs::exists(outputFile))
    {
      return warning(outputFile.string()).append(" already exists, so not created\n");
    }

    auto makePath{
      [partName](){
        return get_aux_path("UnitTestCodeTemplates").append("CodeTemplates").append("MyClass").concat(partName);
      }
    };

    testing::create_file(makePath(), outputFile, [this](const fs::path& file) { transform_file(file); });
    return outputFile;
  }

  [[nodiscard]]
  std::string nascent_test::compare_files(std::string_view partName) const
  {
    namespace fs = std::filesystem;

    const auto className{to_camel_case(m_ClassName).append(partName)};

    const auto file{get_output_path("UnitTestCreationDiagnostics").append(className)};
    const auto prediction{get_aux_path("UnitTestCodeTemplates").append("ReferenceExamples").append(className)};

    return testing::compare_files(file, prediction, test_mode::standard);
  }

  void nascent_test::transform_file(const std::filesystem::path& file) const
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
      replace_all(text, "::my_class", m_QualifiedClassName);
      replace_all(text, "my_class", m_ClassName);
      replace_all(text, "MyClass", to_camel_case(m_ClassName));

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

  test_runner::test_runner(int argc, char** argv)
  {
    using namespace parsing::commandline;

    const auto operations{parse(argc, argv, {
          {"test",       {[this](const param_list& args) {
                m_SelectedFamilies.emplace(args.front(), false);
              },         {"test_family_name"}, {"t"}} },
          {"source",       {[this](const param_list& args) {
                m_SelectedSources.emplace(args.front(), false);
              },         {"source_file_name"}, {"s"}} },
          {"create",     {[this](const param_list& args) {
                m_NascentTests.push_back(nascent_test{args[0], args[1], args[2]});
              }, { "directory", "family", "qualified class name" }, {"c"} } },
          {"--async",    {[this](const param_list&) {
                if(m_ConcurrencyMode == concurrency_mode::serial)
                  m_ConcurrencyMode = concurrency_mode::family;
              }, {}, {"-a"}} },
          {"--async-depth", {[this](const param_list& args) {
                const int i{std::clamp(std::stoi(args.front()), 0, 2)};
                m_ConcurrencyMode = static_cast<concurrency_mode>(i);
              }, {"depth [0-2]"}, {"-ad"}} },
          {"--verbose",  {[this](const param_list&) { m_Verbose    = true;  }, {}, {"-v"} } },          
          {"--nofiles",  {[this](const param_list&) { m_WriteFiles = false; }, {}, {"-n"} } },
          {"--pause",    {[this](const param_list&) { m_Pause      = true;  }, {}, {"-p"} } },
          {"--recovery", {[]    (const param_list&) {
                output_manager::recovery_file(get_output_path("Recovery").append("Recovery.txt")); }, {}, {"-r"}} }
        })
    };
    
    for(const auto& ops : operations)
    {
      ops.fn(ops.parameters);
    }

    if(m_Pause)
    {
      std::cout << "Please hit enter to continue...\n";
      while(std::cin.get() != '\n'){}
    }

    check_argument_consistency();   
    run_diagnostics();
  }

  void test_runner::check_argument_consistency()
  {
    using parsing::commandline::error;

    if(concurrent_execution() && !output_manager::recovery_file().empty())
      throw std::runtime_error{error("Can't run asynchronously in recovery mode\n")};
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
      if(m_Verbose) output += summarize(s, log_verbosity::failure_messages, "\t", "\t");
      familySummary.log += s;
    }
          
    if(m_Verbose)
    {
      output.insert(0, report_time(familySummary));
    }
    else
    {
      output += summarize(familySummary, log_verbosity::failure_messages, "\t", "");
    }

    std::cout << output;

    return familySummary;
  }

  void test_runner::check_for_missing_tests()
  {
    for(const auto& test : m_SelectedFamilies)
    {
      if(!test.second)
      {
        std::cout << warning("Test Family '" + test.first + "' not found\n");
      }
    }

    for(const auto& test : m_SelectedSources)
    {
      if(!test.second)
      {
        std::cout << warning("Test File '" + test.first + "' not found\n");
      }
    }
  }
                       
  void test_runner::execute()
  {
    namespace fs = std::filesystem;

    std::cout << create_files(m_NascentTests.cbegin(), m_NascentTests.cend(), "Creating files...\n", fs::copy_options::skip_existing);
    run_tests();
  }

  template<class Iter>
  [[nodiscard]]
  std::string test_runner::create_files(Iter beginNascentTests, Iter endNascentTests, std::string_view message, const std::filesystem::copy_options options)
  {
    std::string mess{}; 
    if(std::distance(beginNascentTests, endNascentTests))
    {
      sentinel s{};

      mess = message;
      while(beginNascentTests != endNascentTests)
      {
        const auto& data{*beginNascentTests};
        for(const auto& stub : st_TestNameStubs)
        {
          append_indented(mess, data.create_file(stub, options).append("\"") , st_Indent + "\"");
        }

        ++beginNascentTests;
      }
    }

    return !mess.empty() ? mess.append("\n\n") : mess;
  }
  
  template<class Iter>
  [[nodiscard]]
  std::string test_runner::compare_files(Iter beginNascentTests, Iter endNascentTests, std::string_view message)
  {
    std::string mess{}; 
    if(std::distance(beginNascentTests, endNascentTests))
    {
      sentinel s{};

      mess = message;
      while(beginNascentTests != endNascentTests)
      {
        const auto& data{*beginNascentTests};
        for(const auto& stub : st_TestNameStubs)
        {
          append_indented(mess, data.compare_files(stub), st_Indent);
        }
        
        ++beginNascentTests;
      }    
    }

    return mess.append("\n");
  }

  void test_runner::false_positive_check()
  {
    static_assert(st_TestNameStubs.size() > 1, "Insufficient data for false-positive test");

    sentinel s_0{};

    std::cout << indent("Running false-positive tests...\n", st_Indent);

    sentinel s_1{};
    std::cout << indent("File comparison:\n", st_Indent);

    sentinel s_2{};
    
    const nascent_test data{get_output_path("UnitTestCreationDiagnostics"), "Iterator", "utilities::iterator"};

    auto partPath{
      [&data](){
        return get_output_path("UnitTestCreationDiagnostics").append(to_camel_case(std::string{data.class_name()}));
      }
    };
    
    const auto file1{partPath().concat(st_TestNameStubs[0])};
    const auto file2{partPath().concat(st_TestNameStubs[1])};

    std::cout << indent(testing::compare_files(file1, file2, test_mode::false_positive), st_Indent) << "\n\n";
  }

  void test_runner::test_file_editing()
  {
    namespace fs = std::filesystem;

    sentinel s_0{};
    
    std::cout << indent("Running file editing diagnostics...\n", st_Indent);
    
    for(auto& p : fs::directory_iterator(get_output_path("FileEditingOutput")))
    {
      fs::remove(p.path());
    }

    sentinel s_1{};

    std::cout << indent("Files built:\n", st_Indent);

    std::string info{indent("Comparisons against reference files:", st_Indent)};

    sentinel s_2{};

    append_indented(
      info,
      test_file_editing("Includes.hpp",
                        [](const fs::path& sandboxFile){
                          add_include(sandboxFile, "Bar.hpp");
                        }),
      st_Indent);

    append_indented(
      info,
      test_file_editing("FakeMain.cpp",
                        [](const fs::path& sandboxFile){
                          add_to_family(sandboxFile, "New Family",
                                        {{"fake_false_positive_test{\"False Positive Test\"}"}, {"fake_test{\"Unit Test\"}"}});
                        }),
      st_Indent);

    append_indented(
      info,
      test_file_editing("FakeMain2.cpp",
                        [](const fs::path& sandboxFile){
                          add_to_family(sandboxFile, "CommandLine Arguments",
                                        {{"commandline_arguments_false_positive_test{\"False Positive Test\"}"}});
                        }),
      st_Indent);

    std::cout << "\n" << info << "\n\n";
  }

  template<class Fn>
    requires invocable<Fn, std::filesystem::path>
  [[nodiscard]]
  std::string test_runner::test_file_editing(std::string_view fileName, Fn action)
  {
    namespace fs = std::filesystem;

    const auto source{get_aux_path("FileEditingTestMaterials").append("BeforeEditing").append(fileName)};
    const auto target{get_output_path("FileEditingOutput").append(fileName)};

    create_file(source, target, action);
    std::cout << st_Indent << target << '\n';

    const auto prediction{get_aux_path("FileEditingTestMaterials").append("AfterEditing").append(fileName)};

    return testing::compare_files(target, prediction, test_mode::standard);
  }

  void test_runner::test_creation()
  {    
    namespace fs = std::filesystem;

    sentinel s_0{};
    
    std::cout << indent("Running test creation tool diagnostics...\n", st_Indent);

    sentinel s_1{};

    const std::array<nascent_test, 1>
      diagnosticFiles{nascent_test{get_output_path("UnitTestCreationDiagnostics"), "Iterator", "utilities::iterator"}};

    std::cout << create_files(diagnosticFiles.cbegin(), diagnosticFiles.cend(),  indent("Files built:", st_Indent), fs::copy_options::overwrite_existing);
    std::cout << compare_files(diagnosticFiles.cbegin(), diagnosticFiles.cend(), indent("Comparisons against reference files:", st_Indent));
    std::cout << '\n';
  }

  void test_runner::run_tests()
  {
    using namespace std::chrono;
    const auto time{steady_clock::now()};

    if(!m_Families.empty() && (m_NascentTests.empty() || !m_SelectedFamilies.empty()))
    {
      std::cout << "Running unit tests...\n";
      log_summary summary{};
      if(!concurrent_execution())
      {
        for(auto& family : m_Families)
        {
          std::cout << family.name() << ":\n";
          summary += process_family(family.execute(m_WriteFiles, m_ConcurrencyMode)).log;
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
            std::async([&family, write{m_WriteFiles}, mode{m_ConcurrencyMode}](){
                return family.execute(write, mode); }));
        }

        for(auto& res : results)
        {
          std::cout << res.first << ":\n";
          summary += process_family(res.second.get()).log;
        }
      }
      std::cout <<  "\n-----------Grand Totals-----------\n";
      std::cout << summarize(summary, steady_clock::now() - time, log_verbosity::absent_checks, "\t",  "");
    }

    check_for_missing_tests();
  }

  std::filesystem::path get_aux_path(std::string_view subDirectory)
  {    
    namespace fs = std::filesystem;
    return fs::current_path().parent_path().append("aux_files").append(subDirectory);
  }
}
