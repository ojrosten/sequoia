////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file MonotonicSequence.hpp
    \brief Classes implementing the concept of a monotonic sequence.
 */

#include "ArrayUtilities.hpp"

#include <stdexcept>

namespace sequoia::maths
{
  namespace impl
  {
    template<class C> struct static_storage : std::false_type {};

    template<class T, std::size_t N> struct static_storage<std::array<T, N>> : std::true_type
    {
      constexpr static std::size_t size() { return N; }
    };
  }
  
  template<class T, class C, class Compare=std::less<T>> class monotonic_sequence_base
  {
  public:
    using value_type = T;
    using size_type  = typename C::size_type;
    
    using const_iterator         = typename C::const_iterator;
    using const_reverse_iterator = typename C::const_reverse_iterator;

    constexpr monotonic_sequence_base() = default;
    
    constexpr monotonic_sequence_base(std::initializer_list<T> list) : monotonic_sequence_base{static_type{}, list}
    {}

    constexpr monotonic_sequence_base(const monotonic_sequence_base&) = default;
      
    [[nodiscard]]
    constexpr size_type size() const noexcept { return m_Sequence.size(); }

    [[nodiscard]]
    constexpr bool empty() const noexcept { return m_Sequence.empty(); }
      
    constexpr const_iterator begin() const noexcept { return m_Sequence.begin(); }
    constexpr const_iterator end() const noexcept { return m_Sequence.end(); }
      
    constexpr const_iterator cbegin() const noexcept { return m_Sequence.cbegin(); }
    constexpr const_iterator cend() const noexcept { return m_Sequence.cend(); }
          
    constexpr const_reverse_iterator rbegin() const noexcept { return m_Sequence.rbegin(); }
    constexpr const_reverse_iterator rend() const noexcept { return m_Sequence.rend(); }
      
    constexpr const_reverse_iterator crbegin() const noexcept { return m_Sequence.crbegin(); }
    constexpr const_reverse_iterator crend() const noexcept { return m_Sequence.crend(); }

    constexpr const T& operator[](const size_type i) const { return m_Sequence[i]; }

    template<class UnaryOp, bool Checked=true>
    constexpr void mutate(const_iterator first, const_iterator last, UnaryOp op)
    {
      using std::distance;
      if constexpr(Checked)
      {
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
      else
      {
        while(first != last)
        {
          auto pos{m_Sequence.begin() + distance(cbegin(), first++)};                    
          *pos = op(*pos);
        }
      }      
    }

    constexpr friend bool operator==(const monotonic_sequence_base& lhs, const monotonic_sequence_base& rhs)
    {
      return lhs.m_Sequence == rhs.m_Sequence;
    }

    constexpr friend bool operator!=(const monotonic_sequence_base& lhs, const monotonic_sequence_base& rhs)
    {
      return !(lhs == rhs);
    }
    
  protected:    
    constexpr monotonic_sequence_base(monotonic_sequence_base&&) noexcept = default;
    
    ~monotonic_sequence_base() = default;
    
    constexpr monotonic_sequence_base& operator=(const monotonic_sequence_base&)     = default;    
    constexpr monotonic_sequence_base& operator=(monotonic_sequence_base&&) noexcept = default;

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


  template<class T, class C=std::vector<T>, class Compare=std::less<T>>
  class monotonic_sequence : public monotonic_sequence_base<T, C, Compare>
  {    
    using base_t = monotonic_sequence_base<T, C, Compare>;
  public:
    using monotonic_sequence_base<T, C, Compare>::monotonic_sequence_base;

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
