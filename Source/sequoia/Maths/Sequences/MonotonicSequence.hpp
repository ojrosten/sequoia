////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Classes implementing the concept of a monotonic sequence.
 */

#include "sequoia/Maths/Sequences/MonotonicSequenceDetails.hpp"
#include "sequoia/Core/Utilities/ArrayUtilities.hpp"
#include "sequoia/Algorithms/Algorithms.hpp"

#include <vector>

namespace sequoia::maths
{
  struct unsafe_t {};

  template<class T, class C, class Compare> class monotonic_sequence_base
  {
  public:
    using value_type     = T;
    using container_type = C;
    using compare_type   = Compare;
    using size_type      = typename C::size_type;

    using const_iterator         = typename C::const_iterator;
    using const_reverse_iterator = typename C::const_reverse_iterator;

    constexpr monotonic_sequence_base() = default;

    constexpr monotonic_sequence_base(std::initializer_list<T> list) : monotonic_sequence_base{static_type{}, list}
    {}

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

    template<class UnaryOp>
    constexpr void mutate(const_iterator first, const_iterator last, UnaryOp op)
    {
      using std::distance;
      if(first != last)
      {
        auto tmp{m_Sequence};
        auto f{tmp.begin() + distance(cbegin(), first)};
        auto l{tmp.begin() + distance(cbegin(), last)};

        for(auto i{f}; i != l; ++i)
        {
          *i = op(*i);
        }

        if(f != tmp.begin()) --f;
        if(l != tmp.end()) ++l;

        while((f != l) && (f + 1) != tmp.end())
        {
          if(Compare{}(*f, *(f+1)))
            throw std::logic_error("monotonic_sequence::mutate - monotonicity violated");

          ++f;
        }

        m_Sequence = std::move(tmp);
      }
    }

    template<class UnaryOp>
    constexpr void mutate(unsafe_t, const_iterator first, const_iterator last, UnaryOp op)
    {
      using std::distance;
      while(first != last)
      {
        auto pos{m_Sequence.begin() + distance(cbegin(), first++)};
        *pos = op(*pos);
      }
    }

    [[nodiscard]]
    friend bool operator==(const monotonic_sequence_base& lhs, const monotonic_sequence_base& rhs) noexcept
      = default;

    [[nodiscard]]
    friend bool operator!=(const monotonic_sequence_base& lhs, const monotonic_sequence_base& rhs) noexcept
      = default;

  protected:
    template<alloc Allocator>
    constexpr explicit monotonic_sequence_base(const Allocator& allocator) noexcept
      : m_Sequence(allocator)
    {}

    template<alloc Allocator>
    constexpr monotonic_sequence_base(std::initializer_list<T> list, const Allocator& allocator)
      : m_Sequence{list, allocator}
    {
      check();
    }

    constexpr monotonic_sequence_base(monotonic_sequence_base&&) noexcept = default;

    template<alloc Allocator>
    constexpr monotonic_sequence_base(const monotonic_sequence_base& s, const Allocator& allocator)
      : m_Sequence{s.m_Sequence, allocator}
    {}

    template<alloc Allocator>
    constexpr monotonic_sequence_base(monotonic_sequence_base&& s, const Allocator& allocator)
      : m_Sequence{std::move(s.m_Sequence), allocator}
    {}

    ~monotonic_sequence_base() = default;

    constexpr monotonic_sequence_base& operator=(const monotonic_sequence_base&) = default;
    constexpr monotonic_sequence_base& operator=(monotonic_sequence_base&&)      = default;

    constexpr void swap(monotonic_sequence_base& other) noexcept(impl::noexcept_spec_v<C>)
    {
      using std::swap;
      swap(m_Sequence, other.m_Sequence);
    }

    auto get_allocator() const
    {
      return m_Sequence.get_allocator();
    }

    void push_back(T v)
    {
      if(!m_Sequence.empty() && Compare{}(m_Sequence.back(), v))
        throw std::logic_error{"monotonic_sequence_base::push_back - monotonicity violated"};

      m_Sequence.push_back(std::move(v));
    }

    const_iterator insert(const_iterator pos, T v)
    {
      if(((pos != cend()) && Compare{}(v, *pos)) || ((pos != cbegin()) && Compare{}(*(pos-1), v)))
      {
        throw std::logic_error{"monotonic_sequence_base::insert - monotonicity violated"};
      }

      return m_Sequence.insert(pos, std::move(v));
    }

    const_iterator erase(const_iterator pos)
    {
      return m_Sequence.erase(pos);
    }

    const_iterator erase(const_iterator first, const_iterator last)
    {
      return m_Sequence.erase(first, last);
    }

    void reserve(const size_type new_cap)
    {
      m_Sequence.reserve(new_cap);
    }

    size_type capacity() const noexcept
    {
      return m_Sequence.capacity();
    }

    void shrink_to_fit()
    {
      m_Sequence.shrink_to_fit();
    }

    void clear() noexcept
    {
      m_Sequence.clear();
    }

  private:
    using static_type = impl::static_storage<C>;

    C m_Sequence;

    constexpr monotonic_sequence_base(std::false_type, std::initializer_list<T> list) : m_Sequence{list}
    {
      check();
    }

    constexpr monotonic_sequence_base(std::true_type, std::initializer_list<T> list)
      : m_Sequence{utilities::to_array<T, static_type::size()>(list)}
    {
      check();
    }

    constexpr void check()
    {
      if(size() > 1)
      {
        for(auto i{begin()+1}; i != end(); ++i)
        {
          if(Compare{}(*(i-1), *i))
            throw std::logic_error("monotonic_sequence_base::monotonic_sequence_base - monotonicity violated");
        }
      }
    }
  };


