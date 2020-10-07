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
  outcome parse(int argc, char** argv, const std::vector<option>& options)
  {
    argument_parser p{argc, argv, options};

    return p.acquire();
  }

  void invoke_depth_first(const std::vector<operation>& operations)
  {
    for(auto& op : operations)
    {
      invoke_depth_first(op.nested_operations);
      
      if(op.fn) op.fn(op.parameters);
    }
  }
  
  argument_parser::argument_parser(int argc, char** argv, const std::vector<option>& options)
    : m_ArgCount{argc}
    , m_Argv{argv}
    , m_ZerothArg{m_ArgCount ? m_Argv[0] : ""}
  {      
    parse(options, m_Operations);
  }

  bool argument_parser::parse(const std::vector<option>& options, std::vector<operation>& operations)
  {    
    auto optionsIter{options.end()};        
    for(; m_Index < m_ArgCount; ++m_Index)
    {
      const std::string arg{m_Argv[m_Index]};
      if(!arg.empty())
      {        
        if(optionsIter == options.end())
        {          
          optionsIter = std::find_if(options.begin(), options.end(),
                                    [&arg](const auto& opt){ return opt.name == arg; });

          if(optionsIter == options.end())
          {
            optionsIter = std::find_if(options.begin(), options.end(),
                                      [&arg](const auto& opt) { return is_alias(opt, arg); });

            if(process_concatenated_aliases(optionsIter, options.begin(), options.end(), arg, operations))
              continue;
          }

          if(auto maybeIter{process_option(optionsIter, options.end(), arg, operations)})
          {
            optionsIter = *maybeIter;
          }
          else
          {
            return false;
          }
        }
        else
        {
          auto& currentOperation{operations.back()};
          auto& params{currentOperation.parameters};
          params.push_back(arg);

          if(params.size() == optionsIter->parameters.size())
          {
            optionsIter = process_nested_options(optionsIter, options.end(), currentOperation);
          }
        }
      }
    }

    if(   !operations.empty() && (optionsIter != options.end())
       && (operations.back().parameters.size() != optionsIter->parameters.size()))
    {
      const auto& params{optionsIter->parameters};
      const auto expected{params.size()};
      auto mess{std::string{"expected "}.append(std::to_string(expected))
                                        .append(pluralize(expected, "argument"))
                                        .append(", [")};

      for(auto i{params.begin()}; i != params.end(); ++i)
      {
        mess.append(*i);
        if(std::distance(i, params.end()) > 1) mess.append(", ");
      }

      const auto actual{operations.back().parameters.size()};
      mess.append("], but found ").append(std::to_string(actual)).append(pluralize(actual, "argument"));
      
      throw std::runtime_error{error(mess)};
    }

    return true;
  }

  template<class Iter>
  std::optional<Iter> argument_parser::process_option(Iter optionsIter, Iter optionsEnd, std::string_view arg, std::vector<operation>& operations)
  {
    if(optionsIter == optionsEnd)
    {
      if(top_level(operations))
        throw std::runtime_error{error(std::string{"unrecognized option '"}.append(arg).append("'"))};

      return {};
    }

    if(top_level(operations) && !optionsIter->fn)
      throw std::logic_error{error("Commandline option not bound to a function object")};

    operations.push_back(operation{optionsIter->fn, {}});
    if(optionsIter->parameters.empty())
    {
      if(!optionsIter->nested_options.empty())
      {
        process_nested_options(optionsIter, optionsEnd, operations.back());
      }
      
      optionsIter = optionsEnd;
    } 

    return optionsIter;
  }


  template<class Iter>
  bool argument_parser::process_concatenated_aliases(Iter optionsIter, Iter optionsBegin, Iter optionsEnd, std::string_view arg, std::vector<operation>& operations)
  {
    if(optionsIter != optionsEnd) return false;
    
    if((arg.size() > 2) && (arg[0] == '-') && (arg[1] != ' '))
    {
      for(auto j{arg.cbegin() + 1}; j != arg.cend(); ++j)
      {
        const auto c{*j};
        if(c != '-')
        {
          const auto alias{std::string{'-'} + c};

          optionsIter = std::find_if(optionsBegin, optionsEnd,
                                    [&alias](const auto& opt) { return is_alias(opt, alias); });

          process_option(optionsIter, optionsEnd, arg, operations);
        }
      }
    }

    return optionsIter != optionsEnd;
  }

  template<class Iter>
  Iter argument_parser::process_nested_options(Iter optionsIter, Iter optionsEnd, operation& currentOp)
  {
    if(!optionsIter->nested_options.empty())
    {
      if(m_Index + 1 < m_ArgCount)
      {
        ++m_Index;
        bool complete{parse(optionsIter->nested_options, currentOp.nested_operations)};
        
        auto& nestedOperations{currentOp.nested_operations};
        auto i{nestedOperations.begin()};
        while(i != nestedOperations.end())
        {
          if(!i->fn)
          {
            auto& params{currentOp.parameters};
            const auto& nestedParams{i->parameters};
            std::copy(nestedParams.begin(), nestedParams.end(), std::back_inserter(params));

            i = nestedOperations.erase(i);
          }
          else
          {
            ++i;
          }
        }

        if(!complete)
          --m_Index;
      }
    }

    return optionsEnd;
  }

  [[nodiscard]]
  bool argument_parser::top_level(const std::vector<operation>& operations) const noexcept
  {
    return &m_Operations == &operations;
  }

  bool argument_parser::is_alias(const option& opt, const std::string& s)
  {
    return std::find(opt.aliases.begin(), opt.aliases.end(), s) != opt.aliases.end();
  }

}
