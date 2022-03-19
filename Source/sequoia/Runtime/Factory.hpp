////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Factory implementation(s)
 */

#include "sequoia/Core/Meta/Concepts.hpp"
#include "sequoia/Core/Utilities/Iterator.hpp"
#include "sequoia/Core/Meta/Utilities.hpp"

#include <variant>
#include <array>
#include <tuple>
#include <stdexcept>
#include <algorithm>
#include <string>

namespace sequoia::runtime
{
  template<std::input_or_output_iterator Iterator>
  class factory_dereference_policy
  {
  public:
    using value_type = typename std::iterator_traits<Iterator>::value_type;
    using proxy      = std::string;
    using pointer    = typename std::iterator_traits<Iterator>::pointer;
    using reference  = typename std::iterator_traits<Iterator>::reference;

    constexpr factory_dereference_policy() = default;
    constexpr factory_dereference_policy(const factory_dereference_policy&) = default;

    [[nodiscard]]
    friend constexpr bool operator==(const factory_dereference_policy&, const factory_dereference_policy&) noexcept = default;

    [[nodiscard]]
    friend constexpr bool operator!=(const factory_dereference_policy&, const factory_dereference_policy&) noexcept = default;
  protected:
    constexpr factory_dereference_policy(factory_dereference_policy&&) noexcept = default;

    ~factory_dereference_policy() = default;

    constexpr factory_dereference_policy& operator=(const factory_dereference_policy&)     = default;
    constexpr factory_dereference_policy& operator=(factory_dereference_policy&&) noexcept = default;

    [[nodiscard]]
    proxy get(reference ref) const noexcept
    {
      return ref.first;
    }

    [[nodiscard]]
    static pointer get_ptr(reference ref) noexcept { return &ref; }
  };

  /*! \brief Generic factory with statically defined products.

      This class template requires the static specification of a variadic set of products. When
      a factory is created, a mapping from keys to products is created.

   */
  template<class... Products>
    requires (sizeof...(Products) > 0)
  class factory
  {
  public:
    using key          = std::string;
    using vessel       = std::variant<Products...>;

    [[nodiscard]]
    constexpr static std::size_t size() noexcept
    {
      return sizeof...(Products);
    }
  private:
    template<class Product>
    struct product_creator
    {
      template<class... Args>
      [[nodiscard]]
      Product operator()(Args&&... args) const
      {
        return Product{std::forward<Args>(args)...};
      }

      [[nodiscard]]
      friend bool operator==(const product_creator&, const product_creator&) noexcept = default;

      [[nodiscard]]
      friend bool operator!=(const product_creator&, const product_creator&) noexcept = default;
    };

    using creator_variant = std::variant<product_creator<Products>...>;
    using element = std::pair<key, creator_variant>;
    using storage = std::array<element, size()>;
    using const_storage_iterator = typename storage::const_iterator;
  public:
    using names_iterator = utilities::iterator<const_storage_iterator, factory_dereference_policy<const_storage_iterator>>;

    factory(const std::array<std::string_view, size()>& names)
      : m_Creators{make_array(names, std::make_index_sequence<size()>{})}
    {
      std::sort(m_Creators.begin(), m_Creators.end(), [](const element& lhs, const element& rhs){ return lhs.first < rhs.first; });

      auto comp{[](const element& lhs, const element& rhs) { return lhs.first == rhs.first; }};
      if(std::adjacent_find(m_Creators.cbegin(), m_Creators.cend(), comp) != m_Creators.cend())
        throw std::logic_error{"Factory product names must be unique!"};
    }

    template<class... Args>
      requires (std::constructible_from<Products, Args...> && ...)
    [[nodiscard]]
    vessel create(std::string_view name, Args&&... args) const
    {
      const auto found{find(name)};

      if(found == m_Creators.end())
        throw std::runtime_error{std::string{"Factory unable to create product of name '"}.append(name).append("'")};

      return std::visit(variant_visitor{[&](const auto& v) { return vessel{v(std::forward<Args>(args)...)}; }}, found->second);
    };

    template<class Product, class... Args>
      requires (    (std::is_same_v<Product, Products> || ...)
                 && (std::constructible_from<Products, Args...> && ...))
    [[nodiscard]]
    vessel create_or(std::string_view name, Args&&... args) const
    {
      auto found{find(name)};
      if(found == m_Creators.end())
      {
        found = std::find_if(m_Creators.begin(), m_Creators.end(),
                             [](const element& e){ return std::holds_alternative<product_creator<Product>>(e.second); });
      }

      return std::visit(variant_visitor{[&](const auto& v) { return vessel{v(std::forward<Args>(args)...)}; }}, found->second);
    }

    [[nodiscard]]
    friend bool operator==(const factory& lhs, const factory& rhs) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const factory&, const factory&) noexcept = default;

    [[nodiscard]]
    names_iterator begin_names() const noexcept
    {
      return names_iterator{m_Creators.begin()};
    }

    [[nodiscard]]
    names_iterator end_names() const noexcept
    {
      return names_iterator{m_Creators.end()};
    }
  private:
    storage m_Creators{};

    [[nodiscard]]
    auto find(std::string_view name) const
    {
      const auto found{std::lower_bound(m_Creators.begin(), m_Creators.end(), name,
                              [](const element& e, std::string_view n) { return e.first < n; })};

      return (found != m_Creators.end()) && (found->first == name) ? found : m_Creators.end();
    }

    template<std::size_t... I>
    [[nodiscard]]
    std::array<element, size()> make_array(const std::array<std::string_view, size()>& names, std::index_sequence<I...>)
    {
      return {make_element<I, Products>(names)...};
    }

    template<std::size_t I, class Product>
    [[nodiscard]]
    static element make_element(const std::array<std::string_view, size()>& names)
    {
      auto view{names[I]};

      if(view.empty())
        throw std::logic_error{"Factory product names must not be empty!"};

      return {std::string{view}, product_creator<Product>{}};
    }
  };
}

