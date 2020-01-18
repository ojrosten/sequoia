////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UnitTestRunner.hpp"

#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>

namespace sequoia::unit_testing
{
  std::string summarize(const log_summary& log, std::string_view prefix, const log_verbosity suppression)
  {
    constexpr std::size_t entries{6};

    std::array<std::string, entries> summaries{
      std::string{prefix}.append("Standard Top Level Checks:"),
      std::string{prefix}.append("Standard Performance Checks:"),
      std::string{prefix}.append("False Negative Checks:"),
      std::string{prefix}.append("False Negative Performance Checks:"),
      std::string{prefix}.append("False Positive Checks:"),
      std::string{prefix}.append("False Positive PerformanceChecks:")
    };

    pad_right(summaries.begin(), summaries.end(), "  ");

    std::array<std::string, entries> checkNums{
      std::to_string(log.standard_top_level_checks()),
      std::to_string(log.standard_performance_checks()),
      std::to_string(log.false_negative_checks()),
      std::to_string(log.false_negative_performance_checks()),
      std::to_string(log.false_positive_checks()),
      std::to_string(log.false_positive_performance_checks())
    };

    constexpr std::size_t minChars{8};
    pad_left(checkNums.begin(), checkNums.end(), minChars);

    const auto len{10u - std::min(std::size_t{minChars}, checkNums.front().size())};
        
    for(int i{}; i<entries; ++i)
    {
      summaries[i].append(checkNums[i] + ";").append(len, ' ').append("Failures: ");
    }

    std::array<std::string, entries> failures{
      std::to_string(log.standard_top_level_failures()),
      std::to_string(log.standard_performance_failures()),
      std::to_string(log.false_negative_failures()),
      std::to_string(log.false_negative_performance_failures()),
      std::to_string(log.false_positive_failures()),
      std::to_string(log.false_positive_performance_failures())
    };

    pad_left(failures.begin(), failures.end(), 2);

    for(int i{}; i < entries; ++i)
    {
      summaries[i] += failures[i];
    }

    if(log.standard_top_level_checks())
      summaries.front().append("  [Deep checks: " + std::to_string(log.standard_deep_checks()) + "]");

    std::string summary{log.name().empty() ? "" : std::string{log.name()}.append(":\n")};

    if((suppression & log_verbosity::absent_checks) == log_verbosity::absent_checks)
    {
      std::for_each(std::cbegin(summaries), std::cend(summaries), [&summary](const std::string& s){
          (summary += s) += "\n";
        }
      );
    }
    else
    {
      const std::array<std::size_t, entries> checks{
        log.standard_top_level_checks(),
        log.standard_performance_checks(),
        log.false_negative_checks(),
        log.false_negative_performance_checks(),
        log.false_positive_checks(),
        log.false_positive_performance_checks()
      };

      for(int i{}; i<entries; ++i)
      {            
        if(checks[i]) summary += summaries[i] += "\n";
      }
    }

    if(log.critical_failures())
    {
      (summary += "Critical Failures:  ") += std::to_string(log.critical_failures()) += "\n";
    }

    if((suppression & log_verbosity::failure_messages) == log_verbosity::failure_messages)
      summary += log.failure_messages();

    return summary;
  }
  
  std::string unit_test_runner::to_camel_case(std::string text)
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

  [[nodiscard]]
  std::string error(std::string_view message)
  {
    return std::string{"\n  Error: "}.append(message);
  }

  std::vector<commandline_operation> parse(int argc, char** argv, const std::map<std::string, commandline_option_info>& info)
  {
    std::vector<commandline_operation> operations;

    auto infoIter{info.end()};
    for(int i{1}; i<argc; ++i)
    {
      std::string arg{argv[i]};
      if(!arg.empty())
      {        
        if(infoIter == info.end())
        {
          infoIter = info.find(arg);
          if(infoIter == info.end())
          {
            infoIter = std::find_if(info.begin(), info.end(), [&arg](const auto& e) {
                const auto& aliases{e.second.aliases};
                return std::find(aliases.begin(), aliases.end(), arg) != aliases.end();
              });

            if(infoIter == info.end())
              throw std::runtime_error{error("unrecognized option '" + arg + "'")};

            arg = infoIter->first;
          }

          operations.push_back(commandline_operation{infoIter->second.fn});
          if(infoIter->second.parameters.empty())
            infoIter = info.end();
        }
        else
        {
          auto& params{operations.back().parameters};
          params.push_back(arg);
          if(params.size() == infoIter->second.parameters.size())
            infoIter = info.end();
        }
      }
    }

    if(!operations.empty() && (infoIter != info.end()) && (operations.back().parameters.size() != infoIter->second.parameters.size()))
    {
      const auto& params{infoIter->second.parameters};
      const auto expected{params.size()};
      std::string mess{error("expected " + std::to_string(expected) + pluralize(expected, "argument") + ", [")};
      for(auto i{params.begin()}; i != params.end(); ++i)
      {
        mess.append(*i);
        if(std::distance(i, params.end()) > 1) mess.append(", ");
      }

      const auto actual{operations.back().parameters.size()};
      mess.append("], but found " + std::to_string(actual) + pluralize(actual, "argument"));
      
      throw std::runtime_error{mess};
    }

    return operations;
  }

