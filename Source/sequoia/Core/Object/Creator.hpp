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

#include "sequoia/Core/Meta/Concepts.hpp"

namespace sequoia::object
{
  template<class T>
  concept creator = requires(T& a) {
    requires has_value_type_v<T> || has_element_type_v<T>;
    typename T::product_type;

    { a.make(std::declval<typename T::product_type>()) } -> std::same_as<typename T::product_type>;
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
       (impl::dependent_value_type_same_as_v<Product, T>   && initializable_from<Product, T>)
    || (impl::dependent_element_type_same_as_v<Product, T> && initializable_from<Product, T*>)
  };

  template<class Product, class T>
  concept makeable_from = initializable_from<Product, T> || product_for_v<Product, T>;

  template<std::movable T, makeable_from<T> Product>
  struct direct_forwarder
  {
    template<class... Args>
      requires initializable_from<Product, Args...>
    [[nodiscard]]
    constexpr Product operator()(Args&&... args) const
    {
      return Product{std::forward<Args>(args)...};
    }
  };

  /*! \brief Creates a `product` for `T`.
  
   */

  template<std::movable T, makeable_from<T> Product, class Transformer=direct_forwarder<T, Product>>
  class producer
  {
  public:
    using product_type    = Product;
    using value_type      = T;
    using transfomer_type = Transformer;

    template<class... Args>
      requires initializable_from<product_type, std::invoke_result_t<Transformer, Args...>>
    [[nodiscard]]
    constexpr static product_type make(Args&&... args)
    {
      return product_type{Transformer{}(std::forward<Args>(args)...)};
    }

    [[nodiscard]]
    friend constexpr bool operator==(const producer&, const producer&) noexcept = default;
  };
}
