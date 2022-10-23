////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "GraphTraversalTestingUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string to_string(const maths::traversal_flavour f)
  {
    using enum maths::traversal_flavour;
    switch(f)
    {
    case breadth_first:
      return "Breadth-first search";
    case depth_first:
      return "Depth-first search";
    case pseudo_depth_first:
      return "Pseudo Depth-first search";
    case priority:
      return "Priority search";
    }

    throw std::logic_error{"Unrecognized traversal flavour"};
  }
}
