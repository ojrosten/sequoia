////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Factory implementation(s)
 */

#include "sequoia/Core/Meta/Concepts.hpp"
#include "sequoia/Core/Object/Creator.hpp"
#include "sequoia/Core/Object/Nomenclator.hpp"
#include "sequoia/Core/ContainerUtilities/Iterator.hpp"
#include "sequoia/Core/Meta/Utilities.hpp"

#include <variant>
#include <array>
#include <map>
#include <ranges>
#include <vector>
#include <tuple>
#include <stdexcept>
#include <algorithm>
#include <string>

namespace sequoia::object
{
  /** \brief Policy to allow iteration over the names of factory products */
  template<std::input_or_output_iterator Iterator>
  class factory_dereference_policy
  {
  public:
    using value_type      = std::iterator_traits<Iterator>::value_type;
    using reference       = std::string;

    constexpr factory_dereference_policy() = default;
    constexpr factory_dereference_policy(const factory_dereference_policy&) = default;

    [[nodiscard]]
    friend constexpr bool operator==(const factory_dereference_policy&, const factory_dereference_policy&) noexcept = default;

    [[nodiscard]]
    constexpr static reference get(Iterator i)
    {
      return i->first;
    }
  protected:
    constexpr factory_dereference_policy(factory_dereference_policy&&) noexcept = default;

    ~factory_dereference_policy() = default;

    constexpr factory_dereference_policy& operator=(const factory_dereference_policy&)     = default;
    constexpr factory_dereference_policy& operator=(factory_dereference_policy&&) noexcept = default;
  };

  /** \brief Generic factory with statically defined products.

      The constructor requires a list of unique key which are internally mapped to the
      products. To generate a product, clients should call one of `make` / `make_or`.
      The former throws if the supplied string does not match a key; the latter 
      requires specification of a default product which is created in this situation.
   */

  template<class... Products>
    requires (sizeof...(Products) > 0) && (std::movable<Products> && ...)
  class factory
  {
  public:
    using key    = std::string;
    using vessel = std::variant<Products...>;

    [[nodiscard]]
    constexpr static std::size_t size() noexcept
    {
      return sizeof...(Products);
    }
  private:

    template<class Product>
    using product_creator = producer<Product, Product>;

    using creator_variant = std::variant<product_creator<Products>...>;
    using element = std::pair<key, creator_variant>;
    using storage = std::array<element, size()>;
    using const_storage_iterator = storage::const_iterator;
  public:
    using names_iterator = utilities::iterator<const_storage_iterator, factory_dereference_policy<const_storage_iterator>>;

    factory()
      requires (has_extrinsic_nomenclator<Products> && ...)
      : factory{nomenclator<Products>{}()...}
    {}

    template<class... Names>
        requires (sizeof...(Names) == size()) && (std::is_constructible_v<std::string, Names> && ...)
    factory(Names... names)
      : m_Creators{make_element<Products>(std::move(names))...}
    {
      std::ranges::sort(m_Creators, [](const element& lhs, const element& rhs){ return lhs.first < rhs.first; });

      auto comp{[](const element& lhs, const element& rhs) { return lhs.first == rhs.first; }};
      if(std::ranges::adjacent_find(m_Creators, comp) != m_Creators.cend())
        throw std::logic_error{"Factory product names must be unique!"};
    }

    template<class... Args>
      requires (initializable_from<Products, Args...> && ...)
    [[nodiscard]]
    vessel make(std::string_view name, Args&&... args) const
    {
      const auto found{find(name)};

      if(found == m_Creators.end())
        throw std::runtime_error{std::string{"Factory unable to make product of name '"}.append(name).append("'")};

      return make_from(found->second, std::forward<Args>(args)...);
    }

    template<class Product, class... Args>
      requires (    (std::is_same_v<Product, Products> || ...)
                 && (initializable_from<Products, Args...> && ...))
    [[nodiscard]]
    vessel make_or(std::string_view name, Args&&... args) const
    {
      auto found{find(name)};
      if(found == m_Creators.end())
      {
        found = std::ranges::find_if(m_Creators, [](const element& e){ return std::holds_alternative<product_creator<Product>>(e.second); });
      }

      return make_from(found->second, std::forward<Args>(args)...);
    }

    /** \brief Every product whose name satisfies the predicate.

        The order is stable from one run to the next, so a caller which writes the results somewhere
        reproducible gets the same sequence each time.

        Each product is initialized from the same arguments, which must therefore tolerate being
        used more than once.
     */

    template<class Predicate, class... Args>
      requires std::predicate<Predicate, std::string_view> && (initializable_from<Products, const Args&...> && ...)
    [[nodiscard]]
    std::vector<vessel> make_if(Predicate pred, const Args&... args) const
    {
      return   m_Creators
             | std::views::filter([&pred](const element& e){ return pred(std::string_view{e.first}); })
             | std::views::transform([&](const element& e){ return make_from(e.second, args...); })
             | std::ranges::to<std::vector>();
    }

