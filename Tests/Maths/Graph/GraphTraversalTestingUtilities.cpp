////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "GraphTraversalTestingUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string to_string(const traversal_flavour f)
  {
    switch(f)
    {
    case traversal_flavour::BFS:
      return "Breadth-first search";
    case traversal_flavour::DFS:
      return "Depth-first search";
    case traversal_flavour::PDFS:
      return "Pseudo Depth-first search";
    case traversal_flavour::PRS:
      return "Priority search";
    }

    throw std::logic_error{"Unrecognized traversal flavour"};
  }
}
