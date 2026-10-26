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
#include "sequoia/Core/Numeric/IntegerConversions.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <utility>
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
          return std::vector<T>(numeric::unchecked_conversion_to<std::size_t>(std::ranges::distance(elements)));
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

  namespace impl
  {
    [[nodiscard]]
    constexpr const char* not_a_permutation_message() noexcept
    {
      return "sequoia::permute: the destinations are not a permutation of the range's positions";
    }
  }

  /** \brief Moves the element at each position `i` of `r` to position
             `destinations[i]`.

      \throws std::invalid_argument if `destinations` is not a permutation of
      `r`'s positions. `r` then holds its elements in an unspecified order.
   */
  template<std::ranges::random_access_range R, integer Index>
    requires (std::ranges::sized_range<R> && std::indirectly_swappable<std::ranges::iterator_t<R>>)
  constexpr void permute(R&& r, std::vector<Index> destinations)
  {
    const auto size{std::ranges::size(r)};
    if(std::cmp_not_equal(destinations.size(), size))
      throw std::invalid_argument{impl::not_a_permutation_message()};

    auto iteratorAt{
      [first{std::ranges::begin(r)}](const std::size_t position) {
        return first + numeric::unchecked_conversion_to<std::ranges::range_difference_t<R>>(position);
      }
    };

    auto isPosition{
      [size](const Index destination) {
        return std::cmp_greater_equal(destination, 0) && std::cmp_less(destination, size);
      }
    };

    // Each swap leaves the entry at the element's destination equal to that
    // destination, so the loop ends. Finding such an entry before the swap
    // means that two elements share the destination. Either that, or a
    // destination outside the range, shows that the destinations are not a
    // permutation.
    for(const auto position : std::views::iota(0uz, destinations.size()))
    {
      while(std::cmp_not_equal(destinations[position], position))
      {
        if(!isPosition(destinations[position]))
          throw std::invalid_argument{impl::not_a_permutation_message()};

        const auto destination{numeric::unchecked_conversion_to<std::size_t>(destinations[position])};
        if(std::cmp_equal(destinations[destination], destination))
          throw std::invalid_argument{impl::not_a_permutation_message()};

        std::ranges::iter_swap(iteratorAt(position), iteratorAt(destination));
        std::ranges::swap(destinations[position], destinations[destination]);
      }
    }
  }
}
