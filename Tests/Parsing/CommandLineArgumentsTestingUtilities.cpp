////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "CommandLineArgumentsTestingUtilities.hpp"

namespace sequoia::testing
{
  commandline_arguments::commandline_arguments(std::initializer_list<std::string_view> args)
    : commandline_arguments{std::views::transform(args, [](auto sv){ return std::string{sv}; }) | std::ranges::to<std::vector>()}
  {}

  commandline_arguments::commandline_arguments(std::vector<std::string> args)
    : m_Args{std::move(args)}
  {
    if(m_Args.empty())
      throw std::runtime_error{"No commandline arguments - not even empty ones!"};

    m_Ptrs.reserve(m_Args.size());
    std::ranges::transform(m_Args, std::back_inserter(m_Ptrs), [](std::string& s){ return s.data(); });
  }
}
