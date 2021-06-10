////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "GraphTestingUtilities.hpp"
#include "sequoia/Maths/Graph/DynamicGraphTraversals.hpp"

namespace sequoia::testing
{
  struct BFS {};
  struct DFS {};
  struct PRS {};

  template<class SearchType=BFS> struct Traverser
  {
    using type = BFS;

    template<class G, maths::disconnected_discovery_mode Mode, class... Fn>
    static void traverse(const G& g, const maths::traversal_conditions<Mode> conditions, Fn&&... fn)
    {
      maths::breadth_first_search(g, conditions, std::forward<Fn>(fn)...);
    }

    constexpr static bool uses_forward_iterator() noexcept { return true; }

    static std::string iterator_description() noexcept { return "forward"; }
  };

  template<> struct Traverser<DFS>
  {
    using type = DFS;

    template<class G, maths::disconnected_discovery_mode Mode, class... Fn>
    static void traverse(const G& g, const maths::traversal_conditions<Mode> conditions, Fn&&... fn)
    {
      maths::pseudo_depth_first_search(g, conditions, std::forward<Fn>(fn)...);
    }

    constexpr static bool uses_forward_iterator() noexcept { return false; }

    static std::string iterator_description() noexcept { return "reverse"; }
  };

  template<> struct Traverser<PRS>
  {
    using type = PRS;

    template<class G, maths::disconnected_discovery_mode Mode, class... Fn>
    static void traverse(const G& g, const maths::traversal_conditions<Mode> conditions, Fn&&... fn)
    {
      maths::priority_search(g, conditions, std::forward<Fn>(fn)...);
    }

    constexpr static bool uses_forward_iterator() noexcept { return true; }

    static std::string iterator_description() noexcept { return "forward"; }
  };
}
