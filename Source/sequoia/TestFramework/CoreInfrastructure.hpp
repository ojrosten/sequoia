////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Core declarations / definitions used in the testing framework

 */

#include "sequoia/Core/Meta/Concepts.hpp"

#include <format>
#include <filesystem>
#include <string>
#include <string_view>
#include <sstream>

namespace sequoia::testing
{
  /** \brief Specialize this struct template to provide custom serialization of a given class.
      \anchor serializer_primary
   */

  template<class T>
  struct serializer;

  /** \brief Out-of-line serialization for the elementary types.

      `std::format` is instantiated afresh for every type it is asked about, and the
      framework asks about a great many. Routing the types whose formatting is fixed
      through ordinary functions, declared here and defined in the associated cpp,
      keeps that instantiation to one place. The definitions use `std::format`, so the
      output is identical by construction rather than by inspection.
   */

  namespace impl
  {
    [[nodiscard]] std::string serialize(bool);
    [[nodiscard]] std::string serialize(char);
    [[nodiscard]] std::string serialize(long long);
    [[nodiscard]] std::string serialize(unsigned long long);
    [[nodiscard]] std::string serialize(float);
    [[nodiscard]] std::string serialize(double);
    [[nodiscard]] std::string serialize(long double);
    [[nodiscard]] std::string serialize(std::string_view);
  }

  /** \brief Types whose serialization is delegated to an ordinary function. */
  template<class T>
  inline constexpr bool elementary_serializable_v{
       (std::integral<T> || std::floating_point<T>)
    || std::same_as<T, std::string> || std::same_as<T, std::string_view>
  };

  template<class T>
    requires elementary_serializable_v<T>
  struct serializer<T>
  {
    [[nodiscard]]
    static std::string make(const T& val)
    {
      if      constexpr(std::same_as<T, bool>)             return impl::serialize(val);
      else if constexpr(std::same_as<T, char>)             return impl::serialize(val);
      else if constexpr(std::signed_integral<T>)           return impl::serialize(static_cast<long long>(val));
      else if constexpr(std::unsigned_integral<T>)         return impl::serialize(static_cast<unsigned long long>(val));
      else if constexpr(std::floating_point<T>)            return impl::serialize(val);
      else                                                 return impl::serialize(std::string_view{val});
    }
  };

  template<std::formattable<char> T>
    requires (!elementary_serializable_v<T>)
  struct serializer<T>
  {
    [[nodiscard]]
    static std::string make(const T& val)
    {
      return std::format("{}", val);
    }
  };

  template<serializable_to<std::stringstream> T>
    requires (!std::formattable<T, char>) && (!elementary_serializable_v<T>)
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
  concept serializable = requires(serializer<T>& s, T& t) {
    s.make(t);
  };

  template<serializable T>
  [[nodiscard]]
  std::string to_string(const T& value)
  {
    return serializer<T>::make(value);
  }

  /** \brief Primary class template for converting unsigned types of implementation-defined size into fixed-width types. */
  template<class T>
  struct type_normalizer
  {
    using type = T;
  };

  template<class T>
    requires (std::is_unsigned_v<T> && (sizeof(T) == sizeof(uint64_t)))
  struct type_normalizer<T>
  {
    using type = uint64_t;
  };

  template<class T>
    requires (std::is_unsigned_v<T> && (sizeof(T) == sizeof(uint32_t)))
  struct type_normalizer<T>
  {
    using type = uint32_t;
  };

  template<class T>
  using type_normalizer_t = type_normalizer<T>::type;

  template<std::integral T>
  [[nodiscard]]
  auto fixed_width_unsigned_cast(T x) noexcept
  {
    using U = std::make_unsigned_t<T>;

    return static_cast<type_normalizer_t<U>>(x);
  }

  struct uncaught_exception_info
  {
    int num{};
    std::string top_level_message{};
  };
}
