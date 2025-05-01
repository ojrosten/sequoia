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

#ifdef EXPOSE_SEQUOIA_BITMASK
#define NAMESPACE_SEQUOIA_AS_BITMASK inline namespace sequoia_bitmask
#else
#define NAMESPACE_SEQUOIA_AS_BITMASK namespace sequoia
#endif

NAMESPACE_SEQUOIA_AS_BITMASK
{

  /*! Specialize this class template, and inherit from `std::true_type`, to cause an `enum`
      to be automatically useable as a bit mask. If `EXPOSE_SEQUOIA_BITMASK` is not defined,
      this must be done in the `sequoia` namespace. If `EXPOSE_SEQUOIA_BITMASK` is defined,
      then `as_bitmask` lives in the namespace `sequoia_bitmask`, which is inlined. This
      effectively makes `as_bitmask` available in the global namespace, but referable by
      something which can be clearly traced back to `sequoia`. The purpose of this is to
      allow clients of `sequoia`, by appropriately specializing `as_bitmask`, to
      automatically expose the bitwise operators defined below in their project's namespaces.

      For example:

      <pre>
      namespace foo
      {
        enum class bar { none=0, a=1, b=2, c=4}
      }

      NAMESPACE_SEQUOIA_AS_BITMASK
      {
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
  constexpr T operator^(T lhs, T rhs) noexcept
  {
    using underlying = std::underlying_type_t<T>;
    return static_cast<T>(static_cast<underlying>(lhs) ^ static_cast<underlying>(rhs));
  }

  template<class T>
    requires as_bitmask_v<T>
  constexpr T& operator^=(T& lhs, T rhs) noexcept
  {
    lhs = lhs ^ rhs;
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
}