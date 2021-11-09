////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ExperimentalTest.hpp"

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

    template<std::input_or_output_iterator Iter>
    [[nodiscard]]
    auto root(Iter i)
    {
      return *(i->cbegin_node_weights());
    }

    template<maths::network N>
    [[nodiscard]]
    auto& root(N& tree)
    {
      return *tree.cbegin_node_weights();
    }

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
      constexpr forest_dereference_policy(TreeAdaptor adaptor) : m_Adaptor{adaptor} {}
      constexpr forest_dereference_policy(const forest_dereference_policy&) = default;

      [[nodiscard]]
      constexpr proxy adaptor() const noexcept { return m_Adaptor; }

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
        // won't work in general
        return {ref};
      }

      [[nodiscard]]
      static constexpr pointer get_ptr(reference ref) noexcept { return &ref; }
    private:
      TreeAdaptor m_Adaptor{};
    };

    template<std::input_or_output_iterator Iterator, class Adaptor>
    using forest_iterator = utilities::iterator<Iterator, forest_dereference_policy<Iterator, Adaptor>>;

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
      operations_forest m_Operations{};
      int m_Index{1}, m_ArgCount{};
      char** m_Argv{};
      std::string m_ZerothArg{}, m_Help{};

      template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
      bool parse(Iter beginOptions, Sentinel endOptions, current_operation currentOperation);

      template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
      std::optional<Iter> process_option(Iter optionsIter, Sentinel optionsEnd, std::string_view arg, operations_forest& operations);

      template<std::input_iterator Iter, std::sentinel_for<Iter> Sentinel>
      bool process_concatenated_aliases(Iter optionsIter, Iter optionsBegin, Sentinel optionsEnd, std::string_view arg, operations_forest& operations);

      template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
      Iter process_nested_options(Iter optionsIter, Sentinel optionsEnd, operations_tree& currentOp);

      [[nodiscard]]
      bool top_level(const operations_forest& operations) const noexcept;

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

      parse(options.begin(), options.end(), {});
    }

    template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
    bool argument_parser::parse(Iter beginOptions, Sentinel endOptions, current_operation currentOperation)
    {
      current_option_tree currentOptionTree{};
      while(m_Index < m_ArgCount)
      {
        if(const std::string arg{m_Argv[m_Index++]}; !arg.empty())
        {
          if(!currentOperation.tree || currentOperation.current_op_complete)
          {
            const auto optionsIter{std::find_if(beginOptions, endOptions,
              [&arg](const auto& optTree) {
                const tree_adaptor tree{optTree, 0};

                return (tree.root_weight().name == arg) || is_alias(tree.root_weight(), arg);
              })
            };

            if(optionsIter == endOptions)
            {
              // TO DO: concatenated alias

              if(arg == "--help")
              {
                m_Help = generate_help(beginOptions, endOptions);
                return true;
              }

              throw std::runtime_error{error(std::string{"unrecognized option '"}.append(arg).append("'"))};
            }

            // TO DO: fix index of 0
            currentOptionTree = {*optionsIter, 0};

            if(!currentOptionTree.root_weight().early && !currentOptionTree.root_weight().late)
              throw std::logic_error{error("Commandline option not bound to a function object")};

            m_Operations.push_back({{currentOptionTree.root_weight().early, currentOptionTree.root_weight().late, {}}, forward_tree_type{}});

            // TO DO: fix index of 0
            currentOperation = {{m_Operations.back(), 0}};
            // etc
          }
          else
          {
            if(!currentOptionTree) throw std::logic_error{"Current option not found"};

            if(currentOperation.tree.root_weight().arguments.size() < currentOptionTree.root_weight().parameters.size())
            {
              currentOperation.tree.mutate_root_weight([&arg](auto& w){ w.arguments.push_back(arg); });

              if(currentOperation.tree.root_weight().arguments.size() == currentOptionTree.root_weight().parameters.size())
                currentOperation.current_op_complete = true;
            }
            else
            {
              --m_Index;
              const auto node{currentOptionTree.node()};
              // set intermediate construction complete
              //parse(currentOptionTree.tree().cbegin_edges(node), currentOptionTree.tree().cend_edges(node), currentOperation);
            }
          }
        }
      }


      /*auto optionsIter{options.end()};
      for(; m_Index < m_ArgCount; ++m_Index)
      {
        const std::string arg{m_Argv[m_Index]};
        if(optionsIter == options.end())
        {
          if(!arg.empty())
          {
            optionsIter = std::find_if(options.begin(), options.end(),
              [&arg](const auto& tree) { return (root(tree).name == arg) || is_alias(root(tree), arg); });

            if(optionsIter == options.end())
            {
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
          currentOperation.mutate_node_weight(currentOperation.cbegin_node_weights(), [&arg](auto& w){ w.arguments.push_back(arg); });

          const auto& arguments{currentOperation.cbegin_node_weights()->arguments};

          if(arguments.size() == optionsIter->cbegin_node_weights()->parameters.size())
          {
            optionsIter = process_nested_options(optionsIter, options.end(), currentOperation);
          }
        }
      }

      if(    !operations.empty()
         && (optionsIter != options.end())
         && (root(operations.back()).arguments.size() != root(optionsIter).parameters.size()))
      {
        const auto& params{root(optionsIter).parameters};
        const auto expected{params.size()};
        auto mess{std::string{"while parsing option \""}
                    .append(root(optionsIter).name)
                    .append("\": expected ")
                    .append(std::to_string(expected))
                    .append(pluralize(expected, "argument"))
                    .append(", [")};

        for(auto i{params.begin()}; i != params.end(); ++i)
        {
          mess.append(*i);
          if(std::distance(i, params.end()) > 1) mess.append(", ");
        }

        const auto actual{root(operations.back()).arguments.size()};
        mess.append("], but found ").append(std::to_string(actual)).append(pluralize(actual, "argument"));

        throw std::runtime_error{error(mess)};
      }*/

      return true;
    }

    template<std::input_iterator Iter, std::sentinel_for<Iter> Sentinel>
    bool argument_parser::process_concatenated_aliases(Iter optionsIter, Iter optionsBegin, Sentinel optionsEnd, std::string_view arg, operations_forest& operations)
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
              [&arg](const auto& tree) { return is_alias(root(tree), arg); });

            process_option(optionsIter, optionsEnd, arg, operations);
          }
        }
      }

      return optionsIter != optionsEnd;
    }

    template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
    std::optional<Iter> argument_parser::process_option(Iter optionsIter, Sentinel optionsEnd, std::string_view arg, operations_forest& operations)
    {
      if(optionsIter == optionsEnd)
      {
        if(top_level(operations))
          throw std::runtime_error{error(std::string{"unrecognized option '"}.append(arg).append("'"))};

        return {};
      }

      if(top_level(operations) && !root(optionsIter).early && !root(optionsIter).late)
        throw std::logic_error{error("Commandline option not bound to a function object")};

      operations.push_back({{root(optionsIter).early, root(optionsIter).late, {}}, forward_tree_type{}});
      if(root(optionsIter).parameters.empty())
      {
//        optionsIter = process_nested_options(optionsIter, optionsEnd, operations.back());
      }

      return optionsIter;
    }

    template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
    Iter argument_parser::process_nested_options(Iter optionsIter, Sentinel optionsEnd, operations_tree& currentOp)
    {
      /*if(!optionsIter->nested_options.empty())
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
      }*/

      return optionsEnd;
    }

    [[nodiscard]]
    bool argument_parser::top_level(const operations_forest& operations) const noexcept
    {
      return &m_Operations == &operations;
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
        const auto& optTree{*(beginOptions++)};

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
    using namespace maths;
    namespace cmd = parsing::commandline;
    using options = graph<directed_flavour::directed, null_weight, experimental::option>;

    options opts{{ {"create", {"c"}, {}, [](const cmd::arg_list&) {}} }, forward_tree_type{}};

    //options opt{{"create", {"c"}, {}, [](const cmd::arg_list&) {}}, symmetric_tree_type{}};

  }
}
