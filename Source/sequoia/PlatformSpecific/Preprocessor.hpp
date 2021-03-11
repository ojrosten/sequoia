////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

/*! \file Preprocessor logic for dealing with different platforms */

namespace sequoia
{
  #ifdef _MSC_VER
    inline constexpr bool has_msvc_v{true};
    inline constexpr bool has_clang_v{false};
    inline constexpr bool has_gcc_v{false};

    #define SPECULATIVE_CONSTEVAL constexpr
    constexpr int iterator_debug_level() noexcept
    {
      return _ITERATOR_DEBUG_LEVEL;
    }
  #else
    inline constexpr bool has_msvc_v{false};
    #define SPECULATIVE_CONSTEVAL consteval
    int iterator_debug_level() noexcept;
    #ifdef __clang__
      inline constexpr bool has_clang_v{true};
      inline constexpr bool has_gcc_v{false};
    #elif defined(__GNUG__)
      inline constexpr bool has_clang_v{false};
      inline constexpr bool has_gcc_v{true};
    #endif
  #endif
}
