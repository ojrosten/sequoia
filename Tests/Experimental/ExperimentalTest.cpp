////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ExperimentalTest.hpp"
#include "../Parsing/CommandLineArgumentsTestingUtilities.hpp"
#include "../Maths/Graph/GraphTestingUtilities.hpp"

#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/Maths/Graph/DynamicTree.hpp"
#include "sequoia/Maths/Graph/GraphTraversalFunctions.hpp"

#include <functional>

namespace sequoia::testing
{
  namespace experimental
  {
    using namespace maths;
    using namespace parsing::commandline;

    struct option
    {
      proper_string name;
      param_list aliases,
                 parameters;
      executor early{},
               late{};
    };

    struct operation
    {
      executor early, late;
      arg_list arguments;
    };

    using operations_tree = maths::tree<maths::directed_flavour::directed, maths::tree_link_direction::forward, maths::null_weight, operation>;
    using operations_forest = std::vector<operations_tree>;

    struct outcome
    {
      std::string zeroth_arg;
      operations_forest operations;
      std::string help{};
    };

    template<dynamic_tree T>
    [[nodiscard]]
    auto root_weight_iter(const basic_tree_adaptor<T>& adaptor)
    {
      return adaptor.tree().cbegin_node_weights() + adaptor.node();
    }
    
    template<dynamic_tree T>
    [[nodiscard]]
    const auto& root_weight(const basic_tree_adaptor<T>& adaptor)
    {
      return adaptor.tree().cbegin_node_weights()[adaptor.node()];
    }

    template<dynamic_tree T, std::invocable<operation&> Fn>
    void mutate_root_weight(basic_tree_adaptor<T>& adaptor, Fn fn)
    {
      adaptor.tree().mutate_node_weight(root_weight_iter(adaptor), fn);
    }

    template<std::input_or_output_iterator Iterator, class TreeAdaptor>
    class forest_dereference_policy
    {
    public:
      using value_type = typename std::iterator_traits<Iterator>::value_type;
      using proxy      = TreeAdaptor;
      using pointer    = typename std::iterator_traits<Iterator>::pointer;
      using reference  = typename std::iterator_traits<Iterator>::reference;

      constexpr forest_dereference_policy() = default;
      constexpr forest_dereference_policy(const forest_dereference_policy&) = default;

      [[nodiscard]]
      friend constexpr bool operator==(const forest_dereference_policy&, const forest_dereference_policy&) noexcept = default;

      [[nodiscard]]
      friend constexpr bool operator!=(const forest_dereference_policy&, const forest_dereference_policy&) noexcept = default;
    protected:
      constexpr forest_dereference_policy(forest_dereference_policy&&) = default;

      ~forest_dereference_policy() = default;

      constexpr forest_dereference_policy& operator=(const forest_dereference_policy&) = default;
      constexpr forest_dereference_policy& operator=(forest_dereference_policy&&) = default;

      [[nodiscard]]
      constexpr proxy get(reference ref) const noexcept
      {
        return {ref, 0};
      }

      [[nodiscard]]
      static constexpr pointer get_ptr(reference ref) noexcept { return &ref; }
    };

    template<std::input_or_output_iterator Iterator, class Adaptor>
    using forest_iterator = utilities::iterator<Iterator, forest_dereference_policy<Iterator, Adaptor>>;

    using options_tree   = maths::tree<maths::directed_flavour::directed, maths::tree_link_direction::forward, maths::null_weight, option>;
    using options_forest = std::vector<options_tree>;
    using option_tree    = const_tree_adaptor<options_tree>;
    using operation_tree = tree_adaptor<operations_tree>;

    class argument_parser
    {
    public:

      argument_parser(int argc, char** argv, const options_forest& options);

      [[nodiscard]]
      outcome get() const
      {
        return {m_ZerothArg, m_Operations, m_Help};
      }
    private:
      enum class top_level {yes, no};

      struct operation_data
      {
        operation_tree oper_tree{};
        std::size_t    saturated_args{};
      };

      operations_forest m_Operations{};
      int m_Index{1}, m_ArgCount{};
      char** m_Argv{};
      std::string m_ZerothArg{}, m_Help{};

      template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
      void parse(Iter beginOptions, Sentinel endOptions, const operation_data& previousOperationData, top_level topLevel);

      template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
      [[nodiscard]]
      bool process_concatenated_aliases(Iter beginOptions, Sentinel endOptions, std::string_view arg, operation_data currentOperationData, top_level topLevel);

