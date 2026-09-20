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

#include <array>
#include <charconv>
#include <format>
#include <filesystem>
#include <limits>
#include <sstream>

namespace sequoia::testing
{
  /** \brief Specialize this struct template to provide custom serialization of a given class.
      \anchor serializer_primary
   */

  template<class T>
  struct serializer;

  template<std::formattable<char> T>
  struct serializer<T>
  {
    [[nodiscard]]
    constexpr static std::string make(const T& val)
    {
      // std::format is not constexpr; in a constant evaluation an integer is rendered with to_chars
      // and anything else is named as unrendered
      if consteval
      {
        if constexpr(std::integral<T> && !std::is_same_v<T, bool>)
        {
          std::array<char, std::numeric_limits<T>::digits10 + 3> buffer{};
          const auto [end, ec]{std::to_chars(buffer.data(), buffer.data() + buffer.size(), val)};
          return std::string{buffer.data(), end};
        }
        else if constexpr(std::is_same_v<T, bool>)
        {
          return val ? "true" : "false";
        }
        else
        {
          return "<not rendered in a constant evaluation>";
        }
      }
      else
      {
        return std::format("{}", val);
      }
    }
  };

  template<serializable_to<std::stringstream> T>
    requires (!std::formattable<T, char>)
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
  constexpr std::string to_string(const T& value)
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
