////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CommandLineArguments.hpp"

namespace sequoia::parsing::commandline
{
  [[nodiscard]]
  std::string error(std::string_view message, std::string_view prefix)
  {
    return std::string{prefix}.append("Error: ").append(message);
  }

  [[nodiscard]]
  std::string warning(std::string_view message, std::string_view prefix)
  {
    return std::string{prefix}.append("Warning: ").append(message);
  }

  [[nodiscard]]
  std::string pluralize(const std::size_t n, std::string_view noun, std::string_view prefix)
  {
    auto s{std::string{prefix}.append(noun)};
    return (n==1) ? s : s.append("s");
  }

  [[nodiscard]]
  std::vector<operation> parse(int argc, char** argv, const std::vector<option_info>& info)
  {
    std::vector<operation> operations;

    auto infoIter{info.end()};        
    for(int i{1}; i<argc; ++i)
    {
      const std::string arg{argv[i]};
      if(!arg.empty())
      {        
        if(infoIter == info.end())
        {
          auto processOption{
            [&operations, &info, &arg](auto infoIter){
              if(infoIter == info.end())
                throw std::runtime_error{error("unrecognized option '" + arg + "'")};

              if(!infoIter->fn)
                throw std::logic_error{error("Commandline option not bound to a function object")};

              operations.push_back(operation{infoIter->fn, {}});
              if(infoIter->parameters.empty())
                infoIter = info.end();

              return infoIter;
            }
          };
          
          infoIter = std::find_if(info.begin(), info.end(), [&arg](const auto& optionInfo){ return optionInfo.name == arg; });
          if(infoIter == info.end())
          {            
            infoIter = std::find_if(info.begin(), info.end(), [&arg](const auto& e) {
                return std::find(e.aliases.begin(), e.aliases.end(), arg) != e.aliases.end();
              });

            if((infoIter == info.end()) && (arg.size() > 2) && (arg[0] == '-') && (arg[1] != ' '))
            {
              for(auto j{arg.cbegin() + 1}; j != arg.cend(); ++j)
              {
                const auto c{*j};
                if(c != '-')
                {
                  const auto alias{std::string{'-'} + c};

                  infoIter = std::find_if(info.begin(), info.end(), [&alias](const auto& e) {
                      return std::find(e.aliases.begin(), e.aliases.end(), alias) != e.aliases.end();
                    });

                  processOption(infoIter);
                }
              }

              if(infoIter != info.end())
              {
                infoIter = info.end();
                continue;
              }
            }
          }

          infoIter = processOption(infoIter);
        }
        else
        {
          auto& params{operations.back().parameters};
          params.push_back(arg);
          if(params.size() == infoIter->parameters.size())
            infoIter = info.end();
        }
      }
    }

    if(!operations.empty() && (infoIter != info.end()) && (operations.back().parameters.size() != infoIter->parameters.size()))
    {
      const auto& params{infoIter->parameters};
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
}
