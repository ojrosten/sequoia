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
          std::cout << "\nWarning: argument \'" << key << "\' not recognized\n\n";
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
          std::cout << "\nWarning: 'test' requires one argument [test_name], but " << argList.size() << " was/were provided\n";
        }
      }
    );

    m_FunctionMap.emplace("create", [this](const arg_list& argList) {          
        if(argList.size() == 2)
        {
          new_files data{};

          data.directory = argList[0];

          auto& className{argList[1]};
          if(auto pos{className.find_last_of(':')}; pos != std::string::npos)
          {
            if(pos < className.length() - 1)
            {
            
              data.namespaces = className.substr(0, pos+1);
              data.class_name = className.substr(pos+1);
            }
          }
          else
          {
            data.class_name = className;
          }
          
          m_NewFiles.push_back(data);
        }
        else
        {
          std::cout << "\nWarning: 'create' requires two arguments [directory, class_name], but " << argList.size() << " was/were provided\n";
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
          std::cout << "\nWarning: -async requires no arguments, but " << argList.size() << " was/were provided\n";
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
          std::cout << "\nWarning: -verbose requires no arguments, but " << argList.size() << " was/were provided\n";
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
        std::string text{};
        if(std::ifstream ifile{"../aux_files/CodeTemplates/MyClassTestingUtilities.hpp"})
        {
          std::stringstream buffer{};
          buffer << ifile.rdbuf();
          text = buffer.str();
        }
        else
        {
          std::cout << "\nWarning: unable to open Testing Utilities template";
        }
        
        if(!text.empty())
        {
          using size_t = std::string::size_type;

          size_t pos{};
          while((pos = text.find("MyClass", pos)) != std::string::npos)
          {
            text.replace(pos, 7, data.class_name);
            pos += data.class_name.length();
          }
          
          if(std::string path{data.directory + "/" + to_camel_case(data.class_name) + "TestingUtilities.hpp"}; std::ofstream ofile{path})
          {
            std::cout << "Creating file " << path << '\n';
            ofile << text;
          }
          else
          {
            std::cout << "\nWarning: unable to create new Testing Utilities header";
          }
        }
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
