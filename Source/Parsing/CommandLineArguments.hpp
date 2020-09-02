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

  struct option_info
  {
    std::string name;
    param_list aliases;
    param_list parameters;
    executor fn{};

    std::vector<option_info> nested_options{};
  };

  struct operation
  {
    executor fn{};
    param_list parameters;
  };
  
  [[nodiscard]]
  std::string error(std::string_view message, std::string_view prefix="\n  ");

  [[nodiscard]]
  std::string warning(std::string_view message, std::string_view prefix="\n  ");

  [[nodiscard]]
  std::string pluralize(const std::size_t n, std::string_view noun, std::string_view prefix=" ");

  [[nodiscard]]
  std::vector<operation>
  parse(int argc, char** argv, const std::vector<option_info>& info);

  
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
