////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/TextProcessing/Indent.hpp"

#include <stdexcept>
#include <iterator>

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

    return p.get();
  }

  void invoke_depth_first(const std::vector<operation>& operations)
  {
    for(auto& op : operations)
    {
      if(op.early) op.early(op.arguments);

      invoke_depth_first(op.nested_operations);

      if(op.late) op.late(op.arguments);
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
      if(optionsIter == options.end())
      {
        if(!arg.empty())
        {
          optionsIter = std::find_if(options.begin(), options.end(),
                                     [&arg](const auto& opt) { return opt.name == arg; });

          if(optionsIter == options.end())
          {
            optionsIter = std::find_if(options.begin(), options.end(),
              [&arg](const auto& opt) { return is_alias(opt, arg); });

            if(arg == "--help")
            {
              m_Help = generate_help(options);
              return true;
            }


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
      }
      else
      {
        auto& currentOperation{operations.back()};
        auto& arguments{currentOperation.arguments};
        arguments.push_back(arg);

        if(arguments.size() == optionsIter->parameters.size())
        {
          optionsIter = process_nested_options(optionsIter, options.end(), currentOperation);
        }
      }
    }

    if(   !operations.empty()
       && (optionsIter != options.end())
       && (operations.back().arguments.size() != optionsIter->parameters.size()))
    {
      const auto& params{optionsIter->parameters};
      const auto expected{params.size()};
      auto mess{std::string{"while parsing option \""}
                  .append(optionsIter->name)
                  .append("\": expected ")
                  .append(std::to_string(expected))
                  .append(pluralize(expected, "argument"))
                  .append(", [")};

      for(auto i{params.begin()}; i != params.end(); ++i)
      {
        mess.append(*i);
        if(std::distance(i, params.end()) > 1) mess.append(", ");
      }

      const auto actual{operations.back().arguments.size()};
      mess.append("], but found ").append(std::to_string(actual)).append(pluralize(actual, "argument"));

      throw std::runtime_error{error(mess)};
    }

    return true;
  }

  [[nodiscard]]
  std::string argument_parser::generate_help(const std::vector<option>& options)
  {
    std::string help;
    for(const auto& opt : options)
    {
      help += opt.name;
      if(!opt.aliases.empty())
      {
        help += " | ";
        for(const auto& a : opt.aliases)
        {
          help.append(a).append(" ");
        }
        help += "|";
      }

      for(const auto& p : opt.parameters)
      {
        help.append(" ").append(p).append(",");
      }

      if(!help.empty() && (help.back() == ','))
        help.pop_back();

      help += "\n";
      help += indent(generate_help(opt.nested_options), indentation{"  "});
    }

    return help;
  }

  template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
  std::optional<Iter> argument_parser::process_option(Iter optionsIter, Sentinel optionsEnd, std::string_view arg, std::vector<operation>& operations)
  {
    if(optionsIter == optionsEnd)
    {
      if(top_level(operations))
        throw std::runtime_error{error(std::string{"unrecognized option '"}.append(arg).append("'"))};

      return {};
    }

    if(top_level(operations) && !optionsIter->early &&  !optionsIter->late)
      throw std::logic_error{error("Commandline option not bound to a function object")};

    operations.push_back(operation{optionsIter->early, optionsIter->late, {}});
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


  template<std::input_iterator Iter, std::sentinel_for<Iter> Sentinel>
  bool argument_parser::process_concatenated_aliases(Iter optionsIter, Iter optionsBegin, Sentinel optionsEnd, std::string_view arg, std::vector<operation>& operations)
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

  template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
  Iter argument_parser::process_nested_options(Iter optionsIter, Sentinel optionsEnd, operation& currentOp)
  {
    if(!optionsIter->nested_options.empty())
    {
      if(m_Index + 1 < m_ArgCount)
      {
        ++m_Index;
        const bool isNestedOption{parse(optionsIter->nested_options, currentOp.nested_operations)};

        auto& nestedOperations{currentOp.nested_operations};
        auto i{nestedOperations.begin()};
        while(i != nestedOperations.end())
        {
          if(!i->early && !i->late)
          {
            auto& args{currentOp.arguments};
            const auto& nestedArgs{i->arguments};
            std::copy(nestedArgs.begin(), nestedArgs.end(), std::back_inserter(args));

            i = nestedOperations.erase(i);
          }
          else
          {
            ++i;
          }
        }

        if(!isNestedOption)
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

  [[nodiscard]]
  bool argument_parser::is_alias(const option& opt, const std::string& s)
  {
    return std::find(opt.aliases.begin(), opt.aliases.end(), s) != opt.aliases.end();
  }

}
