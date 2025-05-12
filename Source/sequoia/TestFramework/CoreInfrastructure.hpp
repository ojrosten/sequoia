////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Core declarations / definitions used in the testing framework

 */

#include "sequoia/Core/Meta/Concepts.hpp"

#include <format>
#include <filesystem>
#include <sstream>

namespace sequoia::testing
{
  /*! \brief Specialize this struct template to provide custom serialization of a given class.
      \anchor serializer_primary
   */

  template<class T>
  struct serializer;

  template<std::formattable<char> T>
  struct serializer<T>
  {
    [[nodiscard]]
    static std::string make(const T& val)
    {
      return std::format("{}", val);
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
  std::string to_string(const T& value)
  {
    return serializer<T>::make(value);
  }

  /*! \brief Primary class template for converting unsigned types of implementation-defined size into fixed-width types. */
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
  using type_normalizer_t = typename type_normalizer<T>::type;

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
