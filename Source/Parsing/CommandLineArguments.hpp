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

#include <vector>
#include <map>
#include <string>
#include <functional>

namespace sequoia::parsing::commandline
{
  using param_list = std::vector<std::string>;

  using executor = std::function<void (const param_list&)>;

  struct option
  {
    std::string name;
    param_list aliases;
    param_list parameters;
    executor fn{};

    std::vector<option> nested_options{};
  };

  struct operation
  {
    executor fn{};
    param_list parameters;

    std::vector<operation> nested_operations{};
  };
  
  [[nodiscard]]
  std::string error(std::string_view message, std::string_view prefix="\n  ");

  [[nodiscard]]
  std::string warning(std::string_view message, std::string_view prefix="\n  ");

  [[nodiscard]]
  std::string pluralize(const std::size_t n, std::string_view noun, std::string_view prefix=" ");

  [[nodiscard]]
  std::vector<operation> parse(int argc, char** argv, const std::vector<option>& options);

  class argument_parser
  {
  public:
    argument_parser(int argc, char** argv, const std::vector<option>& options);

    std::vector<operation> acquire()
    {
      return std::move(m_Operations);
    }
  private:
    std::vector<operation> m_Operations;
    int m_Index{1}, m_ArgCount{};
    char** m_Argv{};

    void parse(const std::vector<option>& options, std::vector<operation>& operations);

    template<class Iter>
    Iter process_option(Iter optionsIter, Iter optionsEnd, std::string_view arg, std::vector<operation>& operations);

    template<class Iter>
    Iter process_concatenated_aliases(Iter optionsIter, Iter optionsBegin, Iter optionsEnd, std::string_view arg, std::vector<operation>& operations);

    template<class Iter>
    Iter process_nested_options(Iter optionsIter, Iter optionsEnd, operation& currentOp);

    static bool is_alias(const option& opt, const std::string& s);
  };

  
  /*! \brief Class, principally to facilitate testing. */
  
  template<std::size_t... Ns>
  class commandline_arguments
  {
  public:
    constexpr static std::size_t size() noexcept
    {
      return sizeof...(Ns);
    }
    
    commandline_arguments(char const(&...args)[Ns])
      : m_Args{(char*)args...}
    {}

    [[nodiscard]]
    char** get() noexcept { return &m_Args[0]; }
  private:
    std::array<char*, sizeof...(Ns)> m_Args;
  };

  template<std::size_t... Ns>
  commandline_arguments(char const(&...args)[Ns]) -> commandline_arguments<Ns...>;
}