    /** \brief Every product, in the same stable order as `make_if`. */

    template<class... Args>
      requires (initializable_from<Products, const Args&...> && ...)
    [[nodiscard]]
    std::vector<vessel> make_all(const Args&... args) const
    {
      return make_if([](std::string_view){ return true; }, args...);
    }

    [[nodiscard]]
    friend bool operator==(const factory&, const factory&) noexcept = default;

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

    template<class... Args>
    [[nodiscard]]
    static vessel make_from(const creator_variant& creator, Args&&... args)
    {
      return std::visit(overloaded{[&](const auto& v) { return vessel{v.make(std::forward<Args>(args)...)}; }}, creator);
    }

    [[nodiscard]]
    auto find(std::string_view name) const
    {
      const auto found{std::ranges::lower_bound(m_Creators, name, std::ranges::less{}, [](const element& e){ return e.first; })};

      return (found != m_Creators.end()) && (found->first == name) ? found : m_Creators.end();
    }

    template<class Product>
    [[nodiscard]]
    static element make_element(std::string name)
    {
      if(name.empty())
        throw std::logic_error{"Factory product names must not be empty!"};

      return {std::move(name), product_creator<Product>{}};
    }
  };

  /** \brief Factory whose products are registered one at a time and erased into a vessel.

      The sibling above fixes its products in its own type, which buys a compile-time guarantee that
      nothing outside the list can be made. That guarantee costs a template parameter per product,
      so it does not survive a product set numbered in thousands - which is what a test runner for a
      large project is.

      What replaces it is narrower but not nothing: every `add` is type-checked where it is written,
      since the product must be constructible from the arguments *and* fit the vessel. What is given
      up is the closed world - the ability to ask, at compile time, whether some type is among the
      products.

      The creation arguments are fixed by the factory's own type rather than by each call, which is
      where the sibling's requirement that every product be initializable from one common argument
      list reappears: there, a constraint checked per call; here, a signature.

      There is no counterpart to `make_or`. Naming a fallback product means finding the entry which
      makes that type, and erasure is precisely the loss of that knowledge; a caller wanting one can
      supply a fallback vessel instead.
   */

  template<class Vessel, class... Args>
  class erased_factory
  {
  private:
    using creator = Vessel(*)(const Args&...);
    using storage = std::map<std::string, creator, std::less<>>;
    using const_storage_iterator = storage::const_iterator;
  public:
    using key    = std::string;
    using vessel = Vessel;

    using names_iterator = utilities::iterator<const_storage_iterator, factory_dereference_policy<const_storage_iterator>>;

    /** \brief Registers a product under a name, which must be neither empty nor already taken. */

    template<class Product>
      requires initializable_from<Product, const Args&...> && std::constructible_from<Vessel, Product>
    void add(key name)
    {
      if(name.empty())
        throw std::logic_error{"Factory product names must not be empty!"};

      constexpr creator make_product{[](const Args&... args) -> Vessel { return Vessel{Product{args...}}; }};

      if(!m_Creators.emplace(std::move(name), make_product).second)
        throw std::logic_error{"Factory product names must be unique!"};
    }

    [[nodiscard]]
    std::size_t size() const noexcept
    {
      return m_Creators.size();
    }

    [[nodiscard]]
    Vessel make(std::string_view name, const Args&... args) const
    {
      const auto found{m_Creators.find(name)};

      if(found == m_Creators.end())
        throw std::runtime_error{std::string{"Factory unable to make product of name '"}.append(name).append("'")};

      return found->second(args...);
    }

    /** \brief Every product whose name satisfies the predicate.

        As for the sibling above: the order is stable from one run to the next, and each product is
        initialized from the same arguments, which must therefore tolerate being used more than once.
     */

    template<class Predicate>
      requires std::predicate<Predicate, std::string_view>
    [[nodiscard]]
    std::vector<Vessel> make_if(Predicate pred, const Args&... args) const
    {
      return   m_Creators
             | std::views::filter([&pred](const auto& e){ return pred(std::string_view{e.first}); })
             | std::views::transform([&](const auto& e){ return e.second(args...); })
             | std::ranges::to<std::vector>();
    }

    /** \brief Every product, in the same stable order as `make_if`. */

    [[nodiscard]]
    std::vector<Vessel> make_all(const Args&... args) const
    {
      return make_if([](std::string_view){ return true; }, args...);
    }

    [[nodiscard]]
    friend bool operator==(const erased_factory&, const erased_factory&) noexcept = default;

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
  };
}
