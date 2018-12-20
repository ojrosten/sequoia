#include "UnitTestRunner.hpp"

#include <array>

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

    template<class Iter>
    void unit_test_runner::align(Iter begin, Iter end, std::string_view suffix)
    {
       auto maxIter{std::max_element(begin, end, [](const std::string& lhs, const std::string& rhs) {
            return lhs.size() < rhs.size();
        })
      };

      const auto maxChars{maxIter->size()};

      for(; begin != end; ++begin)
      {
        auto& s{*begin};
        s += std::string(maxChars - s.size(), ' ') += std::string{suffix};
      }
    }

    template<class Iter>
    void unit_test_runner::align_right(Iter begin, Iter end)
    {
       auto maxIter{std::max_element(begin, end, [](const std::string& lhs, const std::string& rhs) {
            return lhs.size() < rhs.size();
        })
      };

      const auto maxChars{maxIter->size()};

      for(; begin != end; ++begin)
      {
        auto& s{*begin};
        s = std::string(maxChars - s.size(), ' ') + s;
      }
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
      std::array<std::string, 4> summaries{
        std::string{"Standard Checks:"},
        std::string{"Performance Checks:"},
        std::string{"False Negative Checks:"},
        std::string{"False Positive Checks:"}
      };

      align(summaries.begin(), summaries.end(), "  ");

      std::array<std::string, 4> checks{
        std::to_string(summary.checks()),
        std::to_string(summary.performance_checks()),
        std::to_string(summary.false_negative_checks()),
        std::to_string(summary.false_positive_checks())
      };

      align_right(checks.begin(), checks.end());

      for(int i{}; i<4; ++i)
      {
        summaries[i] += checks[i] += ";    Failures: ";
      }

      std::array<std::string, 4> failures{
        std::to_string(summary.standard_failures()),
        std::to_string(summary.performance_failures()),
        std::to_string(summary.false_negative_failures()),
        std::to_string(summary.false_positive_failures())
      };

      align_right(failures.begin(), failures.end());

      for(int i{}; i<4; ++i)
      {
        summaries[i] += failures[i];
      }

      for(const auto& s : summaries)
      {
        std::cout << s << '\n';
      }

      if(summary.critical_failures()) std::cout << "\nCritical Failures: " << summary.critical_failures() << "\n";
    }
  }
}
