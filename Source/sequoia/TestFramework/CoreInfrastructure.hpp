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

  template<class T>
  inline constexpr bool has_test_equality_v{
    requires{ &value_tester<T>::test_equality; }
  };

  template<class T>
  inline constexpr bool has_test_equivalence_v{
    requires{ &value_tester<T>::test_equivalence; }
  };

  template<class T>
  inline constexpr bool has_test_weak_equivalence_v{
    requires{ &value_tester<T>::test_weak_equivalence; }
  };

  template<class T>
  inline constexpr bool has_test_agnostic_v{
    requires{ &value_tester<T>::test_agnostic; }
  };

  struct equality_tag {};
 
  struct equivalence_tag
  {
    using fallback = equality_tag;
  };

  struct weak_equivalence_tag
  {
    using fallback = equivalence_tag;
  };

  struct agnostic_tag {};

  template<class T>
  inline constexpr bool is_equivalence_disambiguator_v{
    requires { typename T::fallback; }
  };

  template<class T>
  [[nodiscard]]
  constexpr bool defines_test_for(equality_tag) noexcept
  {
    return has_test_equality_v<T>;
  }

  template<class T>
  [[nodiscard]]
  constexpr bool defines_test_for(equivalence_tag) noexcept
  {
    return has_test_equivalence_v<T>;
  }

  template<class T>
  [[nodiscard]]
  constexpr bool defines_test_for(weak_equivalence_tag) noexcept
  {
    return has_test_weak_equivalence_v<T>;
  }

  template<class T>
  [[nodiscard]]
  constexpr bool defines_test_for(agnostic_tag) noexcept
  {
    return has_test_agnostic_v<T>;
  }


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
