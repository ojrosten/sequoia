////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file macros for use within the testing framework */

namespace sequoia::testing
{
  #ifdef _MSC_VER
    inline constexpr bool has_msvc_v{true};
    #define SPECULATIVE_CONSTEVAL constexpr
    constexpr int iterator_debug_level() noexcept
    {
      return _ITERATOR_DEBUG_LEVEL;
    }
  #else
    inline constexpr bool has_msvc_v{false};
    #define SPECULATIVE_CONSTEVAL consteval
    int iterator_debug_level() noexcept;
  #endif
}
