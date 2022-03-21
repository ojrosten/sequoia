////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Traits, Concepts and basic utilities for the creation of objects.
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
  inline constexpr bool product_for_v{
    requires{
      typename Product::value_type;

      requires std::same_as<T, typename Product::value_type>;
      requires std::constructible_from<Product, T>;
    }
  };

  template<class Product, class T>
  concept makeable_from = std::convertible_to<T, Product> || product_for_v<Product, T>;

  /*! \brief Creates a `product` for `T`.
  
   */

  template<class T, makeable_from<T> Product, class Projection=std::identity>
  class producer
  {
  public:
    using product_type = Product;
    using value_type   = T;

    template<class... Args>
      requires std::constructible_from<product_type, std::invoke_result_t<Projection, Args>...>
    [[nodiscard]]
    constexpr static product_type make(Args&&... args)
    {
      return product_type{Projection{}(std::forward<Args>(args))...};
    }

    [[nodiscard]]
    friend constexpr bool operator==(const producer&, const producer&) noexcept = default;

    [[nodiscard]]
    friend constexpr bool operator!=(const producer&, const producer&) noexcept = default;
  };
}