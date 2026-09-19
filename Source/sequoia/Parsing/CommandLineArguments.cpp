////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/TextProcessing/Indent.hpp"

#include <algorithm>
#include <format>
#include <ranges>
#include <stdexcept>

namespace sequoia::parsing::commandline
{
  namespace
  {
    constexpr std::string_view help_request{"--help"};

    [[nodiscard]]
    std::string make(std::string_view type, std::string_view message, std::string_view indent)
    {
      return std::string{indent}.append(type).append(": ").append(message);
    }

    [[nodiscard]]
    std::string make(std::string_view type, std::initializer_list<std::string_view> messages, std::string_view indent)
    {
      std::string mess{};

      if(messages.size())
      {
        auto first{messages.begin()};
        mess = make(type, *(first++), indent);

        while(first != messages.end())
        {
          append_indented(mess, *(first++), indentation{std::string{indent}}.append(std::string(type.size() + 2, ' ')));
        }

        mess.append(2, '\n');
      }

      return mess;
    }

    [[nodiscard]]
    std::string join(const param_list& params, std::string_view separator)
    {
      return   params
             | std::views::transform([](const proper_string& p){ return std::string_view{p}; })
             | std::views::join_with(separator)
             | std::ranges::to<std::string>();
    }
  }

  [[nodiscard]]
  std::string error(std::string_view message, std::string_view indent)
  {
    return make("Error", message, indent);
  }

  [[nodiscard]]
  std::string error(std::initializer_list<std::string_view> messages, std::string_view indent)
  {
    return make("Error", messages, indent);
  }

  [[nodiscard]]
  std::string warning(std::string_view message, std::string_view indent)
  {
    return make("Warning", message, indent);
  }

  [[nodiscard]]
  std::string warning(std::initializer_list<std::string_view> messages, std::string_view indent)
  {
    return make("Warning", messages, indent);
  }

  [[nodiscard]]
  std::string pluralize(const std::size_t n, std::string_view noun, std::string_view prefix)
  {
    auto s{std::string{prefix}.append(noun)};
    return (n==1) ? s : s.append("s");
  }

  argument_parser::argument_parser(int argc, char** argv, const options_forest& options)
    : m_ArgCount{argc}
    , m_Argv{argv}
    , m_ZerothArg{m_ArgCount ? m_Argv[0] : ""}
  {
    using iter_t = decltype(options.begin());
    using forest_iter = maths::forest_iterator<iter_t, maths::const_tree_adaptor<options_tree>>;

    parse(std::ranges::subrange{forest_iter{options.begin()}, forest_iter{options.end()}}, {}, {});
  }

  template<std::ranges::input_range Options>
  void argument_parser::parse(const Options& options, option_tree enclosingOption, const operation_data& previousOperationData)
  {
    // An option without nested options has nothing to parse beneath it, so what follows it
    // - a help request included - belongs to the enclosing level
    if(std::ranges::empty(options)) return;

    const top_level topLevel{enclosingOption ? top_level::no : top_level::yes};

    option_tree currentOptionTree{};
    auto currentOperationData{previousOperationData};
    while(m_Index < m_ArgCount)
    {
      std::string_view arg{m_Argv[m_Index++]};

      // Help is for the innermost option still open - collecting its parameters, or with nested
      // options to parse - and at the top level for every option
      if(arg == help_request)
      {
        const option_tree openOption{currentOptionTree ? currentOptionTree : enclosingOption};
        m_Help = openOption ? generate_help(std::views::single(openOption)) : generate_help(options);
        return;
      }

      if(!currentOperationData.oper_tree || !currentOptionTree)
      {
        if(arg.empty()) continue;

        const auto optionsIter{std::ranges::find_if(options,
          [arg](const auto& tree) {
            return (root_weight(tree).name == arg) || is_alias(root_weight(tree), arg);
          })
        };

        if(optionsIter == std::ranges::end(options))
        {
          if(process_concatenated_aliases(options, arg, currentOperationData, topLevel))
            continue;

          if(topLevel == top_level::yes)
            throw std::runtime_error{error(std::string{"unrecognized option '"}.append(arg).append("'"))};

          // Roll back and see if the current argument makes sense at the previous level
          --m_Index;
          return;
        }

        currentOptionTree = *optionsIter;
        currentOperationData = process_option(currentOptionTree, currentOperationData, topLevel);
      }
      else
      {
        if(root_weight(currentOperationData.oper_tree).arguments.size()
          < root_weight(currentOptionTree).parameters.size() + currentOperationData.saturated_args)
        {
          mutate_root_weight(currentOperationData.oper_tree, [arg](auto& w) { w.arguments.emplace_back(arg); });
        }
      }

      if((root_weight(currentOperationData.oper_tree).arguments.size() == root_weight(currentOptionTree).parameters.size() + currentOperationData.saturated_args))
      {
        const auto node{currentOptionTree.node()};

        using iter_t = decltype(currentOptionTree.tree().cbegin_edges(node));
        using forest_iter = maths::forest_from_tree_iterator<iter_t, maths::const_tree_adaptor<options_tree>>;

        parse(std::ranges::subrange{forest_iter{currentOptionTree.tree().cbegin_edges(node), currentOptionTree.tree()},
                                    forest_iter{currentOptionTree.tree().cend_edges(node), currentOptionTree.tree()}},
              currentOptionTree,
              currentOperationData);

        if(!m_Help.empty()) return;

        currentOptionTree = {};
        currentOperationData = previousOperationData;
      }
    }

    if(!m_Operations.empty() && currentOptionTree)
    {
      const auto& params{root_weight(currentOptionTree).parameters};
      const auto expected{params.size()};
      const auto actual{root_weight(currentOperationData.oper_tree).arguments.size() - currentOperationData.saturated_args};

      if(actual != expected)
      {
        throw std::runtime_error{
          error(std::format("while parsing option \"{}\": expected {}{}, [{}], but found {}{}",
                            std::string_view{root_weight(currentOptionTree).name},
                            expected,
                            pluralize(expected, "argument"),
                            join(params, ", "),
                            actual,
                            pluralize(actual, "argument")))
        };
      }
    }
  }

