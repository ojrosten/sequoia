////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "Concepts.hpp"

#include <variant>
#include <array>
#include <tuple>

namespace sequoia::runtime
{  
  template<class... Products>
    requires (sizeof...(Products) > 0) && (std::is_default_constructible_v<Products> && ...)
  class factory
  {
  public:
    constexpr static std::size_t size() noexcept
    {
      return sizeof...(Products);
    }

    factory(const std::array<std::string_view, size()>& names)
      : m_Creators{make_array(names, std::make_index_sequence<size()>{})}
    {
      std::sort(m_Creators.begin(), m_Creators.end(), [](const element& lhs, const element& rhs){ return lhs.first < rhs.first; });
    }

    [[nodiscard]]
    std::variant<Products...> create(std::string_view name) const
    {
      auto found{std::lower_bound(m_Creators.begin(), m_Creators.end(), name,
                                  [](const element& e, std::string_view n) { return e.first < n; } )};

      if((found == m_Creators.end()) || (found->first != name))
        throw std::runtime_error{std::string{"Factory unable to create product of name '"}.append(name).append("'")};

      return found->second;
    };

    [[nodiscard]]
    friend bool operator==(const factory&, const factory&) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const factory&, const factory&) noexcept = default;
  private:
    using element = std::pair<std::string, std::variant<Products...>>;
    
    std::array<element, size()> m_Creators{};

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
      return {std::string{names[I]}, Product{}};
    }
  };
}

