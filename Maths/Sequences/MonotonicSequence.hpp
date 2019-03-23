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

#include <stdexcept>

namespace sequoia::maths
{
  template<class T, class C, class Compare=std::less<T>> class monotonic_sequence_base
  {
  public:
    using value_type = T;
    using size_type  = typename C::size_type;
    
    using const_iterator         = typename C::const_iterator;
    using const_reverse_iterator = typename C::const_iterator;

    monotonic_sequence_base() = default;
    
    monotonic_sequence_base(std::initializer_list<T> list) : m_Sequence{list}
    {
      if(list.size() > 1)
      {
        for(auto i{list.begin()+1}; i != list.end(); ++i)
        {
          if(Compare{}(*(i-1), *i))
            throw std::logic_error("monotonic_sequence_base::monotonic_sequence_base - monotonicity violated");
        }
      }
    }

    monotonic_sequence_base(const monotonic_sequence_base&) = default;
      
    [[nodiscard]]
    constexpr size_type size() const noexcept { return m_Sequence.size(); }
      
    const_iterator begin() const noexcept { return m_Sequence.begin(); }
    const_iterator end() const noexcept { return m_Sequence.end(); }
      
    const_iterator cbegin() const noexcept { return m_Sequence.cbegin(); }
    const_iterator cend() const noexcept { return m_Sequence.cend(); }
          
    const_reverse_iterator rbegin() const noexcept { return m_Sequence.rbegin(); }
    const_reverse_iterator rend() const noexcept { return m_Sequence.rend(); }
      
    const_reverse_iterator crbegin() const noexcept { return m_Sequence.crbegin(); }
    const_reverse_iterator crend() const noexcept { return m_Sequence.crend(); }

    template<class UnaryOp>
    void mutate(const_iterator first, const_iterator last, UnaryOp op)
    {
      while(first != last)
      {
        using std::distance;
        iterator pos{begin() + distance(cbegin(), first++)};
        const auto nascent{op(*pos)};

        if(   ((pos   != begin()) && Compare{}(*(pos - 1), nascent))
           || ((pos+1 != end())   && Compare{}(nascent, *(pos + 1))))
        {
          throw std::logic_error{"monotonic_sequence_base::mutate - monotonicity violated"};
        }

        *pos = nascent;
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
    using iterator         = typename C::iterator;    
    using reverse_iterator = typename C::iterator;
    
    ~monotonic_sequence_base() = default;

    monotonic_sequence_base(monotonic_sequence_base&&) noexcept = default;
    
    monotonic_sequence_base& operator=(const monotonic_sequence_base&)     = default;    
    monotonic_sequence_base& operator=(monotonic_sequence_base&&) noexcept = default;
    
    iterator begin() noexcept { return m_Sequence.begin(); }
    iterator end() noexcept { return m_Sequence.end(); }

    reverse_iterator rbegin() noexcept { return m_Sequence.rbegin(); }
    reverse_iterator rend() noexcept { return m_Sequence.rend(); }
    
    void push_back(T v)
    {
      if(!m_Sequence.empty() && Compare{}(m_Sequence.back(), v))
        throw std::logic_error{"monotonic_sequence_base::push_back - monotonicity violated"};
        
      m_Sequence.push_back(std::move(v));
    }

    const_iterator insert(const_iterator pos, T v)
    {
      if(((pos != cend()) && Compare{}(v, *pos)) || ((pos != cbegin()) && Compare(*(pos-1), v)))
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
    C m_Sequence;    
  };
}
