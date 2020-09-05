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
  std::vector<operation> parse(int argc, char** argv, const std::vector<option>& options)
  {
    argument_parser p{argc, argv, options};

    return p.acquire();
  }

  argument_parser::argument_parser(int argc, char** argv, const std::vector<option>& options)
  {
    parse(argc, argv, options, m_Operations);
  }

  template<class Iter>
  Iter argument_parser::process_option(Iter optionsIter, Iter optionsEnd, std::string_view arg, std::vector<operation>& operations)
  {
    if(optionsIter == optionsEnd)
      throw std::runtime_error{error(std::string{"unrecognized option '"}.append(arg).append("'"))};

    const bool topLevel{&operations == &m_Operations};
    if(topLevel && !optionsIter->fn)
      throw std::logic_error{error("Commandline option not bound to a function object")};

    operations.push_back(operation{optionsIter->fn, {}});
    if(optionsIter->parameters.empty())
      optionsIter = optionsEnd;

    return optionsIter;
  }

  void argument_parser::parse(int argc, char** argv, const std::vector<option>& options, std::vector<operation>& operations)
  {    
    auto optionIter{options.end()};        
    for(; m_Index<argc; ++m_Index)
    {
      const std::string arg{argv[m_Index]};
      if(!arg.empty())
      {        
        if(optionIter == options.end())
        {          
          optionIter = std::find_if(options.begin(), options.end(),
                                    [&arg](const auto& opt){ return opt.name == arg; });

          if(optionIter == options.end())
          {            
            optionIter = std::find_if(options.begin(), options.end(), [&arg](const auto& opt) {
                return std::find(opt.aliases.begin(), opt.aliases.end(), arg) != opt.aliases.end();
              });

            if((optionIter == options.end()) && (arg.size() > 2) && (arg[0] == '-') && (arg[1] != ' '))
            {
              for(auto j{arg.cbegin() + 1}; j != arg.cend(); ++j)
              {
                const auto c{*j};
                if(c != '-')
                {
                  const auto alias{std::string{'-'} + c};

                  optionIter = std::find_if(options.begin(), options.end(), [&alias](const auto& opt) {
                      return std::find(opt.aliases.begin(), opt.aliases.end(), alias) != opt.aliases.end();
                    });

                  process_option(optionIter, options.end(), arg, operations);
                }
              }

              if(optionIter != options.end())
              {
                optionIter = options.end();
                continue;
              }
            }
          }

          optionIter = process_option(optionIter, options.end(), arg, operations);
        }
        else
        {
          auto& currentOperation{operations.back()};
          auto& params{currentOperation.parameters};
          params.push_back(arg);
          
          if((params.size() == optionIter->parameters.size()))
          {
            if(!optionIter->nested_options.empty())
            {
              if(m_Index + 1 < argc)
              {
                ++m_Index;
                parse(argc, argv, optionIter->nested_options, currentOperation.nested_operations);

                auto& nestedOperations{currentOperation.nested_operations};
                auto i{nestedOperations.begin()};
                while(i != nestedOperations.end())
                {
                  if(!i->fn)
                  {
                    const auto& nestedParams{i->parameters};
                    std::copy(nestedParams.begin(), nestedParams.end(), std::back_inserter(params));

                    i = nestedOperations.erase(i);
                  }
                  else
                  {
                    ++i;
                  }
                }
              }
            }
            
            optionIter = options.end();            
          }
        }
      }
    }

    if(!operations.empty() && (optionIter != options.end()) && (operations.back().parameters.size() != optionIter->parameters.size()))
    {
      const auto& params{optionIter->parameters};
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
  }
}
