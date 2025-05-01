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
  template<class Iter, class Comp=std::ranges::less, class Proj = std::identity>
  constexpr void bubble_up(Iter begin, Iter current, Comp comp = {}, Proj proj = {})
  {
    if(current == begin) return;

    auto parent{begin + (std::ranges::distance(begin, current) - 1) / 2};
    if(comp(proj(*parent), proj(*current)))
    {
      std::ranges::iter_swap(parent, current);
      bubble_up(begin, parent, comp, proj);
    }
  }

  template<class Iter, class Comp=std::ranges::less, class Proj = std::identity>
  constexpr void bubble_down(Iter begin, Iter current, Iter end, Comp comp = {}, Proj proj = {})
  {
    if(std::ranges::distance(begin, end) <= 1) return;

    if(2*(std::ranges::distance(begin, current) + 1) < std::ranges::distance(begin, end))
    {
      auto rightChild{begin + 2*(std::ranges::distance(begin, current) + 1)};
      auto leftChild{rightChild - 1};

      auto dominantChild{comp(proj(*rightChild), proj(*leftChild)) ? leftChild : rightChild};
      if(comp(proj(*current), proj(*dominantChild)))
      {
        std::ranges::iter_swap(dominantChild, current);
        bubble_down(begin, dominantChild, end, comp, proj);
      }
    }
    else if(2* std::ranges::distance(begin, current) + 1 < std::ranges::distance(begin, end))
    {
      auto leftChild{begin + 2* std::ranges::distance(begin, current) + 1};
      if(comp(proj(*current), proj(*leftChild))) std::ranges::iter_swap(leftChild, current);
    }
  }

  template<class Iter, class Comp=std::ranges::less, class Proj=std::identity>
  constexpr void make_heap(Iter begin, Iter end, Comp comp = {}, Proj proj = {})
  {
    if(std::ranges::distance(begin, end) <= 1) return;

    auto current{begin+1};
    while(current != end)
    {
      bubble_up(begin, current, comp, proj);
      std::ranges::advance(current,1);
    }
  }

  /// This version of swap is retained, at least for now, since it
  /// is guaranteed to use ranges::iter_swap internally.
  /// This means that the behaviour can be customized by overloading
  /// iter_swap. Currently, this is exploited to sort graph nodes.
  /// This needs further thought, not least since ranges::advance etc
  /// cannot be used in the implementation
  template<class Iter, class Comp=std::ranges::less, class Proj = std::identity>
  constexpr void sort(Iter begin, Iter end, Comp comp = {}, Proj proj = {})
  {
    if(std::ranges::distance(begin, end) <= 1) return;

    sequoia::make_heap(begin, end, comp, proj);
    while(end != begin)
    {
      std::ranges::iter_swap(begin, end-1);
      bubble_down(begin, begin, end-1, comp, proj);
      --end;
    }
  }

  template<class Iter, class Comp, class Proj>
  inline constexpr bool merge_sortable{
     requires{
         typename std::iterator_traits<Iter>::value_type;
         requires (std::is_copy_constructible_v<typename std::iterator_traits<Iter>::value_type> || is_initializable_v<typename std::iterator_traits<Iter>::value_type>);
       }
     && std::mergeable<Iter, Iter, typename std::vector<typename std::iterator_traits<Iter>::value_type>::iterator, Comp, Proj, Proj>
  };

  template<std::input_iterator Iter, std::weakly_incrementable OutIter, class Comp = std::ranges::less, class Proj = std::identity>
    requires merge_sortable<Iter, Comp, Proj>
  constexpr void stable_sort(Iter first, Iter last, OutIter out, Comp compare = {}, Proj proj = {})
  {
    const auto dist{std::ranges::distance(first, last)};
    if(dist < 2) return;

    const auto partition{std::ranges::next(first, dist / 2)};
    stable_sort(first, partition, out, compare, proj);
    stable_sort(partition, last, std::ranges::next(out, dist / 2), compare, proj);
    std::ranges::merge(first, partition, partition, last, out, compare, proj, proj);
    std::ranges::copy(out, std::ranges::next(out, dist), first);
  }

  template<std::input_iterator Iter, class Comp = std::ranges::less, class Proj = std::identity>
    requires merge_sortable<Iter, Comp, Proj>
  constexpr void stable_sort(Iter first, Iter last, Comp compare = {}, Proj proj = {})
  {
    using T = typename std::iterator_traits<Iter>::value_type;
    auto v{
      [first, last](){
        if      constexpr (is_initializable_v<T>)           return std::vector<T>(std::ranges::distance(first, last));
        else if constexpr (std::is_copy_constructible_v<T>) return std::vector<T>(first, last);
      }()
    };

    stable_sort(first, last, v.begin(), std::move(compare), std::move(proj));
  }

  template<class Iter, class Comp, class Proj>
  inline constexpr bool clusterable{
       std::forward_iterator<Iter>
    && requires(std::iter_reference_t<Iter> r, Comp comp, Proj proj){
        { comp(proj(r), proj(r)) } -> std::convertible_to<bool>;
       }
  };

  /// \brief An algorithm which clusters together elements which compare equal.
  ///
  /// This is best used in situations where operator< is not defined.
  template<std::forward_iterator Iter, class Comp=std::ranges::equal_to, class Proj = std::identity>
    requires clusterable<Iter, Comp, Proj>
  constexpr void cluster(Iter begin, Iter end, Comp comp = {}, Proj proj = {})
  {
    if(begin == end) return;

    auto current{begin};
    while((current != end) && comp(proj(*current), proj(*begin))) std::ranges::advance(current, 1);

    auto endOfCluster{current};

    while(current != end)
    {
      if(comp(proj(*current), proj(*begin)))
      {
        std::ranges::iter_swap(endOfCluster, current);
        std::ranges::advance(endOfCluster, 1);
      }

      std::ranges::advance(current, 1);
    }

    cluster(endOfCluster, end, comp, proj);
  }
}
