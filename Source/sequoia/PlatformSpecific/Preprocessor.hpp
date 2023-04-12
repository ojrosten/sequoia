////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file Preprocessor logic for dealing with different platforms */

#include "PlatformDiscriminators.hpp"

#include <execution>
#include <vector>

namespace sequoia
{

  #if defined(_MSC_VER)
    using compiler_constant = msvc_type;

    [[nodiscard]]
    constexpr int iterator_debug_level() noexcept
    {
      return _ITERATOR_DEBUG_LEVEL;
    }

    #define SEQUOIA_MSVC_EMPTY_BASE_HACK __declspec(empty_bases)

    #define SEQUOIA_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
  #else
    #if defined(__clang__)
      using compiler_constant = clang_type;
    #elif defined(__GNUG__)
      using compiler_constant = gcc_type;
    #else
      using compiler_constant = other_compiler_type;
    #endif

    int iterator_debug_level() noexcept;

    #define SEQUOIA_MSVC_EMPTY_BASE_HACK

    #define SEQUOIA_NO_UNIQUE_ADDRESS [[no_unique_address]]
  #endif

  #if defined(__clang__)
    namespace execution
    {
      inline constexpr int par{0};
    }
  #else
    namespace execution
    {
      inline constexpr auto par{std::execution::par};
    }
  #endif

  inline constexpr bool with_msvc_v{std::is_same_v<compiler_constant, msvc_type>};
  inline constexpr bool with_clang_v{std::is_same_v<compiler_constant, clang_type>};
  inline constexpr bool with_gcc_v{std::is_same_v<compiler_constant, gcc_type>};
}
