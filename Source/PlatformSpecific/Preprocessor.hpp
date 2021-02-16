////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>

/*! \file Utilities dependent on platform-specific macros */

namespace sequoia
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

    class [[nodiscard]] timer_resolution
    {
      unsigned int m_Resolution{1};
    public:
      explicit timer_resolution(unsigned int millisecs);

      ~timer_resolution();

      [[nodiscard]]
      unsigned int resolution() const noexcept
      {
        return m_Resolution;
      }
    };
}
