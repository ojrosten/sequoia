////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/Core/Meta/Concepts.hpp"
#include "sequoia/Core/Utilities/Iterator.hpp"

#include <variant>
#include <array>
#include <tuple>
#include <stdexcept>
#include <algorithm>
#include <string>

namespace sequoia::runtime
{
  template<class... Products>
    requires (sizeof...(Products) > 0)
  class factory
  {
  public:
    using key    = std::string;
    using vessel = std::variant<Products...>;

    constexpr static std::size_t size() noexcept
    {
      return sizeof...(Products);
    }

    template<class... Args>
      requires (std::constructible_from<Products, Args...> && ...)
    factory(const std::array<std::string_view, size()>& names, Args&&... args)
      : m_Creators{make_array(names, std::make_index_sequence<size()>{}, args...)}
    {
      // Note: arguments not forwarded above to void accidentally reusing moved-from objects
      // const& avoided to allow for mutable state to be passed in
      std::sort(m_Creators.begin(), m_Creators.end(), [](const element& lhs, const element& rhs){ return lhs.first < rhs.first; });

      auto comp{[](const element& lhs, const element& rhs) { return lhs.first == rhs.first; }};
      if(std::adjacent_find(m_Creators.cbegin(), m_Creators.cend(), comp) != m_Creators.cend())
        throw std::logic_error{"Factory product names must be unique!"};
    }

    [[nodiscard]]
    vessel create(std::string_view name) const
    {
      const auto found{find(name)};

      if(found == m_Creators.end())
        throw std::runtime_error{std::string{"Factory unable to create product of name '"}.append(name).append("'")};

      return found->second;
    };

    template<class Product>
      requires (std::is_same_v<Product, Products> || ...)
    [[nodiscard]]
    vessel create_or(std::string_view name) const noexcept
    {
      auto found{find(name)};
      if(found == m_Creators.end())
      {
        found = std::find_if(m_Creators.begin(), m_Creators.end(),
                             [](const element& e){ return std::holds_alternative<Product>(e.second); });
      }

      return found->second;
    }

    [[nodiscard]]
    friend bool operator==(const factory&, const factory&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const factory&, const factory&) noexcept = default;

    [[nodiscard]]
    auto begin() const noexcept
    {
      return m_Creators.begin();
    }

    [[nodiscard]]
    auto end() const noexcept
    {
      return m_Creators.end();
    }
  private:
    using element = std::pair<key, vessel>;

    std::array<element, size()> m_Creators{};

    [[nodiscard]]
    auto find(std::string_view name) const
    {
      const auto found{std::lower_bound(m_Creators.begin(), m_Creators.end(), name,
                              [](const element& e, std::string_view n) { return e.first < n; })};

      return (found != m_Creators.end()) && (found->first == name) ? found : m_Creators.end();
    }

    template<std::size_t... I, class... Args>
    [[nodiscard]]
    std::array<element, size()> make_array(const std::array<std::string_view, size()>& names, std::index_sequence<I...>, Args&&... args)
    {
      return {make_element<I, Products>(names, std::forward<Args>(args)...)...};
    }

    template<std::size_t I, class Product, class... Args>
    [[nodiscard]]
    static element make_element(const std::array<std::string_view, size()>& names, Args&&... args)
    {
      auto view{names[I]};

      if(view.empty())
        throw std::logic_error{"Factory product names must not be empty!"};

      return {std::string{view}, Product{std::forward<Args>(args)...}};
    }
  };
}

