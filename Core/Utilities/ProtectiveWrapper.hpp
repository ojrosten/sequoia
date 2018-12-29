////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file ProtectiveWrapper.hpp
    \brief A class which allows homogenous semantics for families of classes at least one of
    which must use proxies to underlying data.
 */

#include "TypeTraits.hpp"

namespace sequoia::utilities
{
  /*! \class protective_wrapper
      \brief A wrapper which allows for getting, setting and mutation of its stored value.

      This wrapper is designed for use alongside classes which expose proxies to the
      underlying data of interest. Throughout sequoia, such proxies should have the
      same get/set/mutate interface as protective_wrapper. Thus the protective_wrapper
      allows for a homogeneous treatment of families of classes, some of which must
      necessarily use proxies but all of which are desired to have the same semantics in terms
      of getting/setting/mutating the underlying data.
   */
  
  template <class T, bool=std::is_empty_v<T>> class protective_wrapper
  {
  public:
    using value_type = T;
      
    template<class... Args, class=std::enable_if_t<!utilities::same_decay_v<protective_wrapper, Args...>>>
    constexpr explicit protective_wrapper(Args&&... args) : m_Type{std::forward<Args>(args)...} {}

    constexpr protective_wrapper(const protective_wrapper&)     = default;
    constexpr protective_wrapper(protective_wrapper&&) noexcept = default;
    ~protective_wrapper() = default;
      
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

    [[nodiscard]]
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
  [[nodiscard]]
  constexpr bool operator==(const protective_wrapper<T>& lhs, const protective_wrapper<T>& rhs) noexcept
  {
    return lhs.get() == rhs.get();
  }
  
  template<class T>
  [[nodiscard]]
  constexpr bool operator!=(const protective_wrapper<T>& lhs, const protective_wrapper<T>& rhs) noexcept
  {
    return !(lhs == rhs);
  }
  
  template<class T>
  [[nodiscard]]
  constexpr bool operator<(const protective_wrapper<T>& lhs, const protective_wrapper<T>& rhs) noexcept
  {
    return lhs.get() < rhs.get();
  }
  
  template<class T>
  [[nodiscard]]
  constexpr bool operator>(const protective_wrapper<T>& lhs, const protective_wrapper<T>& rhs) noexcept
  {
    return lhs.get() > rhs.get();
  }

  template<class T>
  [[nodiscard]]
  constexpr bool operator>=(const protective_wrapper<T>& lhs, const protective_wrapper<T>& rhs) noexcept
  {
    return !(lhs.get() < rhs.get());
  }

  template<class T>
  [[nodiscard]]
  constexpr bool operator<=(const protective_wrapper<T>& lhs, const protective_wrapper<T>& rhs) noexcept
  {
    return !(lhs.get() > rhs.get());
  }
}
