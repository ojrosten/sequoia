////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/TextProcessing/Substitutions.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <functional>
#include <ranges>
#include <span>
#include <stdexcept>

namespace sequoia::parsing::commandline
{
  namespace
  {
    constexpr std::array<std::string_view, 2> help_requests{"--help", "-h"};

    [[nodiscard]]
    bool is_help_request(std::string_view arg) noexcept
    {
      return std::ranges::contains(help_requests, arg);
    }

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

      auto appendContinuation{
        [&continuationIndent](std::string mess, std::string_view message) -> std::string {
          append_indented(mess, message, continuationIndent);
          return mess;
        }
      };

      return std::ranges::fold_left(messages | std::views::drop(1), make(type, *messages.begin(), indent), appendContinuation)
               .append(2, '\n');
    }

    template<std::ranges::input_range Strings>
      requires std::convertible_to<std::ranges::range_reference_t<Strings>, std::string_view>
    [[nodiscard]]
    std::string join(const Strings& strings, std::string_view separator)
    {
      return   strings
             | std::views::transform([](const auto& s){ return std::string_view{s}; })
             | std::views::join_with(separator)
             | std::ranges::to<std::string>();
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
      return maths::forest_beneath(opt.tree(), opt.node());
    }

    [[nodiscard]]
    bool has_nested_options(const option_tree& opt)
    {
      return !std::ranges::empty(nested_options(opt));
    }

    [[nodiscard]]
    auto top_level_options(const options_forest& options)
    {
      return maths::forest_of(options);
    }

    [[nodiscard]]
    bool is_command(const option& opt) noexcept
    {
      return !std::string_view{opt.name}.starts_with('-');
    }

    template<std::ranges::input_range Options>
    [[nodiscard]]
    bool has_command(const Options& options)
    {
      return std::ranges::any_of(options, [](const option_tree& o){ return is_command(root_weight(o)); });
    }

    /** \brief What may follow the options of a level: [command] if any is a command, and [options]
        if there are any at all, since help is then among them.
     */
    template<std::ranges::input_range Options>
    [[nodiscard]]
    std::string continuation_of(const Options& options)
    {
      std::string continuation{};
      if(has_command(options))
        continuation += " [command]";

      if(!std::ranges::empty(options))
        continuation += " [options]";

      return continuation;
    }

    /** \brief What may follow an option on the command line: its parameters as placeholders, then
        whatever may follow its nested options, as in "<owner> <path> [options]".
     */
    [[nodiscard]]
    std::string continuation_of(const option_tree& opt)
    {
      auto placeholder{[](const proper_string& p){ return std::format(" <{}>", std::string_view{p}); }};
      auto placeholders{root_weight(opt).parameters | std::views::transform(placeholder)};

      return std::ranges::fold_left(placeholders, std::string{}, std::plus{}) + continuation_of(nested_options(opt));
    }

    /** \brief An option's names, "name, alias, alias", followed by its continuation. */
    [[nodiscard]]
    std::string entry_of(const option_tree& opt)
    {
      const option& wt{root_weight(opt)};
      auto aliases{wt.aliases | std::views::transform([](const proper_string& a){ return std::format(", {}", std::string_view{a}); })};

      return std::ranges::fold_left(aliases, std::string{wt.name}, std::plus{}) + continuation_of(opt);
    }

    struct help_entry
    {
      std::string entry{}, description{};
    };

    constexpr std::size_t help_indent_width{2}, help_column_gap{2}, help_column_limit{40};

    /** \brief The description column for a set of entries: past the widest, up to the limit; zero
        for no entries.
     */
    [[nodiscard]]
    std::size_t description_column(std::span<const help_entry> entries)
    {
      if(entries.empty())
        return 0;

      const auto widths{entries | std::views::transform([](const help_entry& e){ return e.entry.size(); })};
      const auto widest{std::ranges::fold_left(widths, std::size_t{}, std::ranges::max)};
      return std::ranges::min(help_indent_width + widest + help_column_gap, help_column_limit);
    }

