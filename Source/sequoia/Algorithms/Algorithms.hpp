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

  template
  <
    std::random_access_iterator I,
    std::sentinel_for<I> S,
    class Comp = std::ranges::less,
    class Proj = std::identity
  >
    requires heap_sortable<I, Comp, Proj>
  constexpr void make_heap(I first, S last, Comp comp = {}, Proj proj = {})
  {
    if(std::ranges::distance(first, last) <= 1)
      return;

    auto current{first+1};
    while(current != last)
    {
      sequoia::bubble_up(first, current, comp, proj);
      std::ranges::advance(current,1);
    }
  }

  template<std::ranges::random_access_range R, class Comp = std::ranges::less, class Proj = std::identity>
    requires heap_sortable<std::ranges::iterator_t<R>, Comp, Proj>
  constexpr void make_heap(R&& r, Comp comp = {}, Proj proj = {})
  {
    sequoia::make_heap(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }

  /** \brief Sorts `[first, last)`, not stably, moving elements only through
             `std::ranges::iter_swap`.

      An `iter_swap` found by argument-dependent lookup therefore customizes
      how elements move.
   */
  template
  <
    std::random_access_iterator I,
    std::sentinel_for<I> S,
    class Comp = std::ranges::less,
    class Proj = std::identity
  >
    requires heap_sortable<I, Comp, Proj>
  constexpr void sort(I first, S last, Comp comp = {}, Proj proj = {})
  {
    auto end{std::ranges::next(first, last)};
    if(std::ranges::distance(first, end) <= 1)
      return;

    sequoia::make_heap(first, end, comp, proj);
    while(end != first)
    {
      std::ranges::iter_swap(first, end-1);
      sequoia::bubble_down(first, first, end-1, comp, proj);
      --end;
    }
  }

  template<std::ranges::random_access_range R, class Comp = std::ranges::less, class Proj = std::identity>
    requires heap_sortable<std::ranges::iterator_t<R>, Comp, Proj>
  constexpr void sort(R&& r, Comp comp = {}, Proj proj = {})
  {
    sequoia::sort(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
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
    std::forward_iterator I,
    std::sentinel_for<I> S,
    std::forward_iterator OutIter,
    class Comp = std::ranges::less,
    class Proj = std::identity
  >
    requires    merge_sortable<I, Comp, Proj>
             && std::mergeable<I, I, OutIter, Comp, Proj, Proj>
             && std::indirectly_copyable<OutIter, I>
  constexpr void stable_sort(I first, S last, OutIter out, Comp comp = {}, Proj proj = {})
  {
    const auto dist{std::ranges::distance(first, last)};
    if(dist < 2)
      return;

    const auto partition{std::ranges::next(first, dist / 2)};
    sequoia::stable_sort(first, partition, out, comp, proj);
    sequoia::stable_sort(partition, last, std::ranges::next(out, dist / 2), comp, proj);
    std::ranges::merge(first, partition, partition, last, out, comp, proj, proj);
    std::ranges::copy(out, std::ranges::next(out, dist), first);
  }

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
    sequoia::stable_sort(std::ranges::begin(r), std::ranges::end(r), out, std::move(comp), std::move(proj));
  }

  template
  <
    std::forward_iterator I,
    std::sentinel_for<I> S,
    class Comp = std::ranges::less,
    class Proj = std::identity
  >
    requires merge_sortable<I, Comp, Proj>
  constexpr void stable_sort(I first, S last, Comp comp = {}, Proj proj = {})
  {
    using T = std::iter_value_t<I>;
    auto v{
      [elements{std::ranges::subrange{first, last}}](){
        if      constexpr (is_initializable_v<T>)           return std::vector<T>(std::ranges::distance(elements));
        else if constexpr (std::is_copy_constructible_v<T>) return std::ranges::to<std::vector<T>>(elements);
      }()
    };

    sequoia::stable_sort(first, last, v.begin(), std::move(comp), std::move(proj));
  }

  template<std::ranges::forward_range R, class Comp = std::ranges::less, class Proj = std::identity>
    requires merge_sortable<std::ranges::iterator_t<R>, Comp, Proj>
  constexpr void stable_sort(R&& r, Comp comp = {}, Proj proj = {})
  {
    sequoia::stable_sort(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
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
  template
  <
    std::forward_iterator I,
    std::sentinel_for<I> S,
    class Comp = std::ranges::equal_to,
    class Proj = std::identity
  >
    requires clusterable<I, Comp, Proj>
  constexpr void cluster(I first, S last, Comp comp = {}, Proj proj = {})
  {
    if(first == last)
      return;

    auto current{first};
    while((current != last) && std::invoke(comp, std::invoke(proj, *current), std::invoke(proj, *first)))
    {
      std::ranges::advance(current, 1);
    }

    auto endOfCluster{current};

    while(current != last)
    {
      if(std::invoke(comp, std::invoke(proj, *current), std::invoke(proj, *first)))
      {
        std::ranges::iter_swap(endOfCluster, current);
        std::ranges::advance(endOfCluster, 1);
      }

      std::ranges::advance(current, 1);
    }

    sequoia::cluster(endOfCluster, last, comp, proj);
  }

  template<std::ranges::forward_range R, class Comp = std::ranges::equal_to, class Proj = std::identity>
    requires clusterable<std::ranges::iterator_t<R>, Comp, Proj>
  constexpr void cluster(R&& r, Comp comp = {}, Proj proj = {})
  {
    sequoia::cluster(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
}
