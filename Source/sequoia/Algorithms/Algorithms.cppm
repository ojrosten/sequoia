////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.algorithms;

import std;

export import sequoia.core.meta;
export import sequoia.core.numeric;

/** \file
    \brief A collection of constexpr algorithms.
*/

export namespace sequoia
{
  template<std::random_access_iterator Iter, class Comp=std::ranges::less, class Proj = std::identity>
    requires std::sortable<Iter, Comp, Proj>
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

  template<std::random_access_iterator Iter, class Comp=std::ranges::less, class Proj = std::identity>
    requires std::sortable<Iter, Comp, Proj>
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

  template
  <
    std::random_access_iterator I,
    std::sentinel_for<I> S,
    class Comp = std::ranges::less,
    class Proj = std::identity
  >
    requires std::sortable<I, Comp, Proj>
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
    requires std::sortable<std::ranges::iterator_t<R>, Comp, Proj>
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

  /** \brief Moves the element at each position `i` of `r` to the position
             held by the `i`th element of `destinations`.

      If `destinations` is a permutation, swaps elements of `r` n - c times,
      where n is the number of positions and c the number of cycles of the
      permutation, fixed points included.

      Moves `destinations` if it is a non-const `std::vector` rvalue.
      Otherwise copies it into a `std::vector` in a single pass, allocating
      unless it is empty, and does not write to it.

      \throws std::invalid_argument if `destinations` is not a permutation of
      `r`'s positions. `r` then holds its elements in an unspecified order.
   */
  template<std::ranges::random_access_range R, range_of_integers D>
    requires (   std::ranges::sized_range<R>
              && std::ranges::sized_range<D>
              && std::indirectly_swappable<std::ranges::iterator_t<R>>
              && integer<std::ranges::range_size_t<R>>
              && integer<std::ranges::range_difference_t<R>>)
  constexpr void permute(R&& r, D&& destinations)
  {
    const auto size{numeric::unchecked_conversion_to<std::size_t>(std::ranges::size(r))};
    if(std::cmp_not_equal(std::ranges::size(destinations), size))
      throw std::invalid_argument{impl::not_a_permutation_message()};

    auto elementDestinations{std::ranges::to<std::vector>(std::forward<D>(destinations))};

    auto iteratorAtPosition{
      [first{std::ranges::begin(r)}](const std::size_t position) {
        return first + numeric::unchecked_conversion_to<std::ranges::range_difference_t<R>>(position);
      }
    };

    auto destinationFor{
      [&elementDestinations, size](const std::size_t position) {
        const auto destination{elementDestinations[position]};
        if(std::cmp_less(destination, 0) || std::cmp_greater_equal(destination, size))
          throw std::invalid_argument{impl::not_a_permutation_message()};

        return numeric::unchecked_conversion_to<std::size_t>(destination);
      }
    };

    // Swapping `r` and `elementDestinations` together keeps
    // `elementDestinations[p]` the destination of the element at position p of
    // `r`. Each step of the inner loop moves the element at `position` to
    // `destination`, putting it in place. No step moves an element already in
    // place: the element at `position` is not, since `destination` differs
    // from `position`, and one in place at `destination` would share its
    // destination with the element at `position`, so the check before the step
    // throws instead. Each step therefore puts at least one more element in
    // place, and the loops, although nested, take at most `size` steps in all;
    // for a permutation, a cycle of k elements takes k - 1, the last of which
    // puts two elements in place. Since swapping only rearranges
    // `elementDestinations`, a value held twice, or one outside [0, size),
    // shows that the destinations passed are not a permutation. If neither is
    // found, every element ends in place.
    for(const auto position : std::views::iota(0uz, size))
    {
      for(auto destination{destinationFor(position)}; destination != position; destination = destinationFor(position))
      {
        if(destinationFor(destination) == destination)
          throw std::invalid_argument{impl::not_a_permutation_message()};

        std::ranges::iter_swap(iteratorAtPosition(position), iteratorAtPosition(destination));
        std::ranges::swap(elementDestinations[position], elementDestinations[destination]);
      }
    }
  }
}
