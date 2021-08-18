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
  /*! \brief class template, specializations of which implement detailed comparison of two instantiations of T;
      \anchor detailed_equality_checker_primary
   */
  template<class T> struct detailed_equality_checker;

  /*! \brief class template, specializations of which implement comparison of two equivalent types
      \anchor equivalence_checker_primary
   */
  template<class T, class... Us> struct equivalence_checker;


  /*! \brief class template, specializations of which implement comparison of two weakly equivalent types;
      \anchor weak_equivalence_checker_primary
   */
  template<class T, class... Us> struct weak_equivalence_checker;

  template<class T>
  inline constexpr bool has_equivalence_checker_v{class_template_is_default_instantiable<equivalence_checker, T>};

  template<class T>
  inline constexpr bool has_weak_equivalence_checker_v{class_template_is_default_instantiable<weak_equivalence_checker, T>};

  template<class T>
  inline constexpr bool has_detailed_equality_checker_v{class_template_is_default_instantiable<detailed_equality_checker, T>};

  struct equality_tag{};
 
  struct equivalence_tag
  {
    template<class T>
    using checker = equivalence_checker<T>;
  };

  struct weak_equivalence_tag
  {
    template<class T>
    using checker = weak_equivalence_checker<T>;
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

  template<class T>
    requires std::is_integral_v<T>
  auto fixed_width_unsigned_cast(T x)
  {
    using U = std::make_unsigned_t<decltype(x)>;

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

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL
  std::size_t operator "" _sz(unsigned long long int n) noexcept
  {
    return static_cast<std::size_t>(n);
  }
}
