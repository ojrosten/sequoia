////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief A collection of constexpr algorithms.

    These algorithms have been written as they have been required by other
    parts of the library; many will be retired once C++20 arrives.
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
      using std::iter_swap;
      iter_swap(parent, current);
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
        using std::iter_swap;
        iter_swap(dominantChild, current);
        bubble_down(begin, dominantChild, end, comp);
      }
    }
    else if(2*distance(begin, current) + 1 < distance(begin, end))
    {
      using std::iter_swap;
      auto leftChild{begin + 2*distance(begin, current) + 1};
      if(comp(*current, *leftChild)) iter_swap(leftChild, current);
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

  // One reason to retain this is that it uses iter_swap internally.
  // This means that the behaviour can be customized by specializing
  // the latter. Currently, this is exploited to sort graph nodes.
  template<class Iter, class Comparer=std::less<std::decay_t<decltype(*Iter())>>>
  constexpr void sort(Iter begin, Iter end, Comparer comp = Comparer{})
  {
    using std::distance;
    if(distance(begin, end) <= 1) return;

    sequoia::make_heap(begin, end, comp);
    while(end != begin)
    {
      using std::iter_swap;
      iter_swap(begin, end-1);
      bubble_down(begin, begin, end-1, comp);
      --end;
    }
  }

  /*template<class ForwardIt>
  constexpr ForwardIt rotate(ForwardIt first, ForwardIt n_first, ForwardIt last)
  {
    if(first == n_first) return last;
    if(last == n_first) return first;

    using namespace std;

    const auto distToEnd{distance(n_first, last)};
    const auto distFromBegin{distance(first, n_first)};
    const auto retIter{next(first, distToEnd)};

    const auto dist{min(distToEnd, distFromBegin)};

    const auto unswapped{next(first, dist)};

    while(first != unswapped)
    {
      iter_swap(first++, n_first++);
    }

    if(distToEnd > distFromBegin)
      sequoia::rotate(unswapped, next(unswapped, dist), last);
    else if(distToEnd < distFromBegin)
      sequoia::rotate(unswapped, last - dist, last);

    return retIter;
  }*/

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
        using std::iter_swap;
        iter_swap(endOfCluster, current);
        ++endOfCluster;
      }

      ++current;
    }

    cluster(endOfCluster, end, comp);
  }

  /*template<class Iter, class T, class Comparer = std::less<std::decay_t<decltype(*Iter())>>>
  constexpr Iter lower_bound(Iter begin, Iter end, const T& val, Comparer comp = Comparer{})
  {
    while(begin != end)
    {
      using namespace std;
      auto partition{begin + distance(begin, end)/2};
      if(comp(*partition, val))
      {
        begin = ++partition;
      }
      else
      {
        end = partition;
      }
    }

    return end;
  }

  template<class Iter, class T, class Comparer=std::less<std::decay_t<decltype(*Iter())>>>
  constexpr Iter upper_bound(Iter begin, Iter end, const T& val, Comparer comp = Comparer{})
  {
    auto notComp{
      [comp](const auto& lhs, const auto& rhs){
        return !comp(rhs, lhs);
      }
    };

    return sequoia::lower_bound(begin, end, val, notComp);
  }

  template<class Iter, class T, class Comparer=std::less<std::decay_t<decltype(*Iter())>>>
  constexpr std::pair<Iter, Iter> equal_range(Iter begin, Iter end, const T& val, Comparer comp = Comparer{})
  {
    return {sequoia::lower_bound(begin, end, val, comp), sequoia::upper_bound(begin, end, val, comp)};
  }

  template<class InputIt1, class InputIt2>
  constexpr bool equal(InputIt1 first1, InputIt2 last1, InputIt2 first2, InputIt2 last2)
  {
    using namespace std;
    if(distance(first1, last1) != distance(first2, last2)) return false;

    for(; first1 != last1; ++first1, ++first2)
    {
      if(*first1 != *first2) return false;
    }

    return true;
  }*/
}
