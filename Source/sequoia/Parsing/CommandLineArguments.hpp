////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file CommandLineArguments.hpp
    \brief Parsing of commandline arguments
*/

#include "sequoia/Core/Meta/Concepts.hpp"

#include "sequoia/Maths/Graph/DynamicTree.hpp"
#include "sequoia/Maths/Graph/GraphTraversalFunctions.hpp"

#include <vector>
#include <string>
#include <functional>
#include <stdexcept>

namespace sequoia::parsing::commandline
{
  class proper_string
  {
  public:
    proper_string(std::string s) : m_String{std::move(s)}
    {
      if(m_String.empty())
        throw std::logic_error{"Empty string detected"};
    }

    proper_string(const char* c) : proper_string{std::string{c}}
    {}

    [[nodiscard]]
    operator const std::string& () const noexcept
    {
      return m_String;
    }

    [[nodiscard]]
    operator std::string_view() const noexcept
    {
      return m_String;
    }

    [[nodiscard]]
    friend bool operator==(const proper_string&, const proper_string&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const proper_string&, const proper_string&) noexcept = default;
  private:
    std::string m_String;
  };

  using param_list = std::vector<proper_string>;
  using arg_list   = std::vector<std::string>;

  using executor = std::function<void (const arg_list&)>;

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

  using options_tree   = maths::tree<maths::directed_flavour::directed, maths::tree_link_direction::forward, maths::null_weight, option>;
  using options_forest = std::vector<options_tree>;
  using option_tree    = maths::const_tree_adaptor<options_tree>;
  
  using operations_tree     = maths::tree<maths::directed_flavour::directed, maths::tree_link_direction::forward, maths::null_weight, operation>;
  using operations_sub_tree = maths::tree_adaptor<operations_tree>;
  using operations_forest   = std::vector<operations_tree>;

  struct outcome
  {
    std::string zeroth_arg;
    operations_forest operations;
    std::string help{};
  };

  [[nodiscard]]
  std::string error(std::string_view message, std::string_view prefix="\n  ");

  [[nodiscard]]
  std::string warning(std::string_view message, std::string_view prefix="\n  ");

  [[nodiscard]]
  std::string pluralize(std::size_t n, std::string_view noun, std::string_view prefix=" ");

  [[nodiscard]]
  outcome parse(int argc, char** argv, const options_forest& options);


  template<std::invocable<std::string> Fn>
  [[nodiscard]]
  std::string parse_invoke_depth_first(int argc, char** argv, const options_forest options, Fn zerothArgProcessor)
  {
    auto[zerothArg, operations, help]{parse(argc, argv, options)};

    if(help.empty())
    {
      zerothArgProcessor(zerothArg);

      using namespace maths;
      for(const auto& operTree : operations)
      {
        depth_first_search(operTree,
                           ignore_disconnected_t{},
                           [&operTree](const auto node) {
                              const operation& oper{operTree.cbegin_node_weights()[node]};
                              if(oper.early) oper.early(oper.arguments);
                           },
                           [&operTree](const auto node) {
                              const operation& oper{operTree.cbegin_node_weights()[node]};
                              if(oper.late) oper.late(oper.arguments);
                           });
      }
    }

    return help;
  }


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
    enum class top_level { yes, no };

    struct operation_data
    {
      operations_sub_tree oper_tree{};
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

    auto process_option(option_tree currentOptionTree, operation_data currentOperationData, top_level topLevel)->operation_data;

    template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
    [[nodiscard]]
    static std::string generate_help(Iter beginOptions, Sentinel endOptions);

    static bool is_alias(const option& opt, std::string_view s);
  };
}
