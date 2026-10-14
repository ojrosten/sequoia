////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief A collection of constexpr algorithms.
*/

#include "sequoia/Core/Meta/Concepts.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>
#include <vector>

namespace sequoia
{
  template<class Iter, class Comp=std::ranges::less, class Proj = std::identity>
  constexpr void bubble_up(Iter begin, Iter current, Comp comp = {}, Proj proj = {})
  {
    if(current == begin) return;

    auto parent{begin + (std::ranges::distance(begin, current) - 1) / 2};
    if(std::invoke(comp, std::invoke(proj, *parent), std::invoke(proj, *current)))
    {
      std::ranges::iter_swap(parent, current);
      sequoia::bubble_up(begin, parent, comp, proj);
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

      const bool leftDominates{std::invoke(comp, std::invoke(proj, *rightChild), std::invoke(proj, *leftChild))};
      auto dominantChild{leftDominates ? leftChild : rightChild};
      if(std::invoke(comp, std::invoke(proj, *current), std::invoke(proj, *dominantChild)))
      {
        std::ranges::iter_swap(dominantChild, current);
        sequoia::bubble_down(begin, dominantChild, end, comp, proj);
      }
    }
    else if(2* std::ranges::distance(begin, current) + 1 < std::ranges::distance(begin, end))
    {
      auto leftChild{begin + 2* std::ranges::distance(begin, current) + 1};
      if(std::invoke(comp, std::invoke(proj, *current), std::invoke(proj, *leftChild)))
        std::ranges::iter_swap(leftChild, current);
    }
  }

  /** \brief Whether `make_heap` and `sort` can order the elements `Iter`
             reaches, by `Comp` applied to `Proj`.

      Unlike `std::sortable`, `heap_sortable` does not require
      `std::indirectly_movable_storable`: these algorithms move elements only
      by swapping them.
   */
  template<class Iter, class Comp, class Proj>
  inline constexpr bool heap_sortable{
       std::indirectly_swappable<Iter>
    && std::indirect_strict_weak_order<Comp, std::projected<Iter, Proj>>
  };

  template<std::ranges::random_access_range R, class Comp = std::ranges::less, class Proj = std::identity>
    requires heap_sortable<std::ranges::iterator_t<R>, Comp, Proj>
  constexpr void make_heap(R&& r, Comp comp = {}, Proj proj = {})
  {
    if(std::ranges::distance(r) <= 1)
      return;

    const auto begin{std::ranges::begin(r)};
    const auto end{std::ranges::end(r)};
    auto current{begin+1};
    while(current != end)
    {
      sequoia::bubble_up(begin, current, comp, proj);
      std::ranges::advance(current,1);
    }
  }

  /** \brief Sorts `r`, not stably, moving elements only through
             `std::ranges::iter_swap`.

      An `iter_swap` found by argument-dependent lookup therefore customizes
      how elements move.
   */
  template<std::ranges::random_access_range R, class Comp = std::ranges::less, class Proj = std::identity>
    requires heap_sortable<std::ranges::iterator_t<R>, Comp, Proj>
  constexpr void sort(R&& r, Comp comp = {}, Proj proj = {})
  {
    if(std::ranges::distance(r) <= 1)
      return;

    const auto begin{std::ranges::begin(r)};
    auto end{std::ranges::next(begin, std::ranges::end(r))};
    sequoia::make_heap(r, comp, proj);
    while(end != begin)
    {
      std::ranges::iter_swap(begin, end-1);
      sequoia::bubble_down(begin, begin, end-1, comp, proj);
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
     && std::indirectly_copyable<typename std::vector<typename std::iterator_traits<Iter>::value_type>::iterator, Iter>
  };

  template
  <
    std::ranges::forward_range R,
    std::forward_iterator OutIter,
    class Comp = std::ranges::less,
    class Proj = std::identity
  >
    requires    merge_sortable<std::ranges::iterator_t<R>, Comp, Proj>
             && std::mergeable<std::ranges::iterator_t<R>, std::ranges::iterator_t<R>, OutIter, Comp, Proj, Proj>
             && std::indirectly_copyable<OutIter, std::ranges::iterator_t<R>>
  constexpr void stable_sort(R&& r, OutIter out, Comp comp = {}, Proj proj = {})
  {
    const auto dist{std::ranges::distance(r)};
    if(dist < 2)
      return;

    const auto first{std::ranges::begin(r)};
    const auto last{std::ranges::end(r)};
    const auto partition{std::ranges::next(first, dist / 2)};
    sequoia::stable_sort(std::ranges::subrange{first, partition}, out, comp, proj);
    sequoia::stable_sort(std::ranges::subrange{partition, last}, std::ranges::next(out, dist / 2), comp, proj);
    std::ranges::merge(first, partition, partition, last, out, comp, proj, proj);
    std::ranges::copy(out, std::ranges::next(out, dist), first);
  }

  template<std::ranges::forward_range R, class Comp = std::ranges::less, class Proj = std::identity>
    requires merge_sortable<std::ranges::iterator_t<R>, Comp, Proj>
  constexpr void stable_sort(R&& r, Comp comp = {}, Proj proj = {})
  {
    using T = std::ranges::range_value_t<R>;
    auto v{
      [&r](){
        if      constexpr (is_initializable_v<T>)           return std::vector<T>(std::ranges::distance(r));
        else if constexpr (std::is_copy_constructible_v<T>) return std::ranges::to<std::vector<T>>(r);
      }()
    };

    sequoia::stable_sort(r, v.begin(), std::move(comp), std::move(proj));
  }

  template<class Iter, class Comp, class Proj>
  inline constexpr bool clusterable{
       std::forward_iterator<Iter>
    && std::indirectly_swappable<Iter>
    && std::indirect_equivalence_relation<Comp, std::projected<Iter, Proj>>
  };

  /** \brief An algorithm which clusters together elements which compare equal.

      This is best used in situations where operator< is not defined.
   */
  template<std::ranges::forward_range R, class Comp = std::ranges::equal_to, class Proj = std::identity>
    requires clusterable<std::ranges::iterator_t<R>, Comp, Proj>
  constexpr void cluster(R&& r, Comp comp = {}, Proj proj = {})
  {
    const auto begin{std::ranges::begin(r)};
    const auto end{std::ranges::end(r)};
    if(begin == end)
      return;

    auto current{begin};
    while((current != end) && std::invoke(comp, std::invoke(proj, *current), std::invoke(proj, *begin)))
    {
      std::ranges::advance(current, 1);
    }

    auto endOfCluster{current};

    while(current != end)
    {
      if(std::invoke(comp, std::invoke(proj, *current), std::invoke(proj, *begin)))
      {
        std::ranges::iter_swap(endOfCluster, current);
        std::ranges::advance(endOfCluster, 1);
      }

      std::ranges::advance(current, 1);
    }

    sequoia::cluster(std::ranges::subrange{endOfCluster, end}, comp, proj);
  }
}
