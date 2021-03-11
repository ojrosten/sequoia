////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file StaticQueue.hpp
    \brief A constexpr queue.

 */

#include "ArrayUtilities.hpp"
#include "Algorithms.hpp"

namespace sequoia::data_structures
{
  /*! \class static_queue
      \brief A queue suitable for constexpr contexts.

   */
  
  template<class T, std::size_t MaxDepth>
  class static_queue
  {
  public:
    constexpr static_queue(std::initializer_list<T> l)
      : m_Queue{utilities::to_array<T, MaxDepth>(l)}
      , m_Front{l.size() ? 0 : MaxDepth}
      , m_Back{l.size() ? l.size() - 1 : MaxDepth}
    {
    }

    constexpr static_queue(const static_queue&)    = default;
    constexpr static_queue(static_queue&) noexcept = default;
    ~static_queue() = default;

    constexpr static_queue& operator=(const static_queue&)    = default;
    constexpr static_queue& operator=(static_queue&) noexcept = default;
    
    constexpr void push(const T& val)
    {
      if constexpr(MaxDepth > 0)
      {   
        if(size() == MaxDepth)
        {
          throw std::logic_error("Attempting to exceed maximum queue depth");
        }
        else if(m_Front == MaxDepth)
        {
          m_Front = 0;
          m_Back  = 0;
        }
        else 
        {
          m_Back = (m_Back + 1) % MaxDepth;
        }

        m_Queue[m_Back] = val;
      }
      else
      {
        throw std::logic_error("Attempting to exceed maximum queue depth");
      }
    }

    [[nodiscard]]
    constexpr const T& back() const noexcept
    {
      return m_Queue[m_Back];
    }

    [[nodiscard]]
    constexpr const T& front() const noexcept
    {
      return m_Queue[m_Front];
    }

    constexpr void pop() noexcept
    {
      if(m_Front == m_Back)
      {
        m_Front = MaxDepth;
        m_Back = MaxDepth;
      }
      else
      {
        m_Front = (m_Front + 1) % MaxDepth;
      }
    }

    [[nodiscard]]
    constexpr bool empty() const noexcept
    {
      return m_Front == MaxDepth;
    }

    [[nodiscard]]
    constexpr std::size_t size() const noexcept
    {
      if(empty())
      {
        return {};
      }
      else if(m_Front <= m_Back)
      {
        return m_Back + 1 - m_Front;
      }
      else
      {
        return MaxDepth + 1 - (m_Front - m_Back);
      }
    }

    [[nodiscard]]
    friend constexpr bool operator==(const static_queue& lhs, const static_queue& rhs) noexcept
    {
      if constexpr(MaxDepth > 0)
      {   
        const auto sz{lhs.size()};
        if(sz != rhs.size()) return false;

        for(std::size_t i{}, l{lhs.m_Front}, r{rhs.m_Front}; i<sz; ++i)
        {

          if(lhs.m_Queue[l] != rhs.m_Queue[r]) return false;
        
          l = (l+1) % MaxDepth;
          r = (r+1) % MaxDepth;
        }

        return true;
      }
      else
      {
        return true;
      }
    }

    [[nodiscard]]
    friend constexpr bool operator!=(const static_queue& lhs, const static_queue& rhs) noexcept
    {
      return !(lhs == rhs);
    }
  private:
    std::array<T, MaxDepth> m_Queue{};

    std::size_t m_Front{MaxDepth}, m_Back{MaxDepth};
  };
}
