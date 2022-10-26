////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for CommandLineArguments.hpp
*/

#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/TextProcessing/Indent.hpp"

#include <stdexcept>
#include <iterator>

namespace sequoia::parsing::commandline
{
  namespace
  {
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

    parse(forest_iter{options.begin()}, forest_iter{options.end()}, {}, top_level::yes);
  }

  template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
  void argument_parser::parse(Iter beginOptions, Sentinel endOptions, const operation_data& previousOperationData, top_level topLevel)
  {
    if(!m_Help.empty() || (beginOptions == endOptions)) return;

    option_tree currentOptionTree{};
    auto currentOperationData{previousOperationData};
    while(m_Index < m_ArgCount)
    {
      std::string_view arg{m_Argv[m_Index++]};
      if(!currentOperationData.oper_tree || !currentOptionTree)
      {
        if(arg.empty()) continue;

        const auto optionsIter{std::find_if(beginOptions, endOptions,
          [arg](const auto& tree) {
            return (root_weight(tree).name == arg) || is_alias(root_weight(tree), arg);
          })
        };

        if(optionsIter == endOptions)
        {
          if(arg == "--help")
          {
            m_Help = generate_help(beginOptions, endOptions);
            return;
          }

          if(process_concatenated_aliases(beginOptions, endOptions, arg, currentOperationData, topLevel))
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
        if(!currentOptionTree) throw std::logic_error{"Current option not found"};

        if(root_weight(currentOperationData.oper_tree).arguments.size()
          < root_weight(currentOptionTree).parameters.size() + currentOperationData.saturated_args)
        {
          mutate_root_weight(currentOperationData.oper_tree, [arg](auto& w) { w.arguments.emplace_back(arg); });
        }
      }

      if(!currentOperationData.oper_tree) throw std::logic_error{"Current option not found"};

      if((root_weight(currentOperationData.oper_tree).arguments.size() == root_weight(currentOptionTree).parameters.size() + currentOperationData.saturated_args))
      {
        const auto node{currentOptionTree.node()};

        using iter_t = decltype(currentOptionTree.tree().cbegin_edges(node));
        using forest_iter = maths::forest_from_tree_iterator<iter_t, maths::const_tree_adaptor<options_tree>>;

        parse(forest_iter{currentOptionTree.tree().cbegin_edges(node), currentOptionTree.tree()},
              forest_iter{currentOptionTree.tree().cend_edges(node), currentOptionTree.tree()},
              currentOperationData,
              top_level::no);

        currentOptionTree = {};
        currentOperationData = previousOperationData;
      }
    }

    if(!m_Operations.empty()
      && currentOptionTree
      && (root_weight(currentOperationData.oper_tree).arguments.size() != root_weight(currentOptionTree).parameters.size()))
    {
      const auto& params{root_weight(currentOptionTree).parameters};
      const auto expected{params.size()};
      auto mess{std::string{"while parsing option \""}
                  .append(root_weight(currentOptionTree).name)
                  .append("\": expected ")
                  .append(std::to_string(expected))
                  .append(pluralize(expected, "argument"))
                  .append(", [")};

      for(auto i{params.begin()}; i != params.end(); ++i)
      {
        mess.append(*i);
        if(std::distance(i, params.end()) > 1) mess.append(", ");
      }

      const auto actual{root_weight(currentOperationData.oper_tree).arguments.size()};
      mess.append("], but found ").append(std::to_string(actual)).append(pluralize(actual, "argument"));

      throw std::runtime_error{error(mess)};
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

  template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
  [[nodiscard]]
  bool argument_parser::process_concatenated_aliases(Iter beginOptions, Sentinel endOptions, std::string_view arg, operation_data currentOperationData, top_level topLevel)
  {
    if((arg.size() < 2) || ((arg[0] == '-') && (arg[1] == ' ')))
      return false;

    for(auto j{arg.cbegin() + 1}; j != arg.cend(); ++j)
    {
      const auto c{*j};
      if(c != '-')
      {
        const auto alias{std::string{'-'} + c};

        auto optionsIter{std::find_if(beginOptions, endOptions,
          [&alias](const auto& tree) { return is_alias(root_weight(tree), alias); })};

        if(optionsIter == endOptions)  return false;

        const option_tree currentOptionTree{*optionsIter};
        process_option(currentOptionTree, currentOperationData, topLevel);
      }
    }

    return true;
  }


  [[nodiscard]]
  bool argument_parser::is_alias(const option& opt, std::string_view s)
  {
    return std::find(opt.aliases.begin(), opt.aliases.end(), s) != opt.aliases.end();
  }

  template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
  [[nodiscard]]
  std::string argument_parser::generate_help(Iter beginOptions, Sentinel endOptions)
  {
    indentation ind{};
    std::string help;

    while(beginOptions != endOptions)
    {
      const auto& optTree{(*beginOptions).tree()};
      const auto subTreeRootNode{(*beginOptions).node()};

      ++beginOptions;

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

      auto nodeLate{
        [&ind](auto) { ind.trim(2); }
      };

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
