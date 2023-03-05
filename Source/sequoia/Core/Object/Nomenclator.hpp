////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for associating types with names
*/

#include <string>
#include <type_traits>

namespace sequoia::object
{
  template<class T>
  inline constexpr bool has_name_value{
    requires(const T& t) { { t.name } -> std::convertible_to<std::string>; }
  };

  template<class T>
  inline constexpr bool has_name_function{
    requires(const T& t) { { t.name() } -> std::convertible_to<std::string>; }
  };

  template<class T>
  struct nomenclator
  {
    static std::string name()         = delete;
    static std::string name(const T&) = delete;
  };

  template<class T>
    requires has_name_value<T> || has_name_function<T>
  struct nomenclator<T>
  {
    [[nodiscard]]
    static std::string name(const T& t)
    {
      if constexpr(has_name_function<T>)
        return t.name();
      else
        return t.name;
    }
  };

  template<class T>
  inline constexpr bool has_extrinsic_nomenclator{
    requires { { nomenclator<T>::name() } -> std::convertible_to<std::string>; }
  };

  template<class T>
  inline constexpr bool has_intrinsic_nomenclator{
    requires(const T& t) { { nomenclator<T>::name(t) } -> std::convertible_to<std::string>; }
  };
}
