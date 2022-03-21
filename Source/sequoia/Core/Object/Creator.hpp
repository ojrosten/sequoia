////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    
 */

#include <concepts>

namespace sequoia::object
{
  template<class T>
  concept creator = requires(T& a) {
    typename T::product_type;
    typename T::value_type;

    { a.make() } -> std::same_as<typename T::product_type>;
  };

  template<class Product, class T>
  concept producer_for = requires { std::same_as<T, typename Product::value_type>; };

  template<class T, producer_for<T> Product>
  class producer
  {
  public:
    using product_type = Product;
    using value_type   = T;

    template<class... Args>
      requires std::constructible_from<product_type, Args...>
    [[nodiscard]]
    constexpr static product_type make(Args&&... args)
    {
      return product_type{std::forward<Args>(args)...};
    }

    [[nodiscard]]
    friend constexpr bool operator==(const producer&, const producer&) noexcept = default;

    [[nodiscard]]
    friend constexpr bool operator!=(const producer&, const producer&) noexcept = default;
  };
}