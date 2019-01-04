////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "UnitTestRunner.hpp"

#include <map>
#include <iostream>

namespace sequoia
{
  namespace unit_testing
  {
    unit_test_runner::unit_test_runner(int argc, char** argv)
    {
      using arg_list = std::vector<std::string>;
      std::vector<arg_list> args;
      for(int i{1}; i<argc; ++i)
      {
        std::string arg{argv[i]};
        if(!arg.empty())
        {
          const bool append{[&arg, &args](){
              if((arg.front() != '-') && !args.empty())
              {
                const auto& lastList = args.back();
                if(lastList.size() == 1)
                {
                  const auto& lastString = args.back().front();
                  if(!lastString.empty() && (lastString.front() != '-'))
                    return true;
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
      
      std::map<std::string, std::function<void (const arg_list&)>> functionMap;

      functionMap.emplace("test", [this](const arg_list& argList) {
          if(argList.size() == 1)
          {
            m_SpecificCases.insert(argList.front());
          }
          else
          {
            std::cout << "Warning: 'test' requires precisely one argument, but " << argList.size() << " were provided\n";
          }
        }
      );

      functionMap.emplace("-async", [this](const arg_list& argList) {
          if(argList.empty())
          {
            m_Asynchronous = true;
          }
          else
          {
            std::cout << "Warning: -async requires no arguments, but " << argList.size() << " were provided\n";
          }
        }
      );

      functionMap.emplace("-v", [this](const arg_list& argList) {
          if(argList.empty())
          {
            m_Verbose = true;
          }
          else
          {
            std::cout << "Warning: -verbose requires no arguments, but " << argList.size() << " were provided\n";
          }
        }
      );

      

      for(const auto& argList : args)
      {
        if(!argList.empty())
        {          
          const std::string& key{argList.front()};
          arg_list remainingArgs(argList.size() - 1);
          std::copy(argList.begin() + 1, argList.end(), remainingArgs.begin());
          auto found = functionMap.find(key);
          if(found != functionMap.end())
          {
            found->second(remainingArgs);
          }
          else
          {
            std::cout << "Warning: argument \'" << key << "\' not recognized\n";
          }
        }
      }
    }
    
    void unit_test_runner::add_test_family(test_family&& f)
    {
      bool accept{true};

      if(!m_SpecificCases.empty())
      {
        if(m_SpecificCases.find(std::string{f.name()}) == m_SpecificCases.end())
          accept = false;
      }
      
      if(accept) m_Families.emplace_back(std::move(f));
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
    
    void unit_test_runner::execute()
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