    /** \brief Lays out entries in two columns, the first line of each description alongside its
        entry; where fewer than help_column_gap spaces would separate them, the description goes
        on the following line.
     */
    [[nodiscard]]
    std::string tabulate(std::span<const help_entry> entries, std::size_t descriptionColumn)
    {
      auto row{
        [descriptionColumn](const help_entry& e) {
          auto line{std::format("{:{}}{}", "", help_indent_width, e.entry)};
          if(!e.description.empty())
          {
            const auto used{help_indent_width + e.entry.size()};
            line += (used + help_column_gap <= descriptionColumn) ? std::format("{:{}}{}", "", descriptionColumn - used, e.description)
                                                                  : std::format("\n{:{}}{}", "", descriptionColumn, e.description);
          }

          return line + '\n';
        }
      };

      return entries | std::views::transform(row) | std::views::join | std::ranges::to<std::string>();
    }

    /** \brief The "Commands:" and "Options:" sections for a level, each present only if it has
        entries, help listed last among the options; nothing for a level with none.
     */
    [[nodiscard]]
    help_entry help_entry_of(const option_tree& opt)
    {
      const auto& description{root_weight(opt).description};
      return {entry_of(opt), description.substr(0, description.find('\n'))};
    }

    template<std::ranges::input_range Options>
    [[nodiscard]]
    std::string sections_of(const Options& level)
    {
      auto entries{
        [&level](auto kind) {
          return   level
                 | std::views::filter([kind](const option_tree& o){ return kind(root_weight(o)); })
                 | std::views::transform([](const option_tree& o){ return help_entry_of(o); })
                 | std::ranges::to<std::vector>();
        }
      };

      auto commands{entries([](const option& o){ return is_command(o); })};
      auto options{entries([](const option& o){ return !is_command(o); })};

      if(commands.empty() && options.empty()) return {};

      options.push_back({join(help_requests, ", "), "Describe the command or option this follows; alone, the top level"});

      const auto column{std::ranges::max(description_column(commands), description_column(options))};

      std::string sections{};
      if(!commands.empty())
        sections += std::format("\nCommands:\n{}", tabulate(commands, column));
      sections += std::format("\nOptions:\n{}", tabulate(options, column));

      return sections;
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
    , m_Options{&options}
    , m_ZerothArg{m_ArgCount ? m_Argv[0] : ""}
  {
    for(const auto& tree : options)
    {
      maths::traverse(maths::depth_first, tree, maths::ignore_disconnected_t{}, [&tree](const auto node) {
        const option& opt{tree.cbegin_node_weights()[node]};
        if(is_help_request(opt.name) || std::ranges::any_of(opt.aliases, [](const proper_string& alias){ return is_help_request(alias); }))
          throw std::logic_error{error(std::format("option \"{}\" is spelt as a help request", std::string_view{opt.name}))};
      });
    }

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

      if(is_help_request(arg))
      {
        m_Help = generate_help();
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
        m_OptionsEntered.push_back(currentOptionTree);
        parse(nested_options(currentOptionTree), currentOperationData, top_level::no);
        m_OptionsEntered.pop_back();

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

  [[nodiscard]]
  std::string argument_parser::generate_help() const
  {
    const auto topLevelOptions{top_level_options(*m_Options)};

    // The described option is the one most recently encountered;
    // when none has been encountered, the top level is described
    auto path{m_OptionsEntered};
    if(m_MostRecentlyEncounteredOption && (path.empty() || (path.back() != m_MostRecentlyEncounteredOption)))
      path.push_back(m_MostRecentlyEncounteredOption);

    auto usage{std::string{"Usage:"}};
    if(const auto executable{std::filesystem::path{m_ZerothArg}.stem().string()}; !executable.empty())
      usage += std::format(" {}", executable);

    auto names{path | std::views::transform([](const option_tree& o){ return std::format(" {}", std::string_view{root_weight(o).name}); })};
    usage = std::ranges::fold_left(names, std::move(usage), std::plus{});

    if(path.empty())
      return std::format("{}{}\n{}", usage, continuation_of(topLevelOptions), sections_of(topLevelOptions));

    const option_tree& described{path.back()};
    usage += continuation_of(described);
    if(const auto& description{root_weight(described).description}; !description.empty())
      usage += std::format("\n\n{}", description);

    return std::format("{}\n{}", usage, sections_of(nested_options(described)));
  }

  [[nodiscard]]
  outcome parse(int argc, char** argv, const options_forest& options)
  {
    argument_parser p{argc, argv, options};

    return p.get();
  }
}
