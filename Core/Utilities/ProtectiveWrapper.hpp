#pragma once

#include "Utilities.hpp"

namespace sequoia
{
  namespace utilities
  {
    template <class T, bool=std::is_empty_v<T>> class protective_wrapper
    {
    public:
      using value_type = T;
      
      template<class... Args, class=std::enable_if_t<!utilities::same_decay_v<protective_wrapper, Args...>>>
      constexpr explicit protective_wrapper(Args&&... args) : m_Type{std::forward<Args>(args)...} {}

      constexpr protective_wrapper(const protective_wrapper&)                = default;
      constexpr protective_wrapper(protective_wrapper&&) noexcept            = default;
      constexpr protective_wrapper& operator=(const protective_wrapper&)     = default;
      constexpr protective_wrapper& operator=(protective_wrapper&&) noexcept = default;

      template<class Arg, class... Args>
      constexpr void set(Arg&& arg, Args&&... args)
      {
        m_Type = T{std::forward<Arg>(arg), std::forward<Args>(args)...};
      }

      template<class Fn>
      constexpr void mutate(Fn fn)
      {
        fn(m_Type);
      }

      constexpr const T& get() const noexcept { return m_Type; }
    private:
      T m_Type;
    };

    template<class T> class protective_wrapper<T, true>
    {
    public:
      using value_type = T;
      
      constexpr protective_wrapper(const protective_wrapper&)                = default;
      constexpr protective_wrapper(protective_wrapper&&) noexcept            = default;
      constexpr protective_wrapper& operator=(const protective_wrapper&)     = default;
      constexpr protective_wrapper& operator=(protective_wrapper&&) noexcept = default;
    private:
    };


    //===================================Comparison operators===================================//

    template<class T>
    constexpr bool operator==(const protective_wrapper<T>& lhs, const protective_wrapper<T>& rhs) noexcept
    {
      return lhs.get() == rhs.get();
    }

    template<class T>
    constexpr bool operator!=(const protective_wrapper<T>& lhs, const protective_wrapper<T>& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    template<class T>
    constexpr bool operator<(const protective_wrapper<T>& lhs, const protective_wrapper<T>& rhs) noexcept
    {
      return lhs.get() < rhs.get();
    }

    template<class T>
    constexpr bool operator>(const protective_wrapper<T>& lhs, const protective_wrapper<T>& rhs) noexcept
    {
      return lhs.get() > rhs.get();
    }

    template<class T>
    constexpr bool operator>=(const protective_wrapper<T>& lhs, const protective_wrapper<T>& rhs) noexcept
    {
      return !(lhs.get() < rhs.get());
    }

    template<class T>
    constexpr bool operator<=(const protective_wrapper<T>& lhs, const protective_wrapper<T>& rhs) noexcept
    {
      return !(lhs.get() > rhs.get());
    }
  }
}
