////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CommandLineArgumentsTestingUtilities.hpp"

namespace sequoia::testing
{
  namespace
  {
    [[nodiscard]]
    std::vector<char*> data_pointers(std::vector<std::string>& args)
    {
      auto dataOf{[](std::string& s) { return s.data(); }};

      return args | std::views::transform(dataOf) | std::ranges::to<std::vector>();
    }
  }

  commandline_arguments::commandline_arguments(std::vector<std::string> args)
    : m_Args{std::move(args)}
    , m_Ptrs{data_pointers(m_Args)}
  {
    if(m_Args.empty())
      throw std::runtime_error{"No commandline arguments - not even empty ones!"};
  }
}
