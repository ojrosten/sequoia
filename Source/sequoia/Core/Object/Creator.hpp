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
    requires has_value_type<T> || has_element_type<T>;
    typename T::product_type;
    
    { a.make() } -> std::same_as<typename T::product_type>;
  };

  namespace impl
  {
    template<class Product, class T>
    inline constexpr bool dependent_value_type_same_as_v{
      requires{
        typename Product::value_type;

        requires std::same_as<T, typename Product::value_type>;
      }
    };

    template<class Product, class T>
    inline constexpr bool dependent_element_type_same_as_v{
      requires{
        typename Product::element_type;

        requires std::same_as<T, typename Product::element_type>;
      }
    };
  }

  template<class Product, class T>
  inline constexpr bool product_for_v{
       (impl::dependent_value_type_same_as_v<Product, T>   && std::constructible_from<Product, T>)
    || (impl::dependent_element_type_same_as_v<Product, T> && std::constructible_from<Product, T*>)
  };

  template<class Product, class T>
  concept makeable_from = std::convertible_to<T, Product> || product_for_v<Product, T>;

  template<class T, makeable_from<T> Product>
  struct direct_forwarder
  {
    template<class... Args>
      requires std::constructible_from<Product, Args...>
    [[nodiscard]]
    constexpr Product operator()(Args&&... args) const
    {
      return Product{std::forward<Args>(args)...};
    }
  };

  /*! \brief Creates a `product` for `T`.
  
   */

  template<class T, makeable_from<T> Product, class Transformer=direct_forwarder<T, Product>>
  class producer
  {
  public:
    using product_type    = Product;
    using value_type      = T;
    using transfomer_type = Transformer;

    template<class... Args>
      requires std::constructible_from<product_type, std::invoke_result_t<Transformer, Args...>>
    [[nodiscard]]
    constexpr static product_type make(Args&&... args)
    {
      return Transformer{}(std::forward<Args>(args)...);
    }

    [[nodiscard]]
    friend constexpr bool operator==(const producer&, const producer&) noexcept = default;

    [[nodiscard]]
    friend constexpr bool operator!=(const producer&, const producer&) noexcept = default;
  };
}