#pragma once

#include "TestGraphHelper.hpp"

namespace sequoia::unit_testing
{
  struct BFS {};
  struct DFS {};
  struct PRS {};

  template<class SearchType=BFS> struct Traverser
  {
    using type = BFS;

    template<class G, class... Fn>
    static void traverse(const G& g, const bool findDisconnected, const std::size_t start, Fn&&... fn)
    {
      maths::breadth_first_search(g, findDisconnected, start, std::forward<Fn>(fn)...);
    }

    constexpr static bool uses_forward_iterator() { return true; }

    static std::string iterator_description() { return "forward"; }
  };

  template<> struct Traverser<DFS>
  {
    using type = DFS;

    template<class G, class... Fn>
    static void traverse(const G& g, const bool findDisconnected, const std::size_t start, Fn&&... fn)
    {
      maths::depth_first_search(g, findDisconnected, start, std::forward<Fn>(fn)...);
    }

    constexpr static bool uses_forward_iterator() { return false; }

    static std::string iterator_description() { return "reverse"; }
  };

  template<> struct Traverser<PRS>
  {
    using type = PRS;

    template<class G, class... Fn>
    static void traverse(const G& g, const bool findDisconnected, const std::size_t start, Fn&&... fn)
    {
      maths::priority_search(g, findDisconnected, start, std::forward<Fn>(fn)...);
    }

    constexpr static bool uses_forward_iterator() { return true; }

    static std::string iterator_description() { return "forward"; }
  };

  template<> struct type_to_string<Traverser<BFS>>
  {
    static std::string str() { return "BFS"; }
  };

  template<> struct type_to_string<Traverser<DFS>>
  {
    static std::string str() { return "DFS"; }
  };

  template<> struct type_to_string<Traverser<PRS>>
  {
    static std::string str() { return "PRS"; }
  };
}
