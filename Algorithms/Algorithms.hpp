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

#include "Concepts.hpp"

#include <functional>

namespace sequoia
{
  template<class T> constexpr void swap(T& a, T& b) noexcept(noexcept(std::swap(a,b)))
  {
    if constexpr (has_allocator_type<T>)
    {
      using std::swap;
      swap(a,b);
    }
    else
    {
      auto tmp{std::move(a)};
      a = std::move(b);
      b = std::move(tmp);
    }
  }
  
  template<class Iter> constexpr void iter_swap(Iter a, Iter b)
  {
    sequoia::swap(*a, *b);
  }
  
  template<class FwdIter, class Comparer=std::less<std::decay_t<decltype(*FwdIter())>>>
  constexpr void bubble_up(FwdIter begin, FwdIter current, Comparer comp = Comparer{})
  {
    if(current == begin) return;

    using namespace std;
    auto parent{begin + (distance(begin, current) - 1) / 2};
    if(comp(*parent, *current))
    {
      iter_swap(parent, current);
      bubble_up(begin, parent, comp);
    }
  }
  
  template<class FwdIter, class Comparer=std::less<std::decay_t<decltype(*FwdIter())>>>
  constexpr void bubble_down(FwdIter begin, FwdIter current, FwdIter end, Comparer comp = Comparer{})
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
        iter_swap(dominantChild, current);
        bubble_down(begin, dominantChild, end, comp);
      }
    }
    else if(2*distance(begin, current) + 1 < distance(begin, end))
    {
      auto leftChild{begin + 2*distance(begin, current) + 1};
      if(comp(*current, *leftChild)) iter_swap(leftChild, current);
    }
  }

  template<class FwdIter, class Comparer=std::less<std::decay_t<decltype(*FwdIter())>>>
  constexpr void make_heap(FwdIter begin, FwdIter end, Comparer comp = Comparer{})
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

  template<class FwdIter, class Comparer=std::less<std::decay_t<decltype(*FwdIter())>>>
  constexpr void sort(FwdIter begin, FwdIter end, Comparer comp = Comparer{})
  {
    using namespace std;
    if(distance(begin, end) <= 1) return;
    
    sequoia::make_heap(begin, end, comp);
    while(end != begin)
    {
      iter_swap(begin, end-1);
      bubble_down(begin, begin, end-1, comp);
      --end;
    }
  }

  template<class ForwardIt>
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
  }

  template<class FwdIter, class Comparer=std::equal_to<std::decay_t<decltype(*FwdIter())>>>
  constexpr void cluster(FwdIter begin, FwdIter end, Comparer comp = Comparer{})
  {
    if(begin == end) return;
    
    auto current{begin};
    while((current != end) && comp(*current, *begin)) ++current;

    auto endOfCluster{current};
    
    while(current != end)
    {
      if(comp(*current, *begin))
      {
        iter_swap(endOfCluster, current);
        ++endOfCluster;
      }
      
      ++current;
    }

    cluster(endOfCluster, end, comp);
  }

  template<class FwdIter, class T, class Comparer=std::less<std::decay_t<decltype(*FwdIter())>>>
  constexpr FwdIter lower_bound(FwdIter begin, FwdIter end, const T& val, Comparer comp = Comparer{})
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

  template<class FwdIter, class T, class Comparer=std::less<std::decay_t<decltype(*FwdIter())>>>
  constexpr FwdIter upper_bound(FwdIter begin, FwdIter end, const T& val, Comparer comp = Comparer{})
  {
    auto notComp{
      [comp](const auto& lhs, const auto& rhs){
        return !comp(rhs, lhs);
      }
    };

    return sequoia::lower_bound(begin, end, val, notComp);
  }

  template<class FwdIter, class T, class Comparer=std::less<std::decay_t<decltype(*FwdIter())>>>
  constexpr std::pair<FwdIter, FwdIter> equal_range(FwdIter begin, FwdIter end, const T& val, Comparer comp = Comparer{})
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
  }
}
