////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Element types for testing how containers construct and destroy their
           elements.
 */

#include "sequoia/TestFramework/CoreInfrastructure.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace sequoia::testing
{
  /** \brief An element type implicitly constructible from any type.

      Braced construction of a container of this element type, from a container, an initializer list or an allocator,
      selects the initializer-list constructor: each argument becomes an element.
   */
  struct constructible_from_anything
  {
    int value{};

    constexpr constructible_from_anything() = default;

    constexpr constructible_from_anything(const int initialValue)
      : value{initialValue}
    {}

    template<class U>
    constexpr constructible_from_anything(const U&) {}

    [[nodiscard]]
    constexpr bool operator==(const constructible_from_anything&) const noexcept = default;
  };

  template<>
  struct serializer<constructible_from_anything>
  {
    [[nodiscard]]
    static std::string make(const constructible_from_anything& x) { return std::to_string(x.value); }
  };

  template<class T, class U>
  inline constexpr bool equality_expression_valid_v{
    requires(const T& t, const U& u) { t == u; }
  };

  // A hidden-friend operator== would be found by ADL for any type naming this one, and with the converting
  // constructor would make, for example, an iterator comparable with an int
  static_assert(!equality_expression_valid_v<std::vector<constructible_from_anything>::iterator, int>);

  // Likewise a hidden-friend operator<<, which would make a vector of this type serializable as a single element
  static_assert(!serializable<std::vector<constructible_from_anything>>);

  /** \brief The lineage of an object: the object, and every object which a
             chain of moves from it reaches.

      -# A copy, constructed or assigned, starts a lineage of its own. The
         lineage of the object copied counts the copy.
      -# A move, constructed or assigned, puts its target in the lineage of
         the object moved from.
      -# Destroying an object, or assigning to it, removes the object from its
         lineage.

      Every lineage compares equal to every other: a lineage is no part of a
      value.
   */
  class observable_lineage
  {
  public:
    observable_lineage() = default;

    observable_lineage(const observable_lineage& other)
      : observable_lineage{}
    {
      ++*other.m_Record->copies;
    }

    observable_lineage(observable_lineage&& other) noexcept
      : m_Record{other.m_Record}
    {}

    observable_lineage& operator=(const observable_lineage& other)
    {
      auto fresh{std::make_shared<record>()};
      ++*other.m_Record->copies;
      m_Record = std::move(fresh);
      return *this;
    }

    observable_lineage& operator=(observable_lineage&& other) noexcept
    {
      m_Record = other.m_Record;
      return *this;
    }

    /// Returns an observer which expires when the lineage ends
    [[nodiscard]]
    std::weak_ptr<const void> observer() const { return m_Record; }

    /** \brief Returns the number of copies made of objects in the lineage, as
               a count which outlives the lineage.
     */
    [[nodiscard]]
    std::shared_ptr<const std::size_t> copies() const { return m_Record->copies; }

    [[nodiscard]]
    friend bool operator==(const observable_lineage&, const observable_lineage&) noexcept { return true; }
  private:
    struct record
    {
      std::shared_ptr<std::size_t> copies{std::make_shared<std::size_t>()};
    };

    std::shared_ptr<record> m_Record{std::make_shared<record>()};
  };

  /** \brief An element type for checking that a container neither copies nor
             keeps an element it removes.
   */
  struct element_with_lineage
  {
    int value{};
    observable_lineage lineage{};

    element_with_lineage(const int initialValue)
      : value{initialValue}
    {}

    [[nodiscard]]
    bool operator==(const element_with_lineage&) const noexcept = default;
  };

  template<>
  struct serializer<element_with_lineage>
  {
    [[nodiscard]]
    static std::string make(const element_with_lineage& x) { return std::to_string(x.value); }
  };
}