  const std::array<std::string_view, 5> unit_test_runner::st_TestNameStubs {
    "TestingUtilities.hpp",
    "TestingDiagnostics.hpp",
    "TestingDiagnostics.cpp",
    "Test.hpp",
    "Test.cpp"
  };

  std::string unit_test_runner::warning(std::string_view message)
  {
    return std::string{"\n  Warning: "}.append(message);
  }

  void unit_test_runner::replace_all(std::string& text, std::string_view from, const std::string& to)
  {
    std::string::size_type pos{};
    while((pos = text.find(from, pos)) != std::string::npos)
    {
      text.replace(pos, from.length(), to);
      pos += to.length();
    }
  }

  bool unit_test_runner::file_exists(const std::string& path) noexcept
  {
    // use filesystem when available!
    return static_cast<bool>(std::ifstream{path});    
  }

  auto unit_test_runner::compare_files(std::string_view referenceFile, std::string_view generatedFile) -> file_comparison
  {
    std::ifstream file1{referenceFile.data()}, file2{generatedFile.data()};
    if(!file1) warning("unable to open file ").append(referenceFile).append("\n");
    if(!file2) warning("unable to open file ").append(generatedFile).append("\n");
    
    if(file1 && file2)
    {
      std::stringstream buffer1{}, buffer2{};
      buffer1 << file1.rdbuf();
      buffer2 << file2.rdbuf();

      return (buffer1.str() == buffer2.str()) ? file_comparison::same : file_comparison::different;
    }

    return file_comparison::failed;
  }

  void unit_test_runner::compare_files(const nascent_test& data, std::string_view partName)
  {
    const auto referenceFile{std::string{"../output/UnitTestCreationDiagnostics/" + to_camel_case(data.class_name)}.append(partName)};
    const auto generatedFile{std::string{"../aux_files/UnitTestCodeTemplates/ReferenceExamples/" + to_camel_case(data.class_name)}.append(partName)};

    switch(compare_files(referenceFile, generatedFile))
    {
    case file_comparison::same:
      std::cout << "    passed\n";
      break;
    case file_comparison::different:
      std::cout << warning("Contents of\n  " ).append(generatedFile).append("\n  no longer matches\n  ").append(referenceFile).append("\n");
      break;
    case file_comparison::failed:
      std::cout << warning("Unable to perform file comparison\n");
      break;
    }
  }

  void unit_test_runner::false_positive_check(const nascent_test& data)
  {
    static_assert(st_TestNameStubs.size() > 1, "Insufficient data for false-positive test");

    std::cout << "  False-positive test for file comparison\n";
    
    const auto referenceFile1{std::string{"../output/UnitTestCreationDiagnostics/" + to_camel_case(data.class_name)}.append(st_TestNameStubs[0])};
    const auto referenceFile2{std::string{"../output/UnitTestCreationDiagnostics/" + to_camel_case(data.class_name)}.append(st_TestNameStubs[1])};

    switch(compare_files(referenceFile1, referenceFile2))
    {
    case file_comparison::same:
      std::cout << warning("Contents of\n  " ).append(referenceFile1).append("\n  spuriously comparing equal to\n  ").append(referenceFile2).append("\n");    
      break;
    case file_comparison::different:
      std::cout << "    passed\n";
      break;
    case file_comparison::failed:
      std::cout << warning("Unable to perform false-positive test\n");
      break;
    }
  }

  unit_test_runner::nascent_test::nascent_test(std::string dir, std::string qualifiedName)
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

