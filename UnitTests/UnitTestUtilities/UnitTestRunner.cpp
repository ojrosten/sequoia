#include "UnitTestRunner.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    unit_test_runner::unit_test_runner(int argc, char** argv)
    {
      using arg_list = std::vector<std::string>;
      std::vector<arg_list> args;
      for(int i=1; i<argc; ++i)
      {
        std::string arg{argv[i]};
        if(!arg.empty())
        {
          const bool append = [&arg, &args](){
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
          }();
                       
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
            std::cout << "Warning: -test requires precisely one argument, but " << argList.size() << " were provided\n";
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
        if(m_SpecificCases.find(f.name()) == m_SpecificCases.end())
          accept = false;
      }
      
      if(accept) m_Families.emplace_back(std::move(f));
    }

    void unit_test_runner::execute()
    {
      std::cout << "Running unit tests...\n";
      log_summary summary{};
      if(!m_Asynchronous)
      {
        for(auto& family : m_Families)
        {
          const auto s = family.execute();
          std::cout << s.message();
          summary += s;
        }
      }
      else
      {
        std::cout << "Using asynchronous execution\n";
        std::vector<std::future<log_summary>> results;

        std::for_each(m_Families.begin(), m_Families.end(),[&results](auto& family){
            results.emplace_back(std::async([&family](){ return family.execute(); }));
          }
        );
  
        std::for_each(results.begin(), results.end(), [&summary](auto& res){
            const auto& s = res.get();
            std::cout << s.message();
            summary += s;
          }
        );
      }

      std::cout <<  "-----Grand Totals-----\n";
      std::cout << "Standard Checks: " << summary.checks() << ";\t\t " << "Failures: " << summary.standard_failures() << "\n";
      std::cout << "Performance Checks: " << summary.performance_checks() << ";\t\t\t " << "Failures: " << summary.performance_failures() << "\n";
      std::cout << "False Negative Checks: " << summary.false_negative_checks() << ";\t\t " << "Failures: " << summary.false_negative_failures() << "\n";
      std::cout << "False Positive Checks: " << summary.false_positive_checks() << ";\t\t " << "Failures: " << summary.false_positive_failures() << "\n";
      if(summary.critical_failures()) std::cout << "\nCritical Failures: " << summary.critical_failures() << "\n";
    }
  }
}