      auto process_option(option_tree currentOptionTree, operation_data currentOperationData, top_level topLevel) -> operation_data;

      template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
      [[nodiscard]]
      static std::string generate_help(Iter beginOptions, Sentinel endOptions);

      static bool is_alias(const option& opt, std::string_view s);
    };

    argument_parser::argument_parser(int argc, char** argv, const options_forest& options)
      : m_ArgCount{argc}
      , m_Argv{argv}
      , m_ZerothArg{m_ArgCount ? m_Argv[0] : ""}
    {
      using iter_t = decltype(options.begin());
      using forest_iter = forest_iterator<iter_t, const_tree_adaptor<options_tree>>;

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

          if(   root_weight(currentOperationData.oper_tree).arguments.size() 
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
          using forest_iter = forest_from_tree_iterator<iter_t, const_tree_adaptor<options_tree>>;

          parse(forest_iter{currentOptionTree.tree().cbegin_edges(node), currentOptionTree.tree()},
                forest_iter{currentOptionTree.tree().cend_edges(node), currentOptionTree.tree()},
                currentOperationData,
                top_level::no);

          currentOptionTree = {};
          currentOperationData = previousOperationData;
        }
      }

      if(   !m_Operations.empty()
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

        m_Operations.push_back({{root_weight(currentOptionTree).early, root_weight(currentOptionTree).late, {}}});
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
          currentOperationData = {currentOperationData.oper_tree, root_weight(currentOperationData.oper_tree).arguments.size()};
        }
      }

