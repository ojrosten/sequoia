////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief A collection of constexpr algorithms.
*/

#include "sequoia/Core/Meta/Concepts.hpp"

#include <algorithm>
#include <functional>
#include <iterator>

namespace sequoia
{
  template<class Iter, class Comparer=std::less<std::decay_t<decltype(*Iter())>>>
  constexpr void bubble_up(Iter begin, Iter current, Comparer comp = Comparer{})
  {
    if(current == begin) return;

    using namespace std;
    auto parent{begin + (distance(begin, current) - 1) / 2};
    if(comp(*parent, *current))
    {
      std::ranges::iter_swap(parent, current);
      bubble_up(begin, parent, comp);
    }
  }

  template<class Iter, class Comparer=std::less<std::decay_t<decltype(*Iter())>>>
  constexpr void bubble_down(Iter begin, Iter current, Iter end, Comparer comp = Comparer{})
  {
    using namespace std;
    if(distance(begin, end) <= 1) return;

    if(2*(distance(begin, current) + 1) < distance(begin, end))
    {
      auto rightChild{begin + 2*(distance(begin, current) + 1)};
      auto leftChild{rightChild - 1};

      auto dominantChild{comp(*rightChild, *leftChild) ? leftChild : rightChild};
      if(comp(*current, *dominantChild))
      {
        std::ranges::iter_swap(dominantChild, current);
        bubble_down(begin, dominantChild, end, comp);
      }
    }
    else if(2*distance(begin, current) + 1 < distance(begin, end))
    {
      auto leftChild{begin + 2*distance(begin, current) + 1};
      if(comp(*current, *leftChild)) std::ranges::iter_swap(leftChild, current);
    }
  }

  template<class Iter, class Comparer=std::less<std::decay_t<decltype(*Iter())>>>
  constexpr void make_heap(Iter begin, Iter end, Comparer comp = Comparer{})
  {
    using namespace std;
    if(distance(begin, end) <= 1) return;

    auto current{begin+1};
    while(current != end)
    {
      bubble_up(begin, current, comp);
      ++current;
    }
  }

  /// This version of swap is retained, at least for now, since it
  /// is guaranteed to use iter_swap internally.
  /// This means that the behaviour can be customized by specializing
  /// the latter. Currently, this is exploited to sort graph nodes.
  template<class Iter, class Comparer=std::less<std::decay_t<decltype(*Iter())>>>
  constexpr void sort(Iter begin, Iter end, Comparer comp = Comparer{})
  {
    using namespace std;
    if(distance(begin, end) <= 1) return;

    sequoia::make_heap(begin, end, comp);
    while(end != begin)
    {
      std::ranges::iter_swap(begin, end-1);
      bubble_down(begin, begin, end-1, comp);
      --end;
    }
  }

  /// \brief An algorithm which clusters together elements which compare equal.
  ///
  /// This is best used in situations where operator< is not defined.
  template<std::forward_iterator Iter, class Comparer=std::equal_to<std::decay_t<decltype(*Iter())>>>
  constexpr void cluster(Iter begin, Iter end, Comparer comp = Comparer{})
  {
    if(begin == end) return;

    auto current{begin};
    while((current != end) && comp(*current, *begin)) ++current;

    auto endOfCluster{current};

    while(current != end)
    {
      if(comp(*current, *begin))
      {
        std::ranges::iter_swap(endOfCluster, current);
        ++endOfCluster;
      }

      ++current;
    }

    cluster(endOfCluster, end, comp);
  }
}
