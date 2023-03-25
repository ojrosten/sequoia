////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief A class which allows homogenous semantics for families of classes, particularly
    where at least one uses proxies to underlying data.
 */

#include "sequoia/Core/Meta/Concepts.hpp"

#include <compare>

namespace sequoia::object
{
  template<class W>
  concept uniform_wrapper = requires(W& w, const W& cw) {
    typename W::value_type;

    { cw.get() } -> std::same_as<const typename W::value_type&>;

    w.mutate([](typename W::value_type&) {});

    w.set(std::declval<typename W::value_type>());
  };

  template<class W>
  concept transparent_wrapper = uniform_wrapper<W> && requires (W& w) {
    typename W::value_type;

    { w.get() } -> std::same_as<typename W::value_type&>;
  };

  /*! \brief A wrapper which allows for getting, setting and mutation of its stored value.

      This wrapper is designed for use alongside classes which expose proxies to the
      underlying data of interest. Throughout sequoia, such proxies should have the
      same get/set/mutate interface as faithful_wrapper. Thus the faithful_wrapper
      allows for a homogeneous treatment of families of classes, some of which must
      necessarily use proxies but all of which are desired to have the same semantics in terms
      of getting/setting/mutating the underlying data.

      The exception to this is that the faithful_wrapper includes a non-const getter.
   */

  template <class T> class faithful_wrapper
  {
  public:
    using value_type = T;

    template<class... Args>
      requires (!resolve_to_copy_v<faithful_wrapper, Args...>)
    constexpr explicit faithful_wrapper(Args&&... args) : m_Type{std::forward<Args>(args)...} {}

    template<class... Args>
      requires initializable_from<T, Args...>
    constexpr void set(Args&&... args)
    {
      m_Type = T{std::forward<Args>(args)...};
    }

    template<std::invocable<T&> Fn>
    constexpr std::invoke_result_t<Fn, T&> mutate(Fn fn)
    {
      return fn(m_Type);
    }

    [[nodiscard]]
    constexpr const T& get() const noexcept { return m_Type; }

    [[nodiscard]]
    constexpr T& get() noexcept { return m_Type; }

    [[nodiscard]]
    friend bool operator==(const faithful_wrapper&, const faithful_wrapper&) noexcept = default;

    [[nodiscard]]
    friend auto operator<=>(const faithful_wrapper&, const faithful_wrapper&) noexcept = default;
  private:
    T m_Type;
  };
}
