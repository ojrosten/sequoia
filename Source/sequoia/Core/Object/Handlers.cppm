////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include <array>
#include <concepts>
#include <functional>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

export module sequoia.core.object:Handlers;

import :Creator;
import :HandlerTraits;
export import sequoia.core.meta;

/** \file
    \brief Structs to enable homogenous treatment of data which is/is not
    handled via shared pointers.

 */



export namespace sequoia::object
{
  template<class T>
  struct make_shared_braced
  {
    template<class... Args>
      requires initializable_from<T, Args...>
    [[nodiscard]]
    std::shared_ptr<T> operator()(Args&&... args) const
    {
      return std::shared_ptr<T>(new T{std::forward<Args>(args)...});
    }
  };

  template<class T>
  struct shared
  {
  public:
    using product_type  = std::shared_ptr<T>;
    using value_type    = T;
    using producer_type = producer<T, std::shared_ptr<T>, make_shared_braced<T>>;

    [[nodiscard]]
    static T& get(product_type& ptr) noexcept
    {
      return *ptr;
    }

    [[nodiscard]]
    static const T& get(const product_type& ptr) noexcept
    {
      return *ptr;
    }

    [[nodiscard]]
    static T* get_ptr(product_type& ptr) noexcept
    {
      return ptr.get();
    }

    [[nodiscard]]
    static const T* get_ptr(const product_type& ptr) noexcept
    {
      return ptr.get();
    }
  };

  template<class T>
  struct by_value
  {
  public:
    using product_type  = T;
    using value_type    = T;
    using producer_type = producer<T, T>;

    [[nodiscard]]
    constexpr static T& get(T& in) noexcept { return in; }

    [[nodiscard]]
    constexpr static const T& get(const T& in) noexcept { return in; }

    [[nodiscard]]
    constexpr static T* get_ptr(T& in) noexcept { return &in; }

    [[nodiscard]]
    constexpr static const T* get_ptr(const T& in) noexcept { return &in; }
  };
}
