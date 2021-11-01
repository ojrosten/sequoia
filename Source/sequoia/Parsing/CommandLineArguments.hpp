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

#include <vector>
#include <map>
#include <string>
#include <functional>
#include <optional>
#include <filesystem>
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
    param_list aliases;
    param_list parameters;
    executor early{};

    std::vector<option> nested_options{};

    executor late{};
  };

  struct operation
  {
    executor early, late;
    arg_list arguments;

    std::vector<operation> nested_operations{};
  };

  struct outcome
  {
    std::string zeroth_arg;
    std::vector<operation> operations;
    std::string help{};
  };

  [[nodiscard]]
  std::string error(std::string_view message, std::string_view prefix="\n  ");

  [[nodiscard]]
  std::string warning(std::string_view message, std::string_view prefix="\n  ");

  [[nodiscard]]
  std::string pluralize(std::size_t n, std::string_view noun, std::string_view prefix=" ");

  [[nodiscard]]
  outcome parse(int argc, char** argv, const std::vector<option>& options);

  void invoke_depth_first(const std::vector<operation>& operations);

  template<std::invocable<std::string> Fn>
  [[nodiscard]]
  std::string parse_invoke_depth_first(int argc, char** argv, const std::vector<option>& options, Fn zerothArgProcessor)
  {
    auto[zerothArg, ops, help]{parse(argc, argv, options)};

    if(help.empty())
    {
      zerothArgProcessor(zerothArg);
      invoke_depth_first(ops);
    }

    return help;
  }

  class argument_parser
  {
  public:
    argument_parser(int argc, char** argv, const std::vector<option>& options);

    outcome get() const
    {
      return {m_ZerothArg, m_Operations, m_Help};
    }
  private:
    std::vector<operation> m_Operations{};
    int m_Index{1}, m_ArgCount{};
    char** m_Argv{};
    std::string m_ZerothArg{}, m_Help{};

    bool parse(const std::vector<option>& options, std::vector<operation>& operations);

    template<std::input_or_output_iterator Iter>
    std::optional<Iter> process_option(Iter optionsIter, Iter optionsEnd, std::string_view arg, std::vector<operation>& operations);

    template<std::input_iterator Iter>
    bool process_concatenated_aliases(Iter optionsIter, Iter optionsBegin, Iter optionsEnd, std::string_view arg, std::vector<operation>& operations);

    template<std::input_or_output_iterator Iter>
    Iter process_nested_options(Iter optionsIter, Iter optionsEnd, operation& currentOp);

    [[nodiscard]]
    bool top_level(const std::vector<operation>& operations) const noexcept;

    [[nodiscard]]
    static std::string generate_help(const std::vector<option>& options);

    static bool is_alias(const option& opt, const std::string& s);
  };
}
