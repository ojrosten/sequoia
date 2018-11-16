#pragma once

#include "Algorithms.hpp"
#include "ArrayUtilities.hpp"

#include <array>
#include <functional>

namespace sequoia::data_structures
{
  template<class T, std::size_t MaxDepth, class Compare=std::less<T>>
  class static_priority_queue : private Compare
  {
  public:
    constexpr static_priority_queue() = default;

    constexpr explicit static_priority_queue(const Compare& compare) : Compare{compare} {}

    constexpr static_priority_queue(std::initializer_list<T> l)
      : m_Q{make_Q(l, Compare{})}
      , m_End{l.size()}
    {}

    constexpr static_priority_queue(std::initializer_list<T> l, const Compare& compare)
      : Compare{compare}
      , m_Q{make_Q(l, compare)}
      , m_End{l.size()}
    {}

    constexpr static_priority_queue(const static_priority_queue&)     = default;
    constexpr static_priority_queue(static_priority_queue&&) noexcept = default;

    ~static_priority_queue() = default;

    constexpr static_priority_queue& operator=(const static_priority_queue&)     = default;
    constexpr static_priority_queue& operator=(static_priority_queue&&) noexcept = default;
    
    constexpr void push(const T& val)
    {
      if(m_End == MaxDepth)
        throw std::logic_error("Attempting to exceed max priority_queue depth");
      
      m_Q[m_End] = val;
      ++m_End;

      bubble_up(m_Q.begin(), m_Q.begin() + m_End - 1, static_cast<Compare&>(*this));
    }

    constexpr const T& top() const noexcept
    {
      return *m_Q.begin();
    }

    constexpr void pop() noexcept
    {
      sequoia::iter_swap(m_Q.begin(), m_Q.begin() + m_End -1);
      --m_End;
      sequoia::make_heap(m_Q.begin(), m_Q.begin() + m_End, static_cast<Compare&>(*this));
    }

    constexpr bool empty() const noexcept
    {
      return m_End == 0;
    }

    constexpr std::size_t size() const noexcept
    {
      return m_End;
    }

    friend constexpr bool operator==(const static_priority_queue& lhs, const static_priority_queue& rhs) noexcept
    {
      return (lhs.m_End == rhs.m_End) && (lhs.m_Q == rhs.m_Q);
    }

    friend constexpr bool operator!=(const static_priority_queue& lhs, const static_priority_queue& rhs) noexcept
    {
      return !(lhs == rhs);
    }
  private:
    std::array<T, MaxDepth> m_Q{};

    std::size_t m_End{};

    static constexpr std::array<T, MaxDepth> make_Q(std::initializer_list<T> l, const Compare& compare)
    {
      auto q{utilities::to_array<MaxDepth>(l)};
      sequoia::make_heap(q.begin(), q.end(), compare);

      return q;
    }
  };
}
