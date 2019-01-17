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
          std::cout << (warning("argument \'") += key).append("\' not recognized\n\n");
        }
      }
    }
  }

  void unit_test_runner::build_map()
  {
    m_FunctionMap.emplace("test", [this](const arg_list& argList) {          
        if(argList.size() == 1)
        {
          m_SpecificTests.insert(argList.front());
        }
        else
        {
          std::cout << warning("'test' requires one argument [test_name], but ").append(report_arg_num(argList.size()));
        }
      }
    );

    m_FunctionMap.emplace("create", [this](const arg_list& argList) {          
        if(argList.size() == 2)
        {
          new_file data{argList[0], argList[1], ""};
          
          if(auto pos{data.qualified_class_name.rfind("::")}; pos != std::string::npos)
          {
            if(pos < data.qualified_class_name.length() - 2)
            {            
              data.class_name = data.qualified_class_name.substr(pos+2);
            }
          }
          
          m_NewFiles.push_back(data);
        }
        else
        {
          std::cout << warning("'create' requires two arguments [directory, class_name], but ").append(report_arg_num(argList.size()));
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
          std::cout << warning("-async requires no arguments, but ").append(report_arg_num(argList.size()));
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
          std::cout << warning("-verbose requires no arguments, but ").append(report_arg_num(argList.size()));
        }
      }
    );
  }
    
  void unit_test_runner::add_test_family(test_family&& f)
  {
    if(m_SpecificTests.empty() || (m_SpecificTests.find(std::string{f.name()}) != m_SpecificTests.end()))
    {
      m_Families.emplace_back(std::move(f));
    }
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
        if((pos < text.length() - 1) && std::isalpha(text[pos + 1]))
        {
          text[pos + 1] = std::toupper(text[pos + 1]);         
          pos += 2;
        }

        pos += 1;
      }
    }

    return text;
  }

  std::string unit_test_runner::warning(std::string_view message)
  {
    return std::string{"\nWarning: "}.append(message);
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
                        
  void unit_test_runner::execute()
  {
    create_files();
    run_tests();
  }

  void unit_test_runner::create_files()
  {
    if(!m_NewFiles.empty())
    {
      std::cout << "Creating files...\n";

      for(const auto& data : m_NewFiles)
      {
        create_file(data, "Utilities.hpp");
        create_file(data, "Diagnostics.hpp");
        create_file(data, "Diagnostics.cpp");                
      }
    }
  }

  void unit_test_runner::create_file(const new_file& data, std::string_view partName)
  {
    std::string text{};
    if(auto path{std::string{"../aux_files/UnitTestCodeTemplates/CodeTemplates/MyClassTesting"}.append(partName)};
       std::ifstream ifile{path})
    {
      std::stringstream buffer{};
      buffer << ifile.rdbuf();
      text = buffer.str();
    }
    else
    {
      std::cout << warning("unable to open" ).append(path);
    }
        
    if(!text.empty())
    {
      replace_all(text, "::my_class", data.qualified_class_name);
      replace_all(text, "my_class", data.class_name);
      replace_all(text, "MyClass", to_camel_case(data.class_name));

      if(auto path{std::string{data.directory + "/" + to_camel_case(data.class_name)}.append(partName)}; std::ofstream ofile{path})
      {
        std::cout << "  Creating file " << path << '\n';
        ofile << text;
      }
      else
      {
        std::cout << warning("unable to create file ").append(partName);
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
  }
}
