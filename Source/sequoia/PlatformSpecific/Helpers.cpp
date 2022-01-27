////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file
    \brief Definitions for Helpers.hpp
*/

#include "sequoia/PlatformSpecific/Helpers.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#ifdef _MSC_VER
  #include "Windows.h"
#endif
/*! \file Utilities dependent on platform-specific macros */

namespace sequoia
{
  timer_resolution::timer_resolution(std::chrono::milliseconds t)
    : m_Resolution{resolution(t)}
  {
    #ifdef _MSC_VER
      if(m_Resolution > 0) timeBeginPeriod(m_Resolution);
    #endif
  }

  timer_resolution::~timer_resolution()
  {
    #ifdef _MSC_VER
      if(m_Resolution > 0) timeEndPeriod(m_Resolution);
    #endif
  }

  [[nodiscard]]
  unsigned int timer_resolution::resolution(std::chrono::milliseconds t) noexcept
  {
    return t <= std::chrono::milliseconds{} ? 0u : static_cast<unsigned int>(t.count());
  }
}
