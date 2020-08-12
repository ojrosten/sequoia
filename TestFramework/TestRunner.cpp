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
  std::string compare_files(const std::filesystem::path& file, const std::filesystem::path& prediction, const test_mode mode)
  {
    enum class file_comparison {failed, same, different};

    auto fcomp{file_comparison::failed};
    std::string info{};
    
    std::ifstream file1{file}, file2{prediction};
    if(!file1) warning("unable to open file ").append(file).append("\n");
    if(!file2) warning("unable to open file ").append(prediction).append("\n");
    
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
        info = warning("Contents of\n  " )
          .append(file)
          .append("\n  spuriously comparing equal to\n  ")
          .append(prediction)
          .append("\n");    
        break;
      case file_comparison::different:
        info = "      passed\n";
        break;
      case file_comparison::failed:
        info = warning("Unable to perform false-positive test\n");
        break;
      }
      break;
    default:
      switch(fcomp)
      {
      case file_comparison::same:
        info = "      passed\n";
        break;
      case file_comparison::different:
        info = warning("Contents of\n  " )
          .append(file)
          .append("\n  no longer matches\n  ")
          .append(prediction)
          .append("\n");
        break;
      case file_comparison::failed:
        info = warning("Unable to perform file comparison\n");
        break;
      }
      break;
    }

    return info;
  }

  //=========================================== nascent_test ===========================================//

  nascent_test::nascent_test(std::filesystem::path dir, std::string qualifiedName)
    : m_Directory{std::move(dir)}
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

  void nascent_test::create_file(std::string_view partName, const std::filesystem::copy_options options) const
  {
    namespace fs = std::filesystem;
    
    const auto outputFile{(m_Directory / to_camel_case(m_ClassName)) += partName};

    if(((options & fs::copy_options::skip_existing) == fs::copy_options::skip_existing) && fs::exists(outputFile))
    {
      std::cout << warning(outputFile.string()).append(" already exists, so not created\n");
      return;
    }

    auto makePath{
      [partName](){
        return fs::current_path().parent_path()
          .append("aux_files")
          .append("UnitTestCodeTemplates")
          .append("CodeTemplates")
          .append("MyClass") += partName;
      }
    };

    auto fn{
      [this,&outputFile](const fs::path& file) {
        std::string text{};
        if(const auto inputPath{file}; std::ifstream ifile{inputPath})
        {
          std::stringstream buffer{};
          buffer << ifile.rdbuf();
          text = buffer.str();
        }
        else
        {
          throw std::runtime_error{std::string{"Unable to open "}.append(inputPath).append(" for reading")};
        }
        
        if(!text.empty())
        {
          replace_all(text, "::my_class", m_QualifiedClassName);
          replace_all(text, "my_class", m_ClassName);
          replace_all(text, "MyClass", to_camel_case(m_ClassName));

          if(std::ofstream ofile{outputFile})
          {
            ofile << text;
          }
          else
          {
            throw std::runtime_error{std::string{"Unable to open file "}.append(outputFile).append(" for writing")};
          }
        }
      }
    };

    testing::create_file(makePath(), outputFile, fn);
    std::cout << "      " << outputFile << '\n';
  }

  void nascent_test::compare_files(std::string_view partName) const
  {
    namespace fs = std::filesystem;

    const auto className{to_camel_case(m_ClassName).append(partName)};

    const auto file{get_output_path("UnitTestCreationDiagnostics").append(className)};
    const auto prediction{get_aux_path("UnitTestCodeTemplates").append("ReferenceExamples").append(className)};

    std::cout << testing::compare_files(file, prediction, test_mode::standard);
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
                m_NascentTests.push_back(nascent_test{args[0], args[1]});
              }, { "directory", "class_name" }, {"c"} } },
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
    default:      
      throw std::logic_error("Missing treatment for a case of concurrency_mode");
    }
  }

  void test_runner::false_positive_check(const nascent_test& data)
  {
    static_assert(st_TestNameStubs.size() > 1, "Insufficient data for false-positive test");

    std::cout << "  Running false-positive test for file comparison...\n";

    auto partPath{
      [&data](){
        return get_output_path("UnitTestCreationDiagnostics").append(to_camel_case(std::string{data.class_name()}));
      }
    };
    
    const auto file1{partPath().concat(st_TestNameStubs[0])};
    const auto file2{partPath().concat(st_TestNameStubs[1])};

    std::cout << testing::compare_files(file1, file2, test_mode::false_positive);
  }

  void test_runner::run_diagnostics()
  {
    namespace fs = std::filesystem;
    
    const std::array<nascent_test, 1>
      diagnosticFiles{nascent_test{get_output_path("UnitTestCreationDiagnostics"), "utilities::iterator"}};

    std::cout << "Running self-diagnostics...\n";

    std::string_view mess{"  Running test creation tool diagnostics...\n    Files built:\n"};
    create_files(diagnosticFiles.cbegin(), diagnosticFiles.cend(),  mess, fs::copy_options::overwrite_existing);

    compare_files(diagnosticFiles.cbegin(), diagnosticFiles.cend(), "\n    Comparisons against reference files:\n");

    test_file_editing();

    false_positive_check(diagnosticFiles.front());
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

    create_files(m_NascentTests.cbegin(), m_NascentTests.cend(), "Creating files...\n", fs::copy_options::skip_existing);
    run_tests();
  }

  template<class Iter>
  void test_runner::create_files(Iter beginNascentTests, Iter endNascentTests, std::string_view message, const std::filesystem::copy_options options)
  {    
    if(std::distance(beginNascentTests, endNascentTests))
    {
      std::cout << message;

      while(beginNascentTests != endNascentTests)
      {
        const auto& data{*beginNascentTests};
        for(const auto& stub : st_TestNameStubs)
        {
          data.create_file(stub, options);
        }

        ++beginNascentTests;
      }
    }
  }
  
  template<class Iter>
  void test_runner::compare_files(Iter beginNascentTests, Iter endNascentTests, std::string_view message)
  {
    if(std::distance(beginNascentTests, endNascentTests))
    {
      std::cout << message;

      while(beginNascentTests != endNascentTests)
      {
        const auto& data{*beginNascentTests};
        for(const auto& stub : st_TestNameStubs)
        {
          data.compare_files(stub);
        }
        
        ++beginNascentTests;
      }

      static_assert(st_TestNameStubs.size() > 1, "Insufficient data for false-positive test");      
    }
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
    std::cout << "      " << target << '\n';

    const auto prediction{get_aux_path("FileEditingTestMaterials").append("AfterEditing").append(fileName)};

    return testing::compare_files(target, prediction, test_mode::standard);
  }

  void test_runner::test_file_editing()
  {
    namespace fs = std::filesystem;
    
    std::cout << "\n  Running file editing diagnostics...\n";
    
    for(auto& p : fs::directory_iterator(get_output_path("FileEditingOutput")))
    {
      fs::remove(p.path());
    }

    std::cout << "    Files built:\n";

    std::string info{
      test_file_editing("Includes.hpp",
                        [](const fs::path& sandboxFile){
                          add_include(sandboxFile, "Bar.hpp");
                        })
    };

    info.append(
      test_file_editing("FakeMain.cpp",
                        [](const fs::path& sandboxFile){
                          add_to_family(sandboxFile, "New Family",
                                        {{"fake_false_positive_test{\"False Positive Test\"}"}, {"fake_test{\"Unit Test\"}"}});
                        })
                );

    info.append(
      test_file_editing("FakeMain2.cpp",
                        [](const fs::path& sandboxFile){
                          add_to_family(sandboxFile, "CommandLine Arguments",
                                        {{"commandline_arguments_false_positive_test{\"False Positive Test\"}"}});
                        })
                );
    
    std::cout << "\n    Comparisons against reference files:\n";
    std::cout << info << '\n';
  }

  void test_runner::run_tests()
  {
    using namespace std::chrono;
    const auto time{steady_clock::now()};

    if(!m_Families.empty() && (m_NascentTests.empty() || !m_SelectedFamilies.empty()))
    {
      std::cout << "\nRunning unit tests...\n";
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
