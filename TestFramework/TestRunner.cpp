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
  std::string test_runner::to_camel_case(std::string text)
  {
    if(!text.empty())
    {
      if(std::isalpha(text.front()))
      {
        text.front() = std::toupper(text.front());
      }

      using size_t = std::string::size_type;

      size_t pos{};
      while((pos = text.find("_", pos)) != std::string::npos)
      {
        text.erase(pos, 1);
        if((pos < text.length()) && std::isalpha(text[pos]))
        {
          text[pos] = std::toupper(text[pos]);
        }

        pos += 1;
      }
    }

    return text;
  }

  [[nodiscard]] std::string test_runner::stringify(concurrency_mode mode)
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

  const std::array<std::string_view, 5> test_runner::st_TestNameStubs {
    "TestingUtilities.hpp",
    "TestingDiagnostics.hpp",
    "TestingDiagnostics.cpp",
    "Test.hpp",
    "Test.cpp"
  };  

  void test_runner::replace_all(std::string& text, std::string_view from, const std::string& to)
  {
    std::string::size_type pos{};
    while((pos = text.find(from, pos)) != std::string::npos)
    {
      text.replace(pos, from.length(), to);
      pos += to.length();
    }
  }

  auto test_runner::compare_files(const std::filesystem::path& referenceFile, const std::filesystem::path& generatedFile) -> file_comparison
  {    
    std::ifstream file1{referenceFile}, file2{generatedFile};
    if(!file1) warning("unable to open file ").append(referenceFile.string()).append("\n");
    if(!file2) warning("unable to open file ").append(generatedFile.string()).append("\n");
    
    if(file1 && file2)
    {
      std::stringstream buffer1{}, buffer2{};
      buffer1 << file1.rdbuf();
      buffer2 << file2.rdbuf();

      return (buffer1.str() == buffer2.str()) ? file_comparison::same : file_comparison::different;
    }

    return file_comparison::failed;
  }

  void test_runner::compare_files(const nascent_test& data, std::string_view partName)
  {
    namespace fs = std::filesystem;

    const auto className{to_camel_case(data.class_name).append(partName)};

    const auto referenceFile{
      get_aux_path("UnitTestCodeTemplates").append("ReferenceExamples").append(className)
    };
    
    const auto generatedFile{
      get_output_path("UnitTestCreationDiagnostics").append(className)
    };

    switch(compare_files(referenceFile, generatedFile))
    {
    case file_comparison::same:
      std::cout << "    passed\n";
      break;
    case file_comparison::different:
      std::cout << warning("Contents of\n  " )
        .append(generatedFile.string())
        .append("\n  no longer matches\n  ")
        .append(referenceFile.string())
        .append("\n");
      break;
    case file_comparison::failed:
      std::cout << warning("Unable to perform file comparison\n");
      break;
    }
  }

  void test_runner::false_positive_check(const nascent_test& data)
  {
    static_assert(st_TestNameStubs.size() > 1, "Insufficient data for false-positive test");

    std::cout << "  False-positive test for file comparison\n";

    auto partPath{
      [&data](){
        return get_output_path("UnitTestCreationDiagnostics").append(to_camel_case(data.class_name));
      }
    };
    
    const auto referenceFile1{partPath().concat(st_TestNameStubs[0])};
    const auto referenceFile2{partPath().concat(st_TestNameStubs[1])};

    switch(compare_files(referenceFile1, referenceFile2))
    {
    case file_comparison::same:
      std::cout << warning("Contents of\n  " )
        .append(referenceFile1)
        .append("\n  spuriously comparing equal to\n  ")
        .append(referenceFile2)
        .append("\n");    
      break;
    case file_comparison::different:
      std::cout << "    passed\n";
      break;
    case file_comparison::failed:
      std::cout << warning("Unable to perform false-positive test\n");
      break;
    }
  }

  test_runner::nascent_test::nascent_test(std::filesystem::path dir, std::string qualifiedName)
    : directory{std::move(dir)}
    , qualified_class_name{std::move(qualifiedName)}
  {
          
    if(auto pos{qualified_class_name.rfind("::")}; pos != std::string::npos)
    {
      if(pos < qualified_class_name.length() - 2)
      {            
        class_name = qualified_class_name.substr(pos+2);
      }
    }
    else
    {
      class_name = qualified_class_name;
    }
  }

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
                m_NewFiles.push_back(nascent_test{args[0], args[1]});
              }, {"class_name", "directory"}, {"c"} } },
          {"--async",    {[this](const param_list&) {
                if(m_ConcurrencyMode == concurrency_mode::serial)
                  m_ConcurrencyMode = concurrency_mode::family;
              }, {}, {"-a"}} },
          {"--async-depth", {[this](const param_list& args) {
                const int i{std::clamp(std::stoi(args.front()), 0, 2)};
                m_ConcurrencyMode = static_cast<concurrency_mode>(i);
              }, {"depth [0-2]"}, {"-ad"}} },
          {"--verbose",  {[this](const param_list&) { m_Verbose    = true; },  {}, {"-v"} } },          
          {"--nofiles",  {[this](const param_list&) { m_WriteFiles = false; }, {}, {"-n"} } },
          {"--pause",    {[this](const param_list&) { m_Pause      = true; },  {}, {"-p"} } },
          {"--recovery", {[]    (const param_list&) {
                output_manager::recovery_file("../output/Recovery/Recovery.txt"); }, {}, {"-r"}} }
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

  void test_runner::run_diagnostics()
  {    
    const std::array<nascent_test, 1>
      diagnosticFiles{nascent_test{get_output_path("UnitTestCreationDiagnostics"), "utilities::iterator"}};

    create_files(diagnosticFiles.cbegin(), diagnosticFiles.cend(),  "Running unit test creation tool diagnostics...\n", true);

    compare_files(diagnosticFiles.cbegin(), diagnosticFiles.cend(), "  Comparing files against reference files...\n");

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
    create_files(m_NewFiles.cbegin(), m_NewFiles.cend(), "Creating files...\n", false);
    run_tests();
  }

  template<class Iter>
  void test_runner::create_files(Iter beginFiles, Iter endFiles, std::string_view message, const bool overwrite)
  {    
    if(std::distance(beginFiles, endFiles))
    {
      std::cout << message;

      while(beginFiles != endFiles)
      {
        const auto& data{*beginFiles};
        for(const auto& stub : st_TestNameStubs)
        {
          create_file(data, stub, overwrite);
        }

        ++beginFiles;
      }
    }
  }

  template<class Iter>
  void test_runner::compare_files(Iter beginFiles, Iter endFiles, std::string_view message)
  {
    if(std::distance(beginFiles, endFiles))
    {
      std::cout << message;

      while(beginFiles != endFiles)
      {
        const auto& data{*beginFiles};

        for(const auto& stub : st_TestNameStubs)
        {
          compare_files(data, stub);
        }
        
        ++beginFiles;
      }

      static_assert(st_TestNameStubs.size() > 1, "Insufficient data for false-positive test");      
    }
  }

  void test_runner::create_file(const nascent_test& data, std::string_view partName, const bool overwrite)
  {
    namespace fs = std::filesystem;
    
    const fs::directory_entry
      outputFile{(data.directory / to_camel_case(data.class_name)) += partName};
    if(!overwrite && outputFile.exists())
    {
      std::cout << warning(outputFile.path().string()).append(" already exists, so not created\n");
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
    
    std::string text{};
    if(const auto inputPath{makePath()}; std::ifstream ifile{inputPath})
    {
      std::stringstream buffer{};
      buffer << ifile.rdbuf();
      text = buffer.str();
    }
    else
    {
      std::cout << warning("unable to open ").append(inputPath.string());
    }
        
    if(!text.empty())
    {
      replace_all(text, "::my_class", data.qualified_class_name);
      replace_all(text, "my_class", data.class_name);
      replace_all(text, "MyClass", to_camel_case(data.class_name));

      if(std::ofstream ofile{outputFile})
      {
        std::cout << "    Creating file " << outputFile.path() << '\n';
        ofile << text;
      }
      else
      {
        std::cout << warning("unable to create file ").append(outputFile.path().string());
      }
    }
  }

  void test_runner::run_tests()
  {
    using namespace std::chrono;
    const auto time{steady_clock::now()};

    if(!m_Families.empty() && (m_NewFiles.empty() || !m_SelectedFamilies.empty()))
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
