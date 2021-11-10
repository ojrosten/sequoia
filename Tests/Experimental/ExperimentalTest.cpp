////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ExperimentalTest.hpp"
#include "../Parsing/CommandLineArgumentsTestingUtilities.hpp"

#include "sequoia/Parsing/CommandLineArguments.hpp"
#include "sequoia/Maths/Graph/DynamicGraph.hpp"
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

      // Hack
      [[nodiscard]]
      friend auto operator==(const operation& lhs, const operation& rhs) noexcept
      {
        return (static_cast<bool>(lhs.early) == static_cast<bool>(rhs.early))
          && (static_cast<bool>(lhs.late) == static_cast<bool>(rhs.late))
          && (lhs.arguments == rhs.arguments);
      }

      [[nodiscard]]
      friend auto operator!=(const operation& lhs, const operation& rhs) noexcept
      {
        return !(lhs == rhs);
      }

    };

    using operations_tree = maths::graph<maths::directed_flavour::directed, maths::null_weight, operation>;
    using operations_forest = std::vector<operations_tree>;

    struct outcome
    {
      std::string zeroth_arg;
      operations_forest operations;
      std::string help{};
    };

    template<class Tree>
    class tree_adaptor
    {
    public:
      using size_type = Tree::size_type;

      tree_adaptor() = default;

      tree_adaptor(Tree& tree, size_type node)
        : m_pTree{&tree}
        , m_Node{node}
      {};

      [[nodiscard]]
      Tree& tree() noexcept
      {
        return *m_pTree;
      }

      [[nodiscard]]
      const Tree& tree() const noexcept
      {
        return *m_pTree;
      }

      [[nodiscard]]
      size_type node() const noexcept
      {
        return m_Node;
      }

      [[nodiscard]]
      operator bool() const noexcept
      {
        return (m_pTree != nullptr) && (m_Node < m_pTree->order());
      }

      [[nodiscard]]
      auto root_weight_iter() const
      {
        return m_pTree->cbegin_node_weights() + m_Node;
      }

      [[nodiscard]]
      const auto& root_weight() const
      {
        return m_pTree->cbegin_node_weights()[m_Node];
      }

      template<std::invocable<operation&> Fn>
      void mutate_root_weight(Fn fn)
      {
        m_pTree->mutate_node_weight(root_weight_iter(), fn);
      }
    private:
      Tree* m_pTree{};
      size_type m_Node{Tree::npos};
    };

    template<class Tree>
    class const_tree_adaptor
    {
    public:
      using size_type = Tree::size_type;

      using tree_reference_type = const Tree&;

      const_tree_adaptor() = default;

      const_tree_adaptor(const Tree& tree, size_type node)
        : m_pTree{&tree}
        , m_Node{node}
      {};

      [[nodiscard]]
      const Tree& tree() const noexcept
      {
        return *m_pTree;
      }

      [[nodiscard]]
      size_type node() const noexcept
      {
        return m_Node;
      }

      [[nodiscard]]
      operator bool() const noexcept
      {
        return (m_pTree != nullptr) && (m_Node < m_pTree->order());
      }

      [[nodiscard]]
      auto root_weight_iter() const
      {
        return m_pTree->cbegin_node_weights() + m_Node;
      }

      [[nodiscard]]
      const auto& root_weight() const
      {
        return m_pTree->cbegin_node_weights()[m_Node];
      }
    private:
      const Tree* m_pTree{};
      size_type m_Node{Tree::npos};
    };


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

    template<std::input_or_output_iterator Iterator, class TreeAdaptor>
    class forest_from_tree_dereference_policy
    {
    public:
      using value_type = typename std::iterator_traits<Iterator>::value_type;
      using proxy      = TreeAdaptor;
      using pointer    = typename std::iterator_traits<Iterator>::pointer;
      using reference  = typename std::iterator_traits<Iterator>::reference;
      using tree_reference_type = typename TreeAdaptor::tree_reference_type;

      // Hopefully get rid of this once MSVC bug fixed
      constexpr forest_from_tree_dereference_policy() = default;
      constexpr forest_from_tree_dereference_policy(tree_reference_type tree) : m_pTree{&tree} {}
      constexpr forest_from_tree_dereference_policy(const forest_from_tree_dereference_policy&) = default;

      [[nodiscard]]
      friend constexpr bool operator==(const forest_from_tree_dereference_policy&, const forest_from_tree_dereference_policy&) noexcept = default;

      [[nodiscard]]
      friend constexpr bool operator!=(const forest_from_tree_dereference_policy&, const forest_from_tree_dereference_policy&) noexcept = default;
    protected:
      constexpr forest_from_tree_dereference_policy(forest_from_tree_dereference_policy&&) = default;

      ~forest_from_tree_dereference_policy() = default;

      constexpr forest_from_tree_dereference_policy& operator=(const forest_from_tree_dereference_policy&) = default;
      constexpr forest_from_tree_dereference_policy& operator=(forest_from_tree_dereference_policy&&) = default;

      [[nodiscard]]
      constexpr proxy get(reference ref) const
      {
        return {*m_pTree, ref.target_node()};
      }

      [[nodiscard]]
      static constexpr pointer get_ptr(reference ref) noexcept { return &ref; }
    private:
      using tree_pointer_type = std::remove_reference_t<tree_reference_type>*;

      tree_pointer_type m_pTree{};
    };

    template<std::input_or_output_iterator Iterator, class Adaptor>
    using forest_iterator = utilities::iterator<Iterator, forest_dereference_policy<Iterator, Adaptor>>;

    template<std::input_or_output_iterator Iterator, class Adaptor>
    using forest_from_tree_iterator = utilities::iterator<Iterator, forest_from_tree_dereference_policy<Iterator, Adaptor>>;

    struct current_operation
    {
      tree_adaptor<operations_tree> tree{};
      bool current_op_complete{};
    };

    class argument_parser
    {
    public:
      using options_tree   = maths::graph<maths::directed_flavour::directed, maths::null_weight, option>;
      using options_forest = std::vector<options_tree>;
      using current_option_tree = const_tree_adaptor<options_tree>;

      argument_parser(int argc, char** argv, const options_forest& options);

      [[nodiscard]]
      outcome get() const
      {
        return {m_ZerothArg, m_Operations, m_Help};
      }
    private:
      enum class top_level {yes, no};

      operations_forest m_Operations{};
      int m_Index{1}, m_ArgCount{};
      char** m_Argv{};
      std::string m_ZerothArg{}, m_Help{};

      template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
      void parse(Iter beginOptions, Sentinel endOptions, current_operation currentOperation, top_level topLevel);

      template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
      [[nodiscard]]
      bool process_concatenated_aliases(Iter beginOptions, Sentinel endOptions, std::string_view arg, current_operation currentOperation, top_level topLevel);

      current_operation process_option(current_option_tree currentOptionTree, current_operation currentOperation, top_level topLevel);

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
    void argument_parser::parse(Iter beginOptions, Sentinel endOptions, current_operation currentOperation, top_level topLevel)
    {
      if(!m_Help.empty()) return;

      current_option_tree currentOptionTree{};
      while(m_Index < m_ArgCount)
      {
        currentOptionTree = {};

        if(std::string_view arg{m_Argv[m_Index++]}; !arg.empty())
        {
          if(!currentOperation.tree || currentOperation.current_op_complete)
          {
            const auto optionsIter{std::find_if(beginOptions, endOptions,
              [arg](const auto& tree) {
                return (tree.root_weight().name == arg) || is_alias(tree.root_weight(), arg);
              })
            };

            if(optionsIter == endOptions)
            {
              if(arg == "--help")
              {
                m_Help = generate_help(beginOptions, endOptions);
                return;
              }

              if(process_concatenated_aliases(beginOptions, endOptions, arg, currentOperation, topLevel))
                continue;

              if(topLevel == top_level::yes)
                throw std::runtime_error{error(std::string{"unrecognized option '"}.append(arg).append("'"))};

              // Roll back and see if the current argument makes sense at the previous level
              --m_Index;
              return;
            }

            currentOptionTree = *optionsIter;
            currentOperation = process_option(currentOptionTree, currentOperation, topLevel);
          }
          else
          {
            if(!currentOptionTree) throw std::logic_error{"Current option not found"};

            if(currentOperation.tree.root_weight().arguments.size() < currentOptionTree.root_weight().parameters.size())
            {
              currentOperation.tree.mutate_root_weight([arg](auto& w){ w.arguments.emplace_back(arg); });
            }
          }

          if(currentOperation.tree.root_weight().arguments.size() == currentOptionTree.root_weight().parameters.size())
          {
            currentOperation.current_op_complete = true;
            const auto node{currentOptionTree.node()};

            using iter_t = decltype(currentOptionTree.tree().cbegin_edges(node));
            using forest_iter = forest_from_tree_iterator<iter_t, const_tree_adaptor<options_tree>>;

            parse(forest_iter{currentOptionTree.tree().cbegin_edges(node), currentOptionTree.tree()},
                  forest_iter{currentOptionTree.tree().cend_edges(node), currentOptionTree.tree()},
                  currentOperation,
                  top_level::no);
          }
        }
      }

      if(    !m_Operations.empty()
         && currentOptionTree
         && (currentOperation.tree.root_weight().arguments.size() != currentOptionTree.root_weight().parameters.size()))
      {
        const auto& params{currentOptionTree.root_weight().parameters};
        const auto expected{params.size()};
        auto mess{std::string{"while parsing option \""}
                    .append(currentOptionTree.root_weight().name)
                    .append("\": expected ")
                    .append(std::to_string(expected))
                    .append(pluralize(expected, "argument"))
                    .append(", [")};

        for(auto i{params.begin()}; i != params.end(); ++i)
        {
          mess.append(*i);
          if(std::distance(i, params.end()) > 1) mess.append(", ");
        }

        const auto actual{currentOperation.tree.root_weight().arguments.size()};
        mess.append("], but found ").append(std::to_string(actual)).append(pluralize(actual, "argument"));

        throw std::runtime_error{error(mess)};
      }
    }

    current_operation argument_parser::process_option(current_option_tree currentOptionTree, current_operation currentOperation, top_level topLevel)
    {
      if(topLevel == top_level::yes)
      {
        if(!currentOptionTree.root_weight().early && !currentOptionTree.root_weight().late)
          throw std::logic_error{error("Commandline option not bound to a function object")};

        m_Operations.push_back({{currentOptionTree.root_weight().early, currentOptionTree.root_weight().late, {}}, forward_tree_type{}});
        currentOperation = {{m_Operations.back(), 0}};
      }
      else
      {
        if(m_Operations.empty() || !currentOperation.tree)
          throw std::logic_error{"Unable to find commandline operation"};

        auto& operationTree{m_Operations.back()};

        const auto node{operationTree.add_node(currentOptionTree.root_weight().early, currentOptionTree.root_weight().late)};
        operationTree.join(currentOptionTree.node(), node);
        currentOperation = {{m_Operations.back(), node}};

        // TO DO: incorporate this
        /*while(i != nestedOperations.end())
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
        }*/
      }

      return currentOperation;
    }

    template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
    [[nodiscard]]
    bool argument_parser::process_concatenated_aliases(Iter beginOptions, Sentinel endOptions, std::string_view arg, current_operation currentOperation, top_level topLevel)
    {
      if((arg.size() > 2) && (arg[0] == '-') && (arg[1] != ' '))
      {
        for(auto j{arg.cbegin() + 1}; j != arg.cend(); ++j)
        {
          const auto c{*j};
          if(c != '-')
          {
            const auto alias{std::string{'-'} + c};

            auto optionsIter{std::find_if(beginOptions, endOptions,
              [arg](const auto& tree) { return tree.root_weight().name == arg; })};

            // need to deal with the case where rollback is allowed
            if(optionsIter == endOptions)
            {
              throw std::runtime_error{"Unrecognized concatenated alias: " + std::string{arg}};
            }

            const current_option_tree currentOptionTree{*optionsIter};
            process_option(currentOptionTree, currentOperation, topLevel);
          }
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
        const auto& optTree{(*(beginOptions++)).tree()};

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
            ind.append(' ', 2);
          }
        };

        auto nodeLate{
          [&ind](auto){ ind.trim(2); }
        };

        depth_first_search(optTree, ignore_disconnected_t{}, nodeEarly, nodeLate);
      }

      return help;
    }
  }

  [[nodiscard]]
  std::string_view experimental_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void experimental_test::run_tests()
  {
    test_flat_parsing();

  }

  void experimental_test::test_flat_parsing()
  {
    using namespace sequoia::parsing::commandline;
    using fo = function_object;

    {
      check_exception_thrown<std::logic_error>(LINE("Empty name"), []() { return option{"", {}, {}, fo{}}; });
      check_exception_thrown<std::logic_error>(LINE("Empty alias"), []() { return option{"test", {""}, {}, fo{}}; });
      check_exception_thrown<std::logic_error>(LINE("Empty parameter"), []() { return option{"test", {}, {""}, fo{}}; });
    }

    {
      check_weak_equivalence(LINE(""), parse(0, nullptr, {}), outcome{});
    }

    {
      commandline_arguments a{"foo", "--async"};

      check_weak_equivalence(LINE("Early"), parse(a.size(), a.get(), {{"--async", {}, {}, fo{}}}), outcome{"foo", {{fo{}, nullptr, {}}}});
      check_weak_equivalence(LINE("Late"), parse(a.size(), a.get(), {{"--async", {}, {}, nullptr, {}, fo{}}}), outcome{"foo", {{nullptr, fo{}, {}}}});
      check_weak_equivalence(LINE("Both"), parse(a.size(), a.get(), {{"--async", {}, {}, fo{"x"}, {}, fo{"y"}}}), outcome{"foo", {{fo{"x"}, fo{"y"}, {}}}});
    }

    {
      commandline_arguments a{"bar", "-a"};

      check_weak_equivalence(LINE(""), parse(a.size(), a.get(), {{"--async", {"-a"}, {}, fo{}}}), outcome{"bar", {{fo{}, nullptr, {}}}});
    }

    {
      commandline_arguments a{"bar", "", "-a"};

      check_weak_equivalence(LINE("Ignored empty option"), parse(a.size(), a.get(), {{"--async", {"-a"}, {}, fo{}}}), outcome{"bar", {{fo{}, nullptr, {}}}});
    }

    {
      commandline_arguments a{"foo", "-a"};

      check_weak_equivalence(LINE(""), parse(a.size(), a.get(), {{"--async", {"-as", "-a"}, {}, fo{}}}), outcome{"foo", {{fo{}, nullptr, {}}}});
    }

    {
      commandline_arguments a{"foo", "--asyng"};

      check_exception_thrown<std::runtime_error>(LINE("Unexpected argument"), [&a](){
        return parse(a.size(), a.get(), {{"--async", {}, {}, fo{}}});
        });
    }

    {
      commandline_arguments a{"foo", "-a"};

      check_exception_thrown<std::runtime_error>(LINE("Unexpected argument"), [&a](){
        return parse(a.size(), a.get(), {{"--async", {"-as"}, {}, fo{}}});
        });
    }
/*
    {
      commandline_arguments a{"foo", "-av"};

      check_weak_equivalence(LINE(""),
        parse(a.size(), a.get(), {{"--async", {"-a"}, {}, fo{}},
                                 {"--verbose", {"-v"}, {}, fo{}}}),
        outcome{"foo", {{fo{}, nullptr, {}}, {fo{}, nullptr, {}}}});
    }

    {
      commandline_arguments a{"foo", "-a-v"};

      check_weak_equivalence(LINE(""),
        parse(a.size(), a.get(), {{"--async", {"-a"}, {}, fo{}},
                                 {"--verbose", {"-v"}, {}, fo{}}}),
        outcome{"foo", {{fo{}, nullptr, {}}, {fo{}, nullptr, {}}}});
    }

    {
      commandline_arguments a{"foo", "-av", "-p"};

      check_weak_equivalence(LINE(""),
        parse(a.size(), a.get(), {{"--async",   {"-a"}, {}, fo{}},
                                 {"--verbose", {"-v"}, {}, fo{}},
                                 {"--pause",   {"-p"}, {}, fo{}}}),
        outcome{"foo", {{fo{}, nullptr, {}}, {fo{}, nullptr, {}}, {fo{}, nullptr, {}}}});
    }

    {
      commandline_arguments a{"foo", "-ac"};

      check_exception_thrown<std::runtime_error>(LINE("Unexpected argument"), [&a](){
        return parse(a.size(), a.get(), {{"--async", {"-a"}, {}, fo{}}});
        });
    }

    {
      commandline_arguments a{"foo", "test", "thing"};

      check_weak_equivalence(LINE(""), parse(a.size(), a.get(), {{"test", {}, {"case"}, fo{}}}), outcome{"foo", {{fo{}, nullptr, {"thing"}}}});
    }

    {
      commandline_arguments a{"foo", "t", "thing"};

      check_weak_equivalence(LINE(""), parse(a.size(), a.get(), {{"test", {"t"}, {"case"}, fo{}}}), outcome{"foo", {{fo{}, nullptr, {"thing"}}}});
    }

    {
      commandline_arguments a{"foo", "test", ""};

      check_weak_equivalence(LINE("Empty parameter"), parse(a.size(), a.get(), {{"test", {}, {"case"}, fo{}}}), outcome{"foo", {{fo{}, nullptr, {""}}}});
    }

    {
      commandline_arguments a{"foo", "test"};

      check_exception_thrown<std::runtime_error>(LINE("Final argument missing"),
        [&a](){
          return parse(a.size(), a.get(), {{"test", {}, {"case"}, fo{}}});
        });
    }

    {
      commandline_arguments a{"foo", "create", "class", "dir"};

      check_weak_equivalence(LINE(""),
        parse(a.size(), a.get(), {{"create", {}, {"class_name", "directory"}, fo{}}}),
        outcome{"foo", {{fo{}, nullptr, {"class", "dir"}}}});
    }

    {
      commandline_arguments a{"foo", "--async", "create", "class", "dir"};

      check_weak_equivalence(LINE(""),
        parse(a.size(), a.get(), {{"create",  {}, {"class_name", "directory"}, fo{}},
                                   {"--async", {}, {}, fo{}}}),
        outcome{"foo", {{fo{}, nullptr, {}}, {fo{}, nullptr, {"class", "dir"}}}});
    }

    {
      commandline_arguments a{"foo", "--async", "create", "class", "dir"};

      check_weak_equivalence(LINE(""),
        argument_parser{a.size(), a.get(), { {"create",  {}, {"class_name", "directory"}, fo{}},
                                   {"--async", {}, {}, fo{}} }},
        outcome{"foo", {{fo{}, nullptr, {}}, {fo{}, nullptr, {"class", "dir"}}}});
    }
    */
  }
}
