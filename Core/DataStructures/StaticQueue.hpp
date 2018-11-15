#pragma once

#include <array>
#include <utility>

namespace sequoia::data_structures
{
  template<class T, std::size_t MaxDepth>
  class static_queue
  {
  public:
    constexpr static_queue(std::initializer_list<T> l)
      : m_Queue{make_array(l, std::make_index_sequence<MaxDepth>{})}
      , m_Front{l.size() ? 0 : MaxDepth}
      , m_Back{l.size() ? l.size() - 1 : MaxDepth}
    {
      if(l.size() > MaxDepth)
        throw std::logic_error("Attempting to exceed maximum queue depth");
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

    constexpr const T& back() const noexcept
    {
      return m_Queue[m_Back];
    }

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

    constexpr bool empty() const noexcept
    {
      return m_Front == MaxDepth;
    }

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

    friend constexpr bool operator==(const static_queue& lhs, const static_queue& rhs) noexcept
    {
      return (lhs.m_Front == rhs.m_Front) && (lhs.m_Back == rhs.m_Back) && (lhs.m_Queue == rhs.m_Queue);
    }

    friend constexpr bool operator!=(const static_queue& lhs, const static_queue& rhs) noexcept
    {
      return !(lhs == rhs);
    }
  private:
    std::array<T, MaxDepth> m_Queue{};

    std::size_t m_Front{MaxDepth}, m_Back{MaxDepth};

    template<std::size_t... I> static constexpr std::array<T, MaxDepth>
    make_array(std::initializer_list<T> l, std::index_sequence<I...>)
    {
      return std::array<T, MaxDepth>{ make_element(l, I)...};
    }

    static constexpr T make_element(std::initializer_list<T> l, const std::size_t i)
    {
      return (i < l.size()) ?  *(l.begin() + i) : T{};
    }
  };
}
