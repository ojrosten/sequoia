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
  const std::map<std::string, std::size_t> unit_test_runner::s_ArgCount{
    {"create", 2}
  };
  
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

  const std::array<std::string, 5> unit_test_runner::st_TestNameStubs {
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

  std::string unit_test_runner::error(std::string_view message)
  {
    return std::string{"\n  Error: "}.append(message);
  }

  std::string unit_test_runner::report_arg_num(const std::size_t n)
  {
    return (std::to_string(n) += ((n==1) ? " was" : " were")) += " provided\n";
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

  bool unit_test_runner::file_exists(const std::string& path)
  {
    // use filesystem when available!
    return static_cast<bool>(std::ifstream{path});    
  }

  auto unit_test_runner::compare_files(const std::string& referenceFile, const std::string& generatedFile) -> file_comparison
  {
    std::ifstream file1{referenceFile}, file2{generatedFile};
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

  void unit_test_runner::compare_files(const nascent_test& data, const std::string& partName)
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
    build_map();
    
    std::vector<arg_list> args;
    for(int i{1}; i<argc; ++i)
    {
      std::string arg{argv[i]};
      if(!arg.empty())
      {
        const bool append{[&arg, &args](){
            if((arg.front() != '-') && !args.empty())
            {
              const auto& lastList{args.back()};
              if(!lastList.empty())
              {
                if(auto found{s_ArgCount.find(lastList.front())}; found != s_ArgCount.end())
                {
                  if(lastList.size() <= found->second)
                    return true;
                }
                else if(lastList.size() == 1)
                {
                  const auto& lastString{args.back().front()};
                  if(!lastString.empty() && (lastString.front() != '-'))
                    return true;
                }
              }
            }
            return false;
          }()
        };
                       
        if(append)
        {
          args.back().push_back(arg);
        }
        else
        {
          args.push_back(arg_list{{arg}});
        }
      }
    }
    
    for(const auto& argList : args)
    {
      if(!argList.empty())
      {          
        const std::string& key{argList.front()};
        arg_list remainingArgs(argList.size() - 1);
        std::copy(argList.begin() + 1, argList.end(), remainingArgs.begin());
        auto found{m_FunctionMap.find(key)};
        if(found != m_FunctionMap.end())
        {
          found->second(remainingArgs);
        }
        else
        {
          throw argument_error{(error("argument \'") += key).append("\' not recognized\n\n")};
        }
      }
    }

    if(m_Pause)
    {
      std::cout << "Please hit enter to continue...\n";
      while(std::cin.get() != '\n'){}
    }
        
    run_diagnostics();
  }

  void unit_test_runner::build_map()
  {
    m_FunctionMap.emplace("test", [this](const arg_list& argList) {          
        if(argList.size() == 1)
        {
          m_SpecificTests.emplace(argList.front(), false);
        }
        else
        {
          throw argument_error{error("'test' requires one argument [test_name], but ").append(report_arg_num(argList.size()))};
        }
      }
    );

    m_FunctionMap.emplace("create", [this](const arg_list& argList) {          
        if(argList.size() == 2)
        {          
          m_NewFiles.push_back(nascent_test{argList[0], argList[1]});
        }
        else
        {
          throw argument_error{error("'create' requires two arguments [directory, class_name], but ").append(report_arg_num(argList.size()))};
        }
      }
    );

    m_FunctionMap.emplace("-async", [this](const arg_list& argList) {
        if(argList.empty())
        {
          m_Asynchronous = true;
        }
        else
        {
          throw argument_error{error("-async requires no arguments, but ").append(report_arg_num(argList.size()))};
        }
      }
    );

    m_FunctionMap.emplace("-v", [this](const arg_list& argList) {
        if(argList.empty())
        {
          m_Verbose = true;
        }
        else
        {
          throw argument_error{error("-v requires no arguments, but ").append(report_arg_num(argList.size()))};
        }
      }
    );

    m_FunctionMap.emplace("-pause", [this](const arg_list& argList) {
        if(argList.empty())
        {
          m_Pause = true;
        }
        else
        {
          throw argument_error{error("-pause requires no arguments, but ").append(report_arg_num(argList.size()))};
        }
      }
    );
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
      if(m_Verbose) std::cout << "    " << s.summarize("        ", log_verbosity::failure_messages);
      familySummary += s;
    }
          
    if(!m_Verbose) std::cout << familySummary.summarize("    ", log_verbosity::failure_messages);

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
          summary += process_family(family.execute());
        }
      }
      else
      {
        std::cout << "Using asynchronous execution\n";
        std::vector<std::pair<std::string, std::future<std::vector<log_summary>>>> results;

        for(auto& family : m_Families)
        {
          results.emplace_back(family.name(), std::async([&family](){ return family.execute(); }));
        }

        for(auto& res : results)
        {
          std::cout << res.first << ":\n";
          summary += process_family(res.second.get());
        }        
      }
      std::cout <<  "-----Grand Totals-----\n";
      std::cout << summary.summarize("", log_verbosity::absent_checks);
    }

    check_for_missing_tests();
  }
}
