////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "Concepts.hpp"

#include <memory>
#include <functional>
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
    template<class T>
    struct mutating_product_visitor
    {
      virtual ~mutating_product_visitor() = default;
      virtual void visit(T&) = 0;
    };

    
    struct abstract_mutating_visitor : mutating_product_visitor<Products>...
    {};
    
    class vessel
    {
    public:
      template<class T>
        requires (same_as<T, Products> || ...)
      vessel(T&& t)
        : m_Handle{std::make_unique<essence<T>>(std::forward<T>(t))}
      {}

      vessel(const vessel& other)
        : m_Handle{other.clone()}
      {}

      vessel(vessel&&) noexcept = default;

      vessel& operator=(const vessel& other)
      {
        m_Handle = other.clone();
        return *this;
      }

      vessel& operator=(vessel&&) noexcept = default;
    private:
      struct soul
      {
        virtual ~soul() = default;

        virtual std::unique_ptr<soul> clone() const = 0;
        
        virtual void accept(abstract_mutating_visitor&) = 0;
      };

      template<class T>
      struct essence final : soul
      {
        ~essence() final = default;

        std::unique_ptr<soul> clone() const final
        {
          return std::make_unique<essence>(m_T);
        }
        
        void accept(abstract_mutating_visitor& v) final
        {
          v.visit(m_T);
        }
        
        T m_T;
      };

      std::unique_ptr<soul> clone() const noexcept
      {
        return m_Handle ? m_Handle->clone() : nullptr;
      }

      std::unique_ptr<soul> m_Handle{};
    };

    constexpr static std::size_t num_products() noexcept
    {
      return sizeof...(Products);
    }

    factory()
      : m_Creators{std::make_pair(type_to_string<Products>(), to_function<Products>())...}
    {
      std::sort(m_Creators.begin(), m_Creators.end(), [](const element& lhs, const element& rhs){ return lhs.first < rhs.first; });
    }

    [[nodiscard]]
    vessel create(std::string_view name) const
    {
      auto found{std::lower_bound(m_Creators.begin(), m_Creators.end(), name,
                                  [](const element& e, std::string_view n) { return e.first < n; } )};

      if((found == m_Creators.end()) || (*found != name))
        throw std::runtime_error{std::string{"Factory unable to create product of name '"}.append(name).append("'")};

      return found->second();
    };
  private:
    using element = std::pair<std::string, std::function<vessel ()>>;
    
    std::array<element, num_products()> m_Creators{};

    template<class T>
    std::function<vessel ()> to_function()
    {
      return [](){ return vessel{T{}}; };
    }
  };
}

