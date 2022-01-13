////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Core declarations / definitions

 */

#include "sequoia/Core/Meta/Concepts.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"

#include <string>
#include <sstream>
#include <filesystem>

namespace sequoia::testing
{
  /*! \brief class template, specializations of which implement detailed comparison of two instantiations of T.
      \anchor value_tester_primary
   */
  template<class T> struct value_tester;

  struct equality_check_t {};
 
  struct equivalence_check_t
  {
    using fallback = equality_check_t;
  };

  struct weak_equivalence_check_t
  {
    using fallback = equivalence_check_t;
  };

  struct agnostic_check_t {};

  inline constexpr equality_check_t equality{};
  inline constexpr equivalence_check_t equivalence{};
  inline constexpr weak_equivalence_check_t weak_equivalence{};
  inline constexpr agnostic_check_t agnostic{};

  template<class T>
  inline constexpr bool is_equivalence_disambiguator_v{
    requires { typename T::fallback; }
  };

  /*! \brief Specialize this struct template to provide custom serialization of a given class.
      \anchor serializer_primary
   */

  template<class T>
  struct serializer;

  template<serializable_to<std::stringstream> T>
  struct serializer<T>
  {
    [[nodiscard]]
    static std::string make(const T& val)
    {
      std::ostringstream os{};
      os << std::boolalpha << val;
      return os.str();
    }
  };

  template<class T>
  concept serializable = requires(serializer<T>& s, T& t)
  {
    s.make(t);
  };

  template<serializable T>
  [[nodiscard]]
  std::string to_string(const T& value)
  {
    return serializer<T>::make(value);
  }

  template<std::integral T>
  [[nodiscard]]
  auto fixed_width_unsigned_cast(T x)
  {
    using U = std::make_unsigned_t<T>;

    if constexpr(sizeof(U) == sizeof(uint64_t))
    {
      return static_cast<uint64_t>(x);
    }
    else if constexpr(sizeof(U) == sizeof(uint32_t))
    {
      return static_cast<uint32_t>(x);
    }
    else
    {
      return x;
    }
  }

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
    return static_cast<T>(static_cast<int>(lhs) | static_cast<int>(rhs));
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
    return static_cast<T>(static_cast<int>(lhs) & static_cast<int>(rhs));
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
    return static_cast<T>(~static_cast<int>(om));
  }

  template<class T>
    requires as_bitmask_v<T>
  [[nodiscard]]
  constexpr T operator^(T lhs, T rhs) noexcept
  {
    return static_cast<T>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
  }

  struct uncaught_exception_info
  {
    int num{};
    std::string top_level_message{};
  };

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL
  std::size_t operator "" _sz(unsigned long long int n) noexcept
  {
    return static_cast<std::size_t>(n);
  }
}