      return currentOperationData;
    }

    template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
    [[nodiscard]]
    bool argument_parser::process_concatenated_aliases(Iter beginOptions, Sentinel endOptions, std::string_view arg, operation_data currentOperationData, top_level topLevel)
    {
      if(!(arg.size() > 2) && (arg[0] == '-') && (arg[1] != ' '))
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
          [&](const auto n){
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
          [&ind](auto){ ind.trim(2); }
        };

        depth_first_search(optTree, ignore_disconnected_t{subTreeRootNode}, nodeEarly, nodeLate);
      }

      return help;
    }

    [[nodiscard]]
    outcome parse(int argc, char** argv, const options_forest& options)
    {
      experimental::argument_parser p{argc, argv, options};

      return p.get();
    }
  }

  template<>
  struct value_tester<experimental::operation>
  {
    using type = experimental::operation;

    template<test_mode Mode>
    static void test(weak_equivalence_check_t, test_logger<Mode>& logger, const type& operation, const type& prediction)
    {
      check_executor(logger, operation.early, prediction.early, "early");
      check_executor(logger, operation.late, prediction.late, "late");
      check(equality, "Operation Parameters differ", logger, operation.arguments, prediction.arguments);
    }
  private:
    using executor = experimental::executor;

    template<test_mode Mode>
    static void check_executor(test_logger<Mode>& logger, const executor& operation, const executor& prediction, std::string_view tag)
    {
      const bool consistent{(operation && prediction) || (!operation && !prediction)};
      testing::check(std::string{"Existence of"}.append(tag).append(" function objects differs"), logger, consistent);

      if(operation && prediction)
      {
        check(equality,
              "Function object tag",
              logger,
              operation.target<function_object>()->tag,
              prediction.target<function_object>()->tag);
      }
    }
  };


  template<>
  struct value_tester<experimental::outcome>
  {
    using type = experimental::outcome;

    template<test_mode Mode>
    static void test(weak_equivalence_check_t, test_logger<Mode>& logger, const type& obtained, const type& prediction)
    {
      check(equality, "Zeroth Argument", logger, obtained.zeroth_arg, prediction.zeroth_arg);
      check(weak_equivalence, "Operations", logger, obtained.operations, prediction.operations);
      check(equality, "Help", logger, obtained.help, prediction.help);
    }
  };

  template<>
  struct value_tester<experimental::argument_parser>
  {
    using type = experimental::argument_parser;
    using prediction_type = experimental::outcome;

    template<test_mode Mode>
    static void test(weak_equivalence_check_t, test_logger<Mode>& logger, const type& obtained, const prediction_type& prediction)
    {
      check(weak_equivalence, "", logger, obtained.get(), prediction);
    }
  };

  [[nodiscard]]
  std::string_view experimental_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void experimental_test::run_tests()
  {
    test_flat_parsing();
    test_flat_parsing_help();
    test_nested_parsing();
    test_nested_parsing_help();
  }

  void experimental_test::test_flat_parsing()
  {
    using namespace experimental;
    using fo = function_object;

    {
      check_exception_thrown<std::logic_error>(LINE("Empty name"),      []() { return option{"", {}, {}, fo{}}; });
      check_exception_thrown<std::logic_error>(LINE("Empty alias"),     []() { return option{"test", {""}, {}, fo{}}; });
      check_exception_thrown<std::logic_error>(LINE("Empty parameter"), []() { return option{"test", {}, {""}, fo{}}; });
    }

    {
      check(weak_equivalence, LINE(""), experimental::parse(0, nullptr, {}), experimental::outcome{});
    }

    {
      commandline_arguments a{"foo", "--async"};

      check(weak_equivalence,
            LINE("Early"),
            experimental::parse(a.size(), a.get(), {{{"--async", {}, {}, fo{}}}}),
            experimental::outcome{"foo", {{{fo{}, nullptr, {}}}}});

      check(weak_equivalence,
            LINE("Late"),
            experimental::parse(a.size(), a.get(), {{{"--async", {}, {}, nullptr, fo{}}}}),
            experimental::outcome{"foo", {{{nullptr, fo{}, {}}}}});
      
      check(weak_equivalence,
            LINE("Both"),
            experimental::parse(a.size(), a.get(), {{{"--async", {}, {}, fo{"x"}, fo{"y"}}}}),
            experimental::outcome{"foo", {{{fo{"x"}, fo{"y"}, {}}}}});
    }

    {
      commandline_arguments a{"bar", "-a"};

      check(weak_equivalence,
            LINE("Argument shorthand"),
            experimental::parse(a.size(), a.get(), {{{"--async", {"-a"}, {}, fo{}}}}),
            experimental::outcome{"bar", {{{fo{}, nullptr, {}}}}});
    }

    {
      commandline_arguments a{"bar", "", "-a"};

      check(weak_equivalence,
            LINE("Ignored empty option"),
            experimental::parse(a.size(), a.get(), {{{"--async", {"-a"}, {}, fo{}}}}),
            experimental::outcome{"bar", {{{fo{}, nullptr, {}}}}});
    }

    {
      commandline_arguments a{"foo", "-a"};

      check(weak_equivalence,
            LINE("Multiple shorthands for a single option"),
            experimental::parse(a.size(), a.get(), {{{"--async", {"-as", "-a"}, {}, fo{}}}}),
            experimental::outcome{"foo", {{{fo{}, nullptr, {}}}}});
    }

    {
      commandline_arguments a{"foo", "--asyng"};

      check_exception_thrown<std::runtime_error>(LINE("Unexpected argument"), [&a](){
        return experimental::parse(a.size(), a.get(), {{{"--async", {}, {}, fo{}}}});
      });
    }

    {
      commandline_arguments a{"foo", "-a"};

      check_exception_thrown<std::runtime_error>(LINE("Unexpected argument"), [&a](){
        return experimental::parse(a.size(), a.get(), {{{"--async", {"-as"}, {}, fo{}}}});
        });
    }

    {
      commandline_arguments a{"foo", "-av"};

      check(weak_equivalence,
            LINE("Concatenated alias"),
            experimental::parse(a.size(), a.get(), {{{"--async",   {"-a"}, {}, fo{}}},
                                                    {{"--verbose", {"-v"}, {}, fo{}}}}),
            experimental::outcome{"foo", { {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}}} }});
    }

    {
      commandline_arguments a{"foo", "-a-v"};

      check(weak_equivalence,
            LINE("Concatenated alias with dash"),
            experimental::parse(a.size(), a.get(), {{{"--async", {"-a"}, {}, fo{}}},
                                                    {{"--verbose", {"-v"}, {}, fo{}}}}),
            experimental::outcome{"foo", { {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}}} }});
    }

    {
      commandline_arguments a{"foo", "-av", "-p"};

      check(weak_equivalence,
            LINE("Concatenated alias and single alias"),
            experimental::parse(a.size(), a.get(), {{{"--async",   {"-a"}, {}, fo{}}},
                                                    {{"--verbose", {"-v"}, {}, fo{}}},
                                                    {{"--pause",   {"-p"}, {}, fo{}}}}),
            experimental::outcome{"foo", { {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}}} }});
    }

    {
      commandline_arguments a{"foo", "-ac"};

      check_exception_thrown<std::runtime_error>(LINE("Concatenated alias with only partial match"), [&a](){
        return experimental::parse(a.size(), a.get(), {{{"--async", {"-a"}, {}, fo{}}}});
      });
    }

    {
      commandline_arguments a{"foo", "test", "thing"};

      check(weak_equivalence,
            LINE("Option with paramater"),
            experimental::parse(a.size(), a.get(), {{{"test", {}, {"case"}, fo{}}}}),
            experimental::outcome{"foo", {{{fo{}, nullptr, {"thing"}}}}});
    }

    {
      commandline_arguments a{"foo", "t", "thing"};

      check(weak_equivalence,
            LINE("Aliased option with parameter"),
            experimental::parse(a.size(), a.get(), {{{"test", {"t"}, {"case"}, fo{}}}}),
            experimental::outcome{"foo", {{{fo{}, nullptr, {"thing"}}}}});
    }

    {
      commandline_arguments a{"foo", "test", ""};

      check(weak_equivalence,
            LINE("Empty parameter"),
            experimental::parse(a.size(), a.get(), {{{"test", {}, {"case"}, fo{}}}}),
            experimental::outcome{"foo", {{{fo{}, nullptr, {""}}}}});
    }

    {
      commandline_arguments a{"foo", "test"};

      check_exception_thrown<std::runtime_error>(LINE("Final argument missing"),
        [&a](){
          return experimental::parse(a.size(), a.get(), {{{"test", {}, {"case"}, fo{}}}});
        });
    }

    {
      commandline_arguments a{"foo", "create", "class", "dir"};

      check(weak_equivalence,
            LINE("Two parameter option"),
            experimental::parse(a.size(), a.get(), {{{"create", {}, {"class_name", "directory"}, fo{}}}}),
            experimental::outcome{"foo", {{{fo{}, nullptr, {"class", "dir"}}}}});
    }

    {
      commandline_arguments a{"foo", "--async", "create", "class", "dir"};

      check(weak_equivalence,
            LINE("Two options"),
            experimental::parse(a.size(), a.get(), {{{"create",  {}, {"class_name", "directory"}, fo{}}},
                                                    {{"--async", {}, {}, fo{}}}}),
            experimental::outcome{"foo", { {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {"class", "dir"}}} }});
    }

    {
      commandline_arguments a{"foo", "--async", "create", "class", "dir"};

      check(weak_equivalence,
            LINE("Two options, invoked with argument_parser"),
            experimental::argument_parser{a.size(), a.get(), { {{"create",  {}, {"class_name", "directory"}, fo{}}},
                                                               {{"--async", {}, {}, fo{}}} }},
            experimental::outcome{"foo", { {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {"class", "dir"}}}}});
    }
  }

  void experimental_test::test_flat_parsing_help()
  {
    using namespace sequoia::parsing::commandline;
    using fo = function_object;

    {
      commandline_arguments a{"foo", "--help"};

      check(weak_equivalence,
            LINE("Single option help"),
            experimental::parse(a.size(), a.get(), {{{"--async", {}, {}, fo{}}}}),
            experimental::outcome{"foo", {}, "--async\n"});
    }

    {
      commandline_arguments a{"foo", "--help"};

      check(weak_equivalence,
            LINE("Single option alias help"),
            experimental::parse(a.size(), a.get(), {{{"--async", {"-a"}, {}, fo{}}}}),
            experimental::outcome{"foo", {}, "--async | -a |\n"});
    }

    {
      commandline_arguments a{"foo", "--help"};

      check(weak_equivalence,
            LINE("Single option multi-alias help"),
            experimental::parse(a.size(), a.get(), {{{"--async", {"-a","-as"}, {}, fo{}}}}),
            experimental::outcome{"foo", {}, "--async | -a -as |\n"});
    }

    {
      commandline_arguments a{"foo", "--help"};

      check(weak_equivalence,
        LINE("Multi-option help"),
        experimental::parse(a.size(), a.get(), {{{"create",  {"-c"}, {"class_name", "directory"}, fo{}}},
                                                {{"--async", {}, {}, fo{}}}}),
        experimental::outcome{"foo", {}, "create | -c | class_name, directory\n--async\n"});
    }

    {
      commandline_arguments a{"foo", "--help"};

      check(weak_equivalence,
            LINE("Multi-option help, with argument_parser"),
            experimental::argument_parser{a.size(), a.get(), { {{"create",  {"-c"}, {"class_name", "directory"}, fo{}}},
                                                               {{"--async", {}, {}, fo{}}} }},
            experimental::outcome{"foo", {}, "create | -c | class_name, directory\n--async\n"});
    }
  }

  void experimental_test::test_nested_parsing()
  {
    using namespace sequoia::parsing::commandline;
    using fo = function_object;

    {
      commandline_arguments a{"", "create", "class", "dir"};

      check(weak_equivalence,
            LINE("A nested option, not bound to a function object, not called"),
            experimental::parse(a.size(),
                                a.get(),
                                {{ {"create", {}, {"class_name", "directory"}, fo{}, {},
                                     { {{"--equivalent-type", {}, {"type"}}} } } }}),
            experimental::outcome{"", {{{fo{}, nullptr, {"class", "dir"}}}}});
    }

    {
      commandline_arguments a{"bar", "create", "class", "dir", "--equivalent-type", "foo"};

      check(weak_equivalence,
            LINE("A nested option, not bound to a function object, utilized"),
            experimental::parse(a.size(),
                                a.get(),
                                {{ {"create", {}, {"class_name", "directory"}, fo{}, {},
                                     { {{"--equivalent-type", {}, {"type"}}} } } }}),
            experimental::outcome{"bar", {{{fo{}, nullptr, {"class", "dir", "foo"}}}}});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo"};

      check(weak_equivalence,
            LINE("A nested option, bound to a function object, utilized"),
            experimental::parse(a.size(),
                                a.get(),
                                {{ {"create", {}, {"class_name", "directory"}, fo{}, {},
                                     { {{"--equivalent-type", {}, {"type"}, fo{}}} } } }}),
            experimental::outcome{"", {{{ fo{}, nullptr, {"class", "dir"}, { { fo{}, nullptr, {"foo"}} } }}}});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo", "--generate", "bar"};

      check(weak_equivalence,
            LINE("Two nested options"),
            experimental::parse(a.size(),
                                a.get(),
                                {{ {"create", {}, {"class_name", "directory"}, fo{}, {},
                                     { {{"--equivalent-type", {}, {"type"}}},
                                       {{"--generate",        {}, {"file"}, fo{}} }}}}}),
            experimental::outcome{"", {{{ fo{}, nullptr, {"class", "dir", "foo"}, { { fo{}, nullptr, {"bar"}} } }}}});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo", "-v"};

      check(weak_equivalence,
            LINE("Two options, one with nesting, the other aliased"),
            experimental::parse(a.size(),
                                a.get(),
                                { {{"create", {}, {"class_name", "directory"}, fo{}, {},
                                     { {"--equivalent-type", {}, {"type"}} } 
                                  }},
                                  {{"--verbose", {"-v"}, {}, fo{}}}}),
            experimental::outcome{"", {{{fo{}, nullptr, {"class", "dir", "foo"}}}, {{fo{}, nullptr, {}}}}});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo", "-v", "-a"};

      check(weak_equivalence,
            LINE("Three options, one with nesting, the other two aliased"),
            experimental::parse(a.size(),
                                a.get(),
                                { {{"create", {}, {"class_name", "directory"}, fo{}, {},
                                     { {"--equivalent-type", {}, {"type"}} }
                                  }},
                                  {{"--verbose", {"-v"}, {}, fo{}}},
                                  {{"--async", {"-a"}, {}, fo{}}}}),
            experimental::outcome{"", {{{fo{}, nullptr, {"class", "dir", "foo"}}}, {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}}}}});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo", "-a", "-v"};

      check(weak_equivalence,
            LINE("Three options, one with nesting, the other two aliased; invoked with concatenated alias"),
            experimental::parse(a.size(),
                                a.get(),
                                { {{"create", {}, {"class_name", "directory"}, fo{}, {},
                                      { {"--equivalent-type", {}, {"type"}} }
                                   }},
                                   {{"--verbose", {"-v"}, {}, fo{}}},
                                   {{"--async", {"-a"}, {}, fo{}}}}),
            experimental::outcome{"", {{{fo{}, nullptr, {"class", "dir", "foo"}}}, {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}}}}});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo", "-va"};

      check(weak_equivalence,
            LINE("Three options, one with nesting, the other two aliased; invoked with concatenated alias"),
            experimental::parse(a.size(),
                                a.get(),
                                {{{"create", {}, {"class_name", "directory"}, fo{}, {},
                                     { {"--equivalent-type", {}, {"type"}} }
                                  }},
                                  {{"--verbose", {"-v"}, {}, fo{}}},
                                  {{"--async", {"-a"}, {}, fo{}}}}),
            experimental::outcome{"", {{{fo{}, nullptr, {"class", "dir", "foo"}}}, {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}}}}});
    }

    {
      commandline_arguments a{"", "create", "regular_test", "maybe<class T>", "std::optional<T>"};

      check(weak_equivalence,
            LINE("Nested mode"),
            experimental::parse(a.size(),
                                a.get(),
                                {{{"create", {"c"}, {}, fo{}, {},
                                     {{ "regular_test",
                                         {"regular"},
                                         {"qualified::class_name<class T>", "equivalent type"},
                                         fo{}
                                     }}
                                }}}),
            experimental::outcome{"", {{{fo{}, nullptr, {}, {{fo{}, nullptr, {"maybe<class T>", "std::optional<T>"}}} }}}});
    }

    {
      commandline_arguments a{"", "c", "regular", "maybe<class T>", "std::optional<T>"};

      check(weak_equivalence,
            LINE("Nested mode, invoked with short-hand"),
            experimental::parse(a.size(),
                                a.get(),
                                {{{"create", {"c"}, {}, fo{}, {},
                                     {{ "regular_test",
                                         {"regular"},
                                         {"qualified::class_name<class T>", "equivalent type"},
                                         fo{}
                                     }}
                                }}}),
            experimental::outcome{"", {{{fo{}, nullptr, {}, {{fo{}, nullptr, {"maybe<class T>", "std::optional<T>"}}} }}}});
    }

    {
      commandline_arguments a{"", "create", "create", "regular", "maybe<class T>", "std::optional<T>"};

      // This is subtle! After the first 'create' is parsed, the second one is not
      // recognized as a nested option and so it re-parsed as a top-level option.
      check(weak_equivalence,
            LINE("Nested mode with duplicated command"),
            experimental::parse(a.size(),
                                a.get(),
                                {{{"create", {"c"}, {}, fo{}, {},
                                     {{ "regular_test",
                                         {"regular"},
                                         {"qualified::class_name<class T>", "equivalent type"},
                                         fo{}
                                     }}
                                }}}),
             experimental::outcome{"", { {{fo{}, nullptr, {}}}, {{fo{}, nullptr, {}, {{fo{}, nullptr, {"maybe<class T>", "std::optional<T>"}}} }} }});
    }

    {
      commandline_arguments a{"", "create", "class", "dir", "--equivalent-type", "foo", "blah"};

      check_exception_thrown<std::runtime_error>(LINE("Two options, one with nesting, illegal argument"),
        [&a]() {
          return experimental::parse(a.size(),
                                     a.get(),
                                     {{{"create", {}, {"class_name", "directory"}, fo{}, {},
                                          { {"--equivalent-type", {}, {"type"}} }
                                       }},
                                       {{"--verbose", {"-v"}, {}, fo{}}}});
        });
    }
  }

  void experimental_test::test_nested_parsing_help()
  {
    using namespace sequoia::parsing::commandline;
    using fo = function_object;

    {
      commandline_arguments a{"", "--help"};

      check(weak_equivalence,
            LINE("Nested help"),
            experimental::parse(a.size(),
                                a.get(),
                                { {{"create", {"c"}, {}, fo{}, {},
                                     {{"regular_test",
                                        {"regular"},
                                        {"qualified::class_name<class T>", "equivalent_type"},
                                        fo{}
                                     }}
                                   }} }),
            experimental::outcome{"",
                                  {},
                                  "create | c |\n  regular_test | regular | "
                                  "qualified::class_name<class T>, equivalent_type\n"});
    }

    {
      commandline_arguments a{"", "create", "--help"};

      check(weak_equivalence,
            LINE("Help requested for nested option"),
            experimental::parse(a.size(),
                                a.get(),
                                { {{"create", {"c"}, {}, fo{}, {},
                                     {{"regular_test",
                                        {"regular"},
                                        {"qualified::class_name<class T>", "equivalent_type"},
                                        fo{}
                                     }}
                                }} }),
            experimental::outcome{"",
                                  {{{fo{}, nullptr, {}}}},
                                  "regular_test | regular | "
                                  "qualified::class_name<class T>, equivalent_type\n"});
    }
  }
}
