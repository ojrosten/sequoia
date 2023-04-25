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
  template<class Iter, class Comparer=std::ranges::less>
  constexpr void bubble_up(Iter begin, Iter current, Comparer comp = Comparer{})
  {
    if(current == begin) return;

    auto parent{begin + (std::ranges::distance(begin, current) - 1) / 2};
    if(comp(*parent, *current))
    {
      std::ranges::iter_swap(parent, current);
      bubble_up(begin, parent, comp);
    }
  }

  template<class Iter, class Comparer=std::ranges::less>
  constexpr void bubble_down(Iter begin, Iter current, Iter end, Comparer comp = Comparer{})
  {
    if(std::ranges::distance(begin, end) <= 1) return;

    if(2*(std::ranges::distance(begin, current) + 1) < std::ranges::distance(begin, end))
    {
      auto rightChild{begin + 2*(std::ranges::distance(begin, current) + 1)};
      auto leftChild{rightChild - 1};

      auto dominantChild{comp(*rightChild, *leftChild) ? leftChild : rightChild};
      if(comp(*current, *dominantChild))
      {
        std::ranges::iter_swap(dominantChild, current);
        bubble_down(begin, dominantChild, end, comp);
      }
    }
    else if(2* std::ranges::distance(begin, current) + 1 < std::ranges::distance(begin, end))
    {
      auto leftChild{begin + 2* std::ranges::distance(begin, current) + 1};
      if(comp(*current, *leftChild)) std::ranges::iter_swap(leftChild, current);
    }
  }

  template<class Iter, class Comparer=std::ranges::less>
  constexpr void make_heap(Iter begin, Iter end, Comparer comp = Comparer{})
  {
    if(std::ranges::distance(begin, end) <= 1) return;

    auto current{begin+1};
    while(current != end)
    {
      bubble_up(begin, current, comp);
      std::ranges::advance(current,1);
    }
  }

  /// This version of swap is retained, at least for now, since it
  /// is guaranteed to use iter_swap internally.
  /// This means that the behaviour can be customized by specializing
  /// the latter. Currently, this is exploited to sort graph nodes.
  /// This needs further thought, not least since ranges::advance tc
  /// cannot be used in the implementation
  template<class Iter, class Comparer=std::ranges::less>
  constexpr void sort(Iter begin, Iter end, Comparer comp = Comparer{})
  {
    if(std::ranges::distance(begin, end) <= 1) return;

    sequoia::make_heap(begin, end, comp);
    while(end != begin)
    {
      std::ranges::iter_swap(begin, end-1);
      bubble_down(begin, begin, end-1, comp);
      --end;
    }
  }

  template<class Iter>
  inline constexpr bool merge_sortable{
    requires {
      std::input_iterator<Iter>;
      std::is_copy_constructible_v<typename Iter::value_type>;
      std::is_copy_assignable_v<typename Iter::value_type>;
    }
  };

  template<std::input_iterator Iter, std::weakly_incrementable OutIter, class Compare=std::ranges::less>
    requires merge_sortable<Iter>
  constexpr void stable_sort(Iter first, Iter last, OutIter out, Compare compare= {})
  {
    const auto dist{std::ranges::distance(first, last)};
    if(dist < 2) return;

    const auto partition{std::ranges::next(first, dist / 2)};
    stable_sort(first, partition, out, compare);
    stable_sort(partition, last, std::ranges::next(out, dist / 2), compare);
    std::ranges::merge(first, partition, partition, last, out, compare);
    std::ranges::copy(out, std::ranges::next(out, dist), first);
  }

  template<class Iter, class Compare = std::ranges::less>
    requires merge_sortable<Iter>
  constexpr void stable_sort(Iter first, Iter last, Compare compare = {})
  {
    using T = typename Iter::value_type;
    auto v{
      [first, last](){
        if      constexpr (is_initializable_v<T>)           return std::vector<T>(std::ranges::distance(first, last));
        else if constexpr (std::is_copy_constructible_v<T>) return std::vector<T>(first, last);
      }()
    };

    stable_sort(first, last, v.begin(), compare);
  }

  /// \brief An algorithm which clusters together elements which compare equal.
  ///
  /// This is best used in situations where operator< is not defined.
  template<std::forward_iterator Iter, class Comparer=std::ranges::equal_to>
  constexpr void cluster(Iter begin, Iter end, Comparer comp = Comparer{})
  {
    if(begin == end) return;

    auto current{begin};
    while((current != end) && comp(*current, *begin)) std::ranges::advance(current, 1);

    auto endOfCluster{current};

    while(current != end)
    {
      if(comp(*current, *begin))
      {
        std::ranges::iter_swap(endOfCluster, current);
        std::ranges::advance(endOfCluster, 1);
      }

      std::ranges::advance(current, 1);
    }

    cluster(endOfCluster, end, comp);
  }
}
