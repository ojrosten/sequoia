////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief A class which allows homogenous semantics for families of classes, particularly
    where at least one uses proxies to underlying data.
 */

#include "Concepts.hpp"

#include <compare>

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
  
  template <class T> class protective_wrapper
  {
  public:
    using value_type = T;
      
    template<class... Args>
      requires (!resolve_to_copy<protective_wrapper, Args...>)
    constexpr explicit protective_wrapper(Args&&... args) : m_Type{std::forward<Args>(args)...} {}

    template<class... Args>
    constexpr void set(Args&&... args)
    {
      m_Type = T{std::forward<Args>(args)...};
    }

    template<invocable<T&> Fn>
    constexpr void mutate(Fn fn)
    {
      fn(m_Type);
    }

    [[nodiscard]]
    constexpr const T& get() const noexcept { return m_Type; }

    [[nodiscard]]
    friend bool operator==(const protective_wrapper&, const protective_wrapper&) noexcept = default;

    [[nodiscard]]
    friend auto operator<=>(const protective_wrapper&, const protective_wrapper&) noexcept = default;
  private:
    T m_Type;
  };
}
