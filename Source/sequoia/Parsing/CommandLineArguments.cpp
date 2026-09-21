////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/Parsing/CommandLineArguments.hpp"

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
    std::string make(std::string_view type, std::string_view message, const indentation& indent)
    {
      return std::format("{}{}: {}", std::string_view{indent}, type, message);
    }

    [[nodiscard]]
    std::string make(std::string_view type, std::initializer_list<std::string_view> messages, const indentation& indent)
    {
      if(std::ranges::empty(messages)) return {};

      // Continuation lines start beneath the first message, past "<type>: "
      const auto continuationIndent{indent + indentation{std::string(type.size() + 2, ' ')}};

      std::string mess{make(type, *messages.begin(), indent)};
      for(const auto message : messages | std::views::drop(1))
      {
        append_indented(mess, message, continuationIndent);
      }

      return mess.append(2, '\n');
    }

    [[nodiscard]]
    std::string join(const param_list& params, std::string_view separator)
    {
      return   params
             | std::views::transform([](const proper_string& p){ return std::string_view{p}; })
             | std::views::join_with(separator)
             | std::ranges::to<std::string>();
    }

    [[nodiscard]]
    std::string with_count(std::string_view noun, std::size_t count)
    {
      return std::format("{} {}{}", count, noun, (count == 1) ? "" : "s");
    }

    [[nodiscard]]
    bool has_function_object(const option& opt) noexcept
    {
      return opt.early || opt.late;
    }

    [[nodiscard]]
    bool has_parameters(const option_tree& opt)
    {
      return !root_weight(opt).parameters.empty();
    }

    [[nodiscard]]
    auto nested_options(const option_tree& opt)
    {
      using iter_t      = decltype(opt.tree().cbegin_edges(opt.node()));
      using forest_iter = maths::forest_from_tree_iterator<iter_t, maths::const_tree_adaptor<options_tree>>;

      return std::ranges::subrange{forest_iter{opt.tree().cbegin_edges(opt.node()), opt.tree()},
                                   forest_iter{opt.tree().cend_edges(opt.node()), opt.tree()}};
    }

    [[nodiscard]]
    bool has_nested_options(const option_tree& opt)
    {
      return !std::ranges::empty(nested_options(opt));
    }

    [[nodiscard]]
    auto top_level_options(const options_forest& options)
    {
      using iter_t      = decltype(options.begin());
      using forest_iter = maths::forest_iterator<iter_t, maths::const_tree_adaptor<options_tree>>;

      return std::ranges::subrange{forest_iter{options.begin()}, forest_iter{options.end()}};
    }
  }

  [[nodiscard]]
  std::string error(std::string_view message, indentation indent)
  {
    return make("Error", message, indent);
  }

  [[nodiscard]]
  std::string error(std::initializer_list<std::string_view> messages, indentation indent)
  {
    return make("Error", messages, indent);
  }

  [[nodiscard]]
  std::string warning(std::string_view message, indentation indent)
  {
    return make("Warning", message, indent);
  }

  [[nodiscard]]
  std::string warning(std::initializer_list<std::string_view> messages, indentation indent)
  {
    return make("Warning", messages, indent);
  }

  argument_parser::argument_parser(int argc, char** argv, const options_forest& options)
    : m_ArgCount{argc}
    , m_Argv{argv}
    , m_ZerothArg{m_ArgCount ? m_Argv[0] : ""}
  {
    parse(top_level_options(options), {}, top_level::yes);
  }

  template<std::ranges::input_range Options>
  void argument_parser::parse(const Options& options, const operation_data& previousOperationData, top_level topLevel)
  {
    if(std::ranges::empty(options)) return;

    // A help request met at any level of nesting ends parsing at every level
    option_tree currentOptionTree{};
    auto currentOperationData{previousOperationData};
    while((m_Index < m_ArgCount) && m_Help.empty())
    {
      std::string_view arg{m_Argv[m_Index++]};

      if(arg == help_request)
      {
        m_Help = m_MostRecentlyEncounteredOption ? generate_help(std::views::single(m_MostRecentlyEncounteredOption))
                                                 : generate_help(options);
        break;
      }

      if(!currentOperationData.oper_tree || !currentOptionTree)
      {
        if(arg.empty()) continue;

        const auto optionsIter{
          std::ranges::find_if(options,
                               [arg](const auto& tree) {
                                 return (root_weight(tree).name == arg) || is_alias(root_weight(tree), arg);
                               })
        };

        if(optionsIter == std::ranges::end(options))
        {
          if(process_concatenated_aliases(options, arg, currentOperationData, topLevel))
            continue;

          if(topLevel == top_level::yes)
            throw std::runtime_error{error(std::format("unrecognized option '{}'", arg))};

          // Roll back and see if the current argument makes sense at the previous level
          --m_Index;
          return;
        }

        currentOptionTree = *optionsIter;
        currentOperationData = process_option(currentOptionTree, currentOperationData, topLevel);
      }
      else
      {
        if(  root_weight(currentOperationData.oper_tree).arguments.size()
           < root_weight(currentOptionTree).parameters.size() + currentOperationData.enclosing_args_supplied)
        {
          mutate_root_weight(currentOperationData.oper_tree, [arg](auto& w) { w.arguments.emplace_back(arg); });
        }
      }

      if(   root_weight(currentOperationData.oper_tree).arguments.size()
         == root_weight(currentOptionTree).parameters.size() + currentOperationData.enclosing_args_supplied)
      {
        parse(nested_options(currentOptionTree), currentOperationData, top_level::no);

        currentOptionTree = {};
        currentOperationData = previousOperationData;
      }
    }

    if(m_Help.empty() && !m_Operations.empty() && currentOptionTree)
    {
      const auto& params{root_weight(currentOptionTree).parameters};
      const auto expected{params.size()};
      const auto actual{root_weight(currentOperationData.oper_tree).arguments.size() - currentOperationData.enclosing_args_supplied};

      if(actual != expected)
      {
        throw std::runtime_error{
          error(std::format("while parsing option \"{}\": expected {}, [{}], but found {}",
                            std::string_view{root_weight(currentOptionTree).name},
                            with_count("argument", expected),
                            join(params, ", "),
                            with_count("argument", actual)))
        };
      }
    }
  }

  auto argument_parser::process_option(option_tree currentOptionTree, operation_data currentOperationData, top_level topLevel) -> operation_data
  {
    m_MostRecentlyEncounteredOption = currentOptionTree;

    const option& opt{root_weight(currentOptionTree)};

    if(topLevel == top_level::yes)
    {
      if(!has_function_object(opt))
        throw std::logic_error{error("Commandline option not bound to a function object")};

      m_Operations.push_back({{{opt.early, opt.late, {}}}});
      return {{m_Operations.back(), 0}};
    }

    if(has_function_object(opt))
    {
      const auto node{m_Operations.back().add_node(currentOperationData.oper_tree.node(), opt.early, opt.late)};
      return {{m_Operations.back(), node}};
    }

    return {currentOperationData.oper_tree, root_weight(currentOperationData.oper_tree).arguments.size()};
  }

  template<std::ranges::input_range Options>
  [[nodiscard]]
  bool argument_parser::process_concatenated_aliases(const Options& options, std::string_view arg, operation_data currentOperationData, top_level topLevel)
  {
    // A group is a dash followed by single-character aliases, each spelt without its own dash
    if((arg.size() < 2) || (arg[0] != '-') || (arg[1] == ' ') || (arg[1] == '-'))
      return false;

    auto optionOf{
      [&options](char c) -> option_tree {
        const auto alias{std::string{'-'} + c};
        const auto iter{std::ranges::find_if(options, [&alias](const auto& tree) { return is_alias(root_weight(tree), alias); })};
        return (iter == std::ranges::end(options)) ? option_tree{} : *iter;
      }
    };

    const auto groupOptions{
        arg.substr(1)
      | std::views::filter([](char c){ return c != '-'; })
      | std::views::transform(optionOf)
      | std::ranges::to<std::vector>()
    };

    if(std::ranges::contains(groupOptions, option_tree{}))
      return false;

    if(const auto refused{std::ranges::find_if(groupOptions, [](const option_tree& o){ return has_parameters(o); })};
       refused != groupOptions.end())
    {
      throw std::runtime_error{
        error(std::format("option \"{}\" expects {}, so its alias cannot be concatenated with others, as in '{}'",
                          std::string_view{root_weight(*refused).name},
                          with_count("argument", root_weight(*refused).parameters.size()),
                          arg))
      };
    }

    if(const auto refused{std::ranges::find_if(groupOptions, [](const option_tree& o){ return has_nested_options(o); })};
       refused != groupOptions.end())
    {
      throw std::runtime_error{
        error(std::format("option \"{}\" has nested options, so its alias cannot be concatenated with others, as in '{}'",
                          std::string_view{root_weight(*refused).name},
                          arg))
      };
    }

    for(const auto& option : groupOptions)
    {
      process_option(option, currentOperationData, topLevel);
    }

    return true;
  }

  [[nodiscard]]
  bool argument_parser::is_alias(const option& opt, std::string_view s) noexcept
  {
    return std::ranges::contains(opt.aliases, s);
  }

  template<std::ranges::input_range Options>
  [[nodiscard]]
  std::string argument_parser::generate_help(const Options& options)
  {
    constexpr std::size_t nestingWidth{2};

    indentation ind{};
    std::string help{};

    for(const auto& opt : options)
    {
      const auto& optTree{opt.tree()};

      auto nodeEarly{
        [&](const auto n) {
          const option& wt{optTree.cbegin_node_weights()[n]};
          help += indent(std::string{wt.name}, ind);
          if(!wt.aliases.empty())    help += std::format(" | {} |", join(wt.aliases, " "));
          if(!wt.parameters.empty()) help += std::format(" {}", join(wt.parameters, ", "));
          help += '\n';

          ind.append(nestingWidth, ' ');
        }
      };

      auto nodeLate{[&ind](auto) { ind.trim(nestingWidth); }};

      traverse(maths::depth_first, optTree, maths::ignore_disconnected_t{opt.node()}, nodeEarly, nodeLate);
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
