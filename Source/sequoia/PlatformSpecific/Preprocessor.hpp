////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Tools for generating compiler-specific behaviour.
 */

#include <vector>
#include <type_traits>

/*! \file Preprocessor logic for dealing with different platforms */

namespace sequoia
{
  enum class compiler_flavour { clang, gcc, msvc, other };

  template<compiler_flavour F>
  using compiler_flavour_constant = std::integral_constant<compiler_flavour, F>;

  using clang_type          = compiler_flavour_constant<compiler_flavour::clang>;
  using gcc_type            = compiler_flavour_constant<compiler_flavour::gcc>;
  using msvc_type           = compiler_flavour_constant<compiler_flavour::msvc>;
  using other_compiler_type = compiler_flavour_constant<compiler_flavour::other>;

  #ifdef _MSC_VER
    using compiler_constant = msvc_type;

  [[nodiscard]]
    constexpr int iterator_debug_level() noexcept
    {
      return _ITERATOR_DEBUG_LEVEL;
    }

    #define MSVC_EMPTY_BASE_HACK __declspec(empty_bases)

    #define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
  #else
    #if defined(__clang__)
      using compiler_constant = clang_type;
    #elif defined(__GNUG__)
      using compiler_constant = gcc_type;
    #else
      using compiler_constant = other_compiler_type;
    #endif

    int iterator_debug_level() noexcept;

    #define MSVC_EMPTY_BASE_HACK

    #define NO_UNIQUE_ADDRESS [[no_unique_address]]
  #endif

  inline constexpr bool with_msvc_v{std::is_same_v<compiler_constant, msvc_type>};
  inline constexpr bool with_clang_v{std::is_same_v<compiler_constant, clang_type>};
  inline constexpr bool with_gcc_v{std::is_same_v<compiler_constant, gcc_type>};
}
