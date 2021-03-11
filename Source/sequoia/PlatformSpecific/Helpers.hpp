////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file Platform-dependent utilities */

namespace sequoia
{
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
