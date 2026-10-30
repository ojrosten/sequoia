////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Classes implementing the concept of a monotonic sequence.
 */

#include "sequoia/Maths/Sequences/MonotonicSequenceDetails.hpp"
#include "sequoia/Core/ContainerUtilities/ArrayUtilities.hpp"

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sequoia::maths
{
  /** \brief The operations common to a monotonic sequence in any container.

      `Compare{}(x, y)` is false for each element `x` and the element `y`
      that follows it. Every operation that changes the elements keeps this
      invariant, and refuses a change that would break it.
   */
  template<class T, class C, class Compare> class monotonic_sequence_base
  {
  public:
    using value_type      = T;
    using container_type  = C;
    using compare_type    = Compare;
    using size_type       = C::size_type;
    using difference_type = C::difference_type;

    using const_iterator         = C::const_iterator;
    using const_reverse_iterator = C::const_reverse_iterator;

    constexpr monotonic_sequence_base() = default;

    constexpr monotonic_sequence_base(std::initializer_list<T> list) : m_Sequence{make_container(list)}
    {
      check();
    }

    constexpr monotonic_sequence_base(C c) : m_Sequence{std::move(c)}
    {
      check();
    }

    constexpr monotonic_sequence_base(const monotonic_sequence_base&) = default;

    [[nodiscard]]
    constexpr size_type size() const noexcept { return m_Sequence.size(); }

    [[nodiscard]]
    constexpr bool empty() const noexcept { return m_Sequence.empty(); }

    [[nodiscard]]
    constexpr const_iterator begin() const noexcept { return m_Sequence.begin(); }

    [[nodiscard]]
    constexpr const_iterator end() const noexcept { return m_Sequence.end(); }

    [[nodiscard]]
    constexpr const_iterator cbegin() const noexcept { return m_Sequence.cbegin(); }

    [[nodiscard]]
    constexpr const_iterator cend() const noexcept { return m_Sequence.cend(); }

    [[nodiscard]]
    constexpr const_reverse_iterator rbegin() const noexcept { return m_Sequence.rbegin(); }

    [[nodiscard]]
    constexpr const_reverse_iterator rend() const noexcept { return m_Sequence.rend(); }

    [[nodiscard]]
    constexpr const_reverse_iterator crbegin() const noexcept { return m_Sequence.crbegin(); }

    [[nodiscard]]
    constexpr const_reverse_iterator crend() const noexcept { return m_Sequence.crend(); }

    [[nodiscard]]
    constexpr const T& operator[](const size_type i) const { return m_Sequence[i]; }

    [[nodiscard]]
    constexpr const T& back() const { return *(end() - 1); }

    [[nodiscard]]
    constexpr const T& front() const { return *begin(); }

    /** \brief Adds `delta` to each element in [`first`, `last`).

        \throws std::invalid_argument if `T` cannot represent an element plus
        `delta`, or if the translation would break the sequence's
        monotonicity. The sequence is then unchanged.
     */
    constexpr void translate(const_iterator first, const_iterator last, const difference_type delta)
      requires integer<T> && impl::numeric_order_v<Compare>
    {
      if(first == last)
        return;

      // The constraint on Compare puts every element of the run between the
      // run's ends, so the ends' range bounds every element's. With the ends
      // in range, translating preserves how Compare orders two elements of
      // the run, so only the ends need comparing with their neighbours.
      const auto back{std::ranges::prev(last)};
      if(!impl::sum_is_representable(*first, delta) || !impl::sum_is_representable(*back, delta))
        throw std::invalid_argument{"monotonic_sequence::translate - an element would leave the range of its type"};

      const auto translated{[delta](const T x){ return impl::wrapping_sum(x, delta); }};
      if(   ((first != cbegin()) && Compare{}(*std::ranges::prev(first), translated(*first)))
         || ((last  != cend())   && Compare{}(translated(*back), *last)))
      {
        throw std::invalid_argument{"monotonic_sequence::translate - monotonicity violated"};
      }

      const auto firstWritable{std::ranges::next(m_Sequence.begin(), std::ranges::distance(cbegin(), first))};
      std::ranges::transform(first, last, firstWritable, translated);
    }

    /** \brief Replaces each element with the corresponding one of `values`.

        \throws std::invalid_argument if `values` has a different number of
        elements, or would break the sequence's monotonicity. The sequence is
        then unchanged.
     */
    template<std::ranges::forward_range R>
      requires (   std::indirectly_copyable<std::ranges::iterator_t<R>, typename C::iterator>
                && std::indirect_binary_predicate<Compare, std::ranges::iterator_t<R>, std::ranges::iterator_t<R>>)
    constexpr void overwrite(R&& values)
    {
      if(std::cmp_not_equal(std::ranges::distance(values), size()))
        throw std::invalid_argument{"monotonic_sequence::overwrite - the number of values differs from the size"};

      if(!is_monotonic(values))
        throw std::invalid_argument{"monotonic_sequence::overwrite - monotonicity violated"};

      std::ranges::copy(values, m_Sequence.begin());
    }

    [[nodiscard]]
    friend constexpr bool operator==(const monotonic_sequence_base& lhs, const monotonic_sequence_base& rhs) = default;

  protected:
    template<alloc Allocator>
    constexpr explicit monotonic_sequence_base(const Allocator& allocator) noexcept
      : m_Sequence(allocator)
    {}

    template<alloc Allocator>
    constexpr monotonic_sequence_base(std::initializer_list<T> list, const Allocator& allocator)
      : m_Sequence(list, allocator)
    {
      check();
    }

    constexpr monotonic_sequence_base(monotonic_sequence_base&&) noexcept = default;

    template<alloc Allocator>
    constexpr monotonic_sequence_base(const monotonic_sequence_base& s, const Allocator& allocator)
      : m_Sequence(s.m_Sequence, allocator)
    {}

    template<alloc Allocator>
    constexpr monotonic_sequence_base(monotonic_sequence_base&& s, const Allocator& allocator)
      : m_Sequence(std::move(s.m_Sequence), allocator)
    {}

    constexpr ~monotonic_sequence_base() = default;

    constexpr monotonic_sequence_base& operator=(const monotonic_sequence_base&) = default;
    constexpr monotonic_sequence_base& operator=(monotonic_sequence_base&&)      = default;

    constexpr void swap(monotonic_sequence_base& other) noexcept(impl::swap_is_noexcept_v<C>)
    {
      std::ranges::swap(m_Sequence, other.m_Sequence);
    }

    constexpr auto get_allocator() const
    {
      return m_Sequence.get_allocator();
    }

    constexpr void push_back(T v)
    {
      if(!m_Sequence.empty() && Compare{}(m_Sequence.back(), v))
        throw std::invalid_argument{"monotonic_sequence::push_back - monotonicity violated"};

      m_Sequence.push_back(std::move(v));
    }

    constexpr const_iterator insert(const_iterator pos, T v)
    {
      if(((pos != cend()) && Compare{}(v, *pos)) || ((pos != cbegin()) && Compare{}(*(pos-1), v)))
      {
        throw std::invalid_argument{"monotonic_sequence::insert - monotonicity violated"};
      }

      return m_Sequence.insert(pos, std::move(v));
    }

    constexpr const_iterator erase(const_iterator pos)
    {
      return m_Sequence.erase(pos);
    }

    constexpr const_iterator erase(const_iterator first, const_iterator last)
    {
      return m_Sequence.erase(first, last);
    }

    constexpr void reserve(const size_type new_cap)
    {
      m_Sequence.reserve(new_cap);
    }

    constexpr size_type capacity() const noexcept
    {
      return m_Sequence.capacity();
    }

    constexpr void shrink_to_fit()
    {
      m_Sequence.shrink_to_fit();
    }

    constexpr void clear() noexcept
    {
      m_Sequence.clear();
    }

  private:
    C m_Sequence;

    [[nodiscard]]
    constexpr static C make_container(std::initializer_list<T> list)
    {
      if constexpr(impl::static_storage<C>::value)
        return utilities::to_array<T, impl::static_storage<C>::size()>(list);
      else
        return C{list};
    }

    template<std::ranges::forward_range R>
    [[nodiscard]]
    constexpr static bool is_monotonic(R&& r)
    {
      return std::ranges::adjacent_find(r, Compare{}) == std::ranges::end(r);
    }

    constexpr void check() const
    {
      if(!is_monotonic(m_Sequence))
        throw std::invalid_argument{"monotonic_sequence::monotonic_sequence - monotonicity violated"};
    }
  };


  template<class T, class Compare=std::ranges::less, class C=std::vector<T>>
  class monotonic_sequence : public monotonic_sequence_base<T, C, Compare>
  {
    using base_type = monotonic_sequence_base<T, C, Compare>;
  public:
    static_assert(has_allocator_type_v<C>);

    using allocator_type = C::allocator_type;

    constexpr monotonic_sequence() = default;

    constexpr explicit monotonic_sequence(const allocator_type& allocator)
      : monotonic_sequence_base<T, C, Compare>(allocator)
    {}

    constexpr monotonic_sequence(std::initializer_list<T> list, const allocator_type& allocator=allocator_type{})
      : monotonic_sequence_base<T, C, Compare>{list, allocator}
    {}

    constexpr monotonic_sequence(const monotonic_sequence&) = default;

    constexpr monotonic_sequence(const monotonic_sequence& s, const allocator_type& allocator)
      : monotonic_sequence_base<T, C, Compare>{s, allocator}
    {}

    constexpr monotonic_sequence(monotonic_sequence&&) noexcept = default;

    constexpr monotonic_sequence(monotonic_sequence&& s, const allocator_type& allocator)
      : monotonic_sequence_base<T, C, Compare>{std::move(s), allocator}
    {}

    constexpr ~monotonic_sequence() = default;

    constexpr monotonic_sequence& operator=(const monotonic_sequence&) = default;
    constexpr monotonic_sequence& operator=(monotonic_sequence&&)      = default;

    friend constexpr void swap(monotonic_sequence& lhs, monotonic_sequence& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    using base_type::get_allocator;
    using base_type::push_back;
    using base_type::insert;
    using base_type::erase;
    using base_type::reserve;
    using base_type::capacity;
    using base_type::shrink_to_fit;
    using base_type::clear;
  };

  template<class T, std::size_t N, class Compare=std::ranges::less>
  class static_monotonic_sequence : public monotonic_sequence_base<T, std::array<T, N>, Compare>
  {
  public:
    using monotonic_sequence_base<T, std::array<T, N>, Compare>::monotonic_sequence_base;
  };
}
