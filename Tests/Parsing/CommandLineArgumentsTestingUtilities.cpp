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
  namespace
  {
    [[nodiscard]]
    std::vector<std::string> convert(std::initializer_list<std::string_view> args)
    {
      std::vector<std::string> a{};

      std::transform(args.begin(), args.end(), std::back_inserter(a), [](auto view){ return std::string{view}; });

      return a;
    }
  }

  commandline_arguments::commandline_arguments(std::initializer_list<std::string_view> args)
    : commandline_arguments{convert(args)}
  {}

  commandline_arguments::commandline_arguments(std::vector<std::string> args)
    : m_Args{std::move(args)}
  {
    if(m_Args.empty())
      throw std::runtime_error{"No commandline arguments - not even empty ones!"};

    m_Ptrs.reserve(m_Args.size());
    std::transform(m_Args.begin(), m_Args.end(), std::back_inserter(m_Ptrs), [](std::string& s){ return s.data(); });
  }
}