  unit_test_runner::unit_test_runner(int argc, char** argv)
  {    
    const auto operations{parse(argc, argv, {
          {"test", {{"test_name"}, {"t"}, [this](const arg_list& args){ m_SpecificTests.emplace(args.front(), false); }} },
          {"create",{{"class_name", "directory"}, {"c"}, [this](const arg_list& args) { m_NewFiles.push_back(nascent_test{args[0], args[1]}); }} },
          {"--async",    {{}, {"-a"}, [this](const arg_list& args) { m_Asynchronous = true; }} },
          {"--verbose",  {{}, {"-v"}, [this](const arg_list& args) { m_Verbose = true; }} },
          {"--recovery", {{}, {"-r"}, [](const arg_list& args) { output_manager::recovery_file("../output/Recovery/Recovery.txt"); }} },
          {"--nofiles",  {{}, {"-n"}, [this](const arg_list& args) {  m_WriteFiles = false; }} },
          {"--pause",    {{}, {"-p"}, [this](const arg_list& args) { m_Pause = true; }} }
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

  void unit_test_runner::check_argument_consistency()
  {
    if(m_Asynchronous && !output_manager::recovery_file().empty())
      throw std::runtime_error{error("Can't run asynchronously in recovery mode\n")};
  }

  void unit_test_runner::run_diagnostics()
  {
    const std::array<nascent_test, 1> diagnosticFiles{nascent_test{"../output/UnitTestCreationDiagnostics", "utilities::iterator"}};
    create_files(diagnosticFiles.cbegin(), diagnosticFiles.cend(),  "Running unit test creation tool diagnostics...\n", true);
    compare_files(diagnosticFiles.cbegin(), diagnosticFiles.cend(), "  Comparing files against reference files...\n");
    false_positive_check(diagnosticFiles.front());
  }
    
  void unit_test_runner::add_test_family(test_family&& f)
  {
    auto process{
      [&specificTests=m_SpecificTests,&f](){
        if(specificTests.empty()) return true;

        auto i{specificTests.find(std::string{f.name()})};
        if(i != specificTests.end())
        {
          i->second = true;
          return true;
        }

        return false;
      }
    };

    if(process()) m_Families.emplace_back(std::move(f));
  }

  log_summary unit_test_runner::process_family(const std::vector<log_summary>& summaries)
  {
    log_summary familySummary{};
    for(const auto& s : summaries)
    {
      if(m_Verbose) std::cout << "    " << summarize(s, "        ", log_verbosity::failure_messages);
      familySummary += s;
    }
          
    if(!m_Verbose) std::cout << summarize(familySummary, "    ", log_verbosity::failure_messages);

    return familySummary;
  }

  void unit_test_runner::check_for_missing_tests()
  {
    for(const auto& test : m_SpecificTests)
    {
      if(!test.second)
      {
        std::cout << warning("Test '" + test.first + "' not found\n");
      }
    }
  }
                       
  void unit_test_runner::execute()
  {
    create_files(m_NewFiles.cbegin(), m_NewFiles.cend(), "Creating files...\n", false);
    run_tests();
  }

  template<class Iter>
  void unit_test_runner::create_files(Iter beginFiles, Iter endFiles, std::string_view message, const bool overwrite)
  {    
    if(std::distance(beginFiles, endFiles))
    {
      std::cout << message;

      while(beginFiles != endFiles)
      {
        const auto& data{*beginFiles};
        for(const auto& stub : st_TestNameStubs)
        {
          create_file(data, stub,   overwrite);
        }

        ++beginFiles;
      }
    }
  }

  template<class Iter>
  void unit_test_runner::compare_files(Iter beginFiles, Iter endFiles, std::string_view message)
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

  void unit_test_runner::create_file(const nascent_test& data, std::string_view partName, const bool overwrite)
  {
    const auto outputPath{std::string{data.directory + "/" + to_camel_case(data.class_name)}.append(partName)};
    if(!overwrite && file_exists(outputPath))
    {
      std::cout << warning(outputPath).append(" already exists, so not created\n");
      return;
    }
    
    std::string text{};
    if(auto path{std::string{"../aux_files/UnitTestCodeTemplates/CodeTemplates/MyClass"}.append(partName)};
       std::ifstream ifile{path})
    {
      std::stringstream buffer{};
      buffer << ifile.rdbuf();
      text = buffer.str();
    }
    else
    {
      std::cout << warning("unable to open ").append(path);
    }
        
    if(!text.empty())
    {
      replace_all(text, "::my_class", data.qualified_class_name);
      replace_all(text, "my_class", data.class_name);
      replace_all(text, "MyClass", to_camel_case(data.class_name));

      if(std::ofstream ofile{outputPath})
      {
        std::cout << "    Creating file " << outputPath << '\n';
        ofile << text;
      }
      else
      {
        std::cout << warning("unable to create file ").append(outputPath);
      }
    }
  }

  void unit_test_runner::run_tests()
  {
    if(!m_Families.empty() && (m_NewFiles.empty() || !m_SpecificTests.empty()))
    {
      std::cout << "Running unit tests...\n";
      log_summary summary{};
      if(!m_Asynchronous)
      {
        for(auto& family : m_Families)
        {
          std::cout << family.name() << ":\n";
          summary += process_family(family.execute(m_WriteFiles));
        }
      }
      else
      {
        std::cout << "Using asynchronous execution\n";
        std::vector<std::pair<std::string, std::future<std::vector<log_summary>>>> results;

        for(auto& family : m_Families)
        {
          results.emplace_back(family.name(),
            std::async([&family, write{m_WriteFiles}](){ return family.execute(write); }));
        }

        for(auto& res : results)
        {
          std::cout << res.first << ":\n";
          summary += process_family(res.second.get());
        }        
      }
      std::cout <<  "-----Grand Totals-----\n";
      std::cout << summarize(summary, "", log_verbosity::absent_checks);
    }

    check_for_missing_tests();
  }
}