  template<class T, class Compare=std::less<T>, class C=std::vector<T>>
  class monotonic_sequence : public monotonic_sequence_base<T, C, Compare>
  {
    using base_t = monotonic_sequence_base<T, C, Compare>;
  public:
    static_assert(has_allocator_type_v<C>);

    using allocator_type = typename C::allocator_type;

    monotonic_sequence() = default;

    explicit monotonic_sequence(const allocator_type& allocator)
      : monotonic_sequence_base<T, C, Compare>(allocator)
    {}

    monotonic_sequence(std::initializer_list<T> list, const allocator_type& allocator=allocator_type{})
      : monotonic_sequence_base<T, C, Compare>{list, allocator}
    {}

    monotonic_sequence(const monotonic_sequence&) = default;

    monotonic_sequence(const monotonic_sequence& s, const allocator_type& allocator)
      : monotonic_sequence_base<T, C, Compare>{s, allocator}
    {}

    monotonic_sequence(monotonic_sequence&&) noexcept = default;

    monotonic_sequence(monotonic_sequence&& s, const allocator_type& allocator)
      : monotonic_sequence_base<T, C, Compare>{std::move(s), allocator}
    {}

    ~monotonic_sequence() = default;

    monotonic_sequence& operator=(const monotonic_sequence&) = default;
    monotonic_sequence& operator=(monotonic_sequence&&)      = default;

    friend void swap(monotonic_sequence& lhs, monotonic_sequence& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    allocator_type get_allocator() const
    {
      return base_t::get_allocator();
    }

    using base_t::push_back;
    using base_t::insert;
    using base_t::erase;
    using base_t::reserve;
    using base_t::capacity;
    using base_t::shrink_to_fit;
    using base_t::clear;
  };

  template<class T, std::size_t N, class Compare=std::less<T>>
  class static_monotonic_sequence : public monotonic_sequence_base<T, std::array<T, N>, Compare>
  {
  public:
    using monotonic_sequence_base<T, std::array<T, N>, Compare>::monotonic_sequence_base;
  };
}