  auto argument_parser::process_option(option_tree currentOptionTree, operation_data currentOperationData, top_level topLevel) -> operation_data
  {
    if(topLevel == top_level::yes)
    {
      if(!root_weight(currentOptionTree).early && !root_weight(currentOptionTree).late)
        throw std::logic_error{error("Commandline option not bound to a function object")};

      m_Operations.push_back({{{root_weight(currentOptionTree).early, root_weight(currentOptionTree).late, {}}}});
      currentOperationData = {{m_Operations.back(), 0}};
    }
    else
    {
      if(m_Operations.empty() || !currentOperationData.oper_tree)
        throw std::logic_error{"Unable to find commandline operation"};

      if(root_weight(currentOptionTree).early || root_weight(currentOptionTree).late)
      {
        auto& operationTree{m_Operations.back()};
        const auto node{operationTree.add_node(currentOperationData.oper_tree.node(), root_weight(currentOptionTree).early, root_weight(currentOptionTree).late)};
        currentOperationData = {{m_Operations.back(), node}};
      }
      else
      {
        currentOperationData = {currentOperationData.oper_tree, maths::root_weight(currentOperationData.oper_tree).arguments.size()};
      }
    }

    return currentOperationData;
  }

  template<std::ranges::input_range Options>
  [[nodiscard]]
  bool argument_parser::process_concatenated_aliases(const Options& options, std::string_view arg, operation_data currentOperationData, top_level topLevel)
  {
    // A group is a dash followed by single-character aliases, each spelt without its own dash
    if((arg.size() < 2) || (arg[0] != '-') || (arg[1] == ' ') || (arg[1] == '-'))
      return false;

    // Every alias is resolved, and every option it names checked, before any is processed, so a
    // group which is refused leaves no operation behind
    const auto findOption{
      [&options](char c) {
        const auto alias{std::string{'-'} + c};
        return std::ranges::find_if(options, [&alias](const auto& tree) { return is_alias(root_weight(tree), alias); });
      }
    };

    const auto optionsIters{
        arg.substr(1)
      | std::views::filter([](char c){ return c != '-'; })
      | std::views::transform(findOption)
      | std::ranges::to<std::vector>()
    };

    if(std::ranges::contains(optionsIters, std::ranges::end(options)))
      return false;

    for(const auto iter : optionsIters)
    {
      if(const auto numParams{root_weight(*iter).parameters.size()}; numParams)
      {
        throw std::runtime_error{
          error(std::format("option \"{}\" expects {}{}, so its alias cannot be concatenated with others, as in '{}'",
                            std::string_view{root_weight(*iter).name},
                            numParams,
                            pluralize(numParams, "argument"),
                            arg))
        };
      }
    }

    for(const auto iter : optionsIters)
    {
      process_option(*iter, currentOperationData, topLevel);
    }

    return true;
  }


  [[nodiscard]]
  bool argument_parser::is_alias(const option& opt, std::string_view s)
  {
    return std::ranges::find(opt.aliases, s) != opt.aliases.end();
  }

  template<std::ranges::input_range Options>
  [[nodiscard]]
  std::string argument_parser::generate_help(const Options& options)
  {
    indentation ind{};
    std::string help;

    for(const auto& opt : options)
    {
      const auto& optTree{opt.tree()};
      const auto subTreeRootNode{opt.node()};

      auto nodeEarly{
        [&](const auto n) {
          const auto& wt{optTree.cbegin_node_weights()[n]};
          help += indent(std::string{wt.name}, ind);
          if(!wt.aliases.empty())
          {
            help += " | ";
            for(const auto& a : wt.aliases)
            {
              help.append(a).append(" ");
            }
            help += "|";
          }

          for(const auto& p : wt.parameters)
          {
            help.append(" ").append(p).append(",");
          }

          if(!help.empty() && (help.back() == ','))
            help.pop_back();

          help += "\n";
          ind.append(2, ' ');
        }
      };

      auto nodeLate{ [&ind](auto) { ind.trim(2); } };

      traverse(maths::depth_first, optTree, maths::ignore_disconnected_t{subTreeRootNode}, nodeEarly, nodeLate);
    }

    return help;
  }

  [[nodiscard]]
  outcome parse(int argc, char** argv, const options_forest& options)
  {
    argument_parser p{argc, argv, options};

    return p.get();
  }
}
