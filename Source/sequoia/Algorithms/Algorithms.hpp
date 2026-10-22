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

#include "sequoia/Core/ContainerUtilities/ArithmeticCasts.hpp"
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

  /** \brief Whether swapping can order the elements `I` reaches, comparing
             their projections under `Proj` with `Comp`.

      Unlike `std::sortable`, `heap_sortable` requires no iterator category,
      and does not require `std::indirectly_movable_storable`.
   */
  template<class I, class Comp, class Proj>
  inline constexpr bool heap_sortable{
       std::indirectly_swappable<I>
    && std::indirect_strict_weak_order<Comp, std::projected<I, Proj>>
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
    auto unsortedEnd{std::ranges::next(first, last)};
    if(std::ranges::distance(first, unsortedEnd) <= 1)
      return;

    sequoia::make_heap(first, unsortedEnd, comp, proj);
    while(unsortedEnd != first)
    {
      std::ranges::iter_swap(first, unsortedEnd-1);
      sequoia::bubble_down(first, first, unsortedEnd-1, comp, proj);
      --unsortedEnd;
    }
  }

  /** \brief Sorts `r`, as the overload taking a sentinel does. */
  template<std::ranges::random_access_range R, class Comp = std::ranges::less, class Proj = std::identity>
    requires heap_sortable<std::ranges::iterator_t<R>, Comp, Proj>
  constexpr void sort(R&& r, Comp comp = {}, Proj proj = {})
  {
    sequoia::sort(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }

  namespace impl
  {
    template<class I>
    using merge_buffer_iterator_t = std::vector<typename std::iterator_traits<I>::value_type>::iterator;
  }

  template<class I, class Comp, class Proj>
  inline constexpr bool merge_sortable{
     requires{
         typename std::iterator_traits<I>::value_type;
         requires (   std::is_copy_constructible_v<typename std::iterator_traits<I>::value_type>
                   || is_initializable_v<typename std::iterator_traits<I>::value_type>);
       }
     && std::mergeable<I, I, impl::merge_buffer_iterator_t<I>, Comp, Proj, Proj>
     && std::indirectly_copyable<impl::merge_buffer_iterator_t<I>, I>
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
    using T = std::iterator_traits<I>::value_type;
    auto v{
      [elements{std::ranges::subrange{first, last}}](){
        if constexpr (is_initializable_v<T>)
          return std::vector<T>(utilities::unchecked_conversion_to<std::size_t>(std::ranges::distance(elements)));
        else if constexpr (std::is_copy_constructible_v<T>)
          return std::ranges::to<std::vector<T>>(elements);
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

  template<class I, class Comp, class Proj>
  inline constexpr bool clusterable{
       std::forward_iterator<I>
    && std::indirectly_swappable<I>
    && std::indirect_equivalence_relation<Comp, std::projected<I, Proj>>
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

  /** \brief Clusters `r`, as the overload taking a sentinel does. */
  template<std::ranges::forward_range R, class Comp = std::ranges::equal_to, class Proj = std::identity>
    requires clusterable<std::ranges::iterator_t<R>, Comp, Proj>
  constexpr void cluster(R&& r, Comp comp = {}, Proj proj = {})
  {
    sequoia::cluster(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
}
