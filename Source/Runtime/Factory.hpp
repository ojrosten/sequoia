////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "Concepts.hpp"

#include <variant>
#include <array>

namespace sequoia::runtime
{
  template<class>
  struct type_to_string
  {
    std::string make() = delete;
  };
  
  template<class... Products>
    requires (std::is_default_constructible_v<Products> && ...)
  class factory
  {
  public:
    constexpr static std::size_t num_products() noexcept
    {
      return sizeof...(Products);
    }

    factory()
      : m_Creators{std::make_pair(type_to_string<Products>(), std::variant<Products...>{Products{}})...}
    {
      std::sort(m_Creators.begin(), m_Creators.end(), [](const element& lhs, const element& rhs){ return lhs.first < rhs.first; });
    }

    [[nodiscard]]
    std::variant<Products...> create(std::string_view name) const
    {
      auto found{std::lower_bound(m_Creators.begin(), m_Creators.end(), name,
                                  [](const element& e, std::string_view n) { return e.first < n; } )};

      if((found == m_Creators.end()) || (*found != name))
        throw std::runtime_error{std::string{"Factory unable to create product of name '"}.append(name).append("'")};

      return found->second();
    };
  private:
    using element = std::pair<std::string, std::variant<Products...>>;
    
    std::array<element, num_products()> m_Creators{};
  };
}

