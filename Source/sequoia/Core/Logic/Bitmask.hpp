////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities to aid logical operations
  */

#include <type_traits>

namespace sequoia
{

  /*! Specialize this class template, and inherit from `std::true_type`, to cause `enum`
      to be automatically useable as bit masks. This must be done in the `sequoia` namespace.
      For example:

      <pre>
      namespace sequoia
      {
        namespace foo
        {
          enum class bar { none=0, a=1, b=2, c=4}
        }

        template<>
        struct as_bitmask<foo::bar> : std::true_type {};
      }
      </pre>

      Clients must ensure themselves that any enums for which this is done actually have a bitmask structure.
   */
  template<class T>
    requires std::is_enum_v<T>
  struct as_bitmask : std::false_type
  {};

  template<class T>
    requires std::is_enum_v<T>
  using as_bitmask_t = typename as_bitmask<T>::type;

  template<class T>
    requires std::is_enum_v<T>
  constexpr bool as_bitmask_v{as_bitmask<T>::value};

  template<class T>
    requires as_bitmask_v<T>
  [[nodiscard]]
  constexpr T operator|(T lhs, T rhs) noexcept
  {
    using underlying = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
  }

  template<class T>
    requires as_bitmask_v<T>
  constexpr T& operator|=(T& lhs, T rhs) noexcept
  {
    lhs = lhs | rhs;
    return lhs;
  }

  template<class T>
    requires as_bitmask_v<T>
  [[nodiscard]]
  constexpr T operator&(T lhs, T rhs) noexcept
  {
    using underlying = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
  }

  template<class T>
    requires as_bitmask_v<T>
  constexpr T& operator&=(T& lhs, T rhs) noexcept
  {
    lhs = lhs & rhs;
    return lhs;
  }

  template<class T>
    requires as_bitmask_v<T>
  [[nodiscard]]
  constexpr T operator~(T om)
  {
    using underlying = std::underlying_type_t<T>;
    return static_cast<T>(~static_cast<underlying>(om));
  }

  template<class T>
    requires as_bitmask_v<T>
  [[nodiscard]]
  constexpr T operator^(T lhs, T rhs) noexcept
  {
    using underlying = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
  }
}
