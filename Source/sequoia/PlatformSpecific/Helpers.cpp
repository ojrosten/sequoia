////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/PlatformSpecific/Helpers.hpp"

#include "sequoia/Maths/Arithmetic/ArithmeticCasts.hpp"

#ifdef _WIN32
  #include "Windows.h"
#endif

namespace sequoia
{
  namespace
  {
    #ifdef _WIN32
      [[nodiscard]]
      unsigned int request(unsigned int resolution) noexcept
      {
        return timeBeginPeriod(resolution) == TIMERR_NOERROR ? resolution : 0;
      }

      void end_request(unsigned int resolution) noexcept
      {
        if(resolution > 0) timeEndPeriod(resolution);
      }
    #else
      [[nodiscard]]
      unsigned int request(unsigned int) noexcept { return 0; }

      void end_request(unsigned int) noexcept {}
    #endif
  }

  timer_resolution::timer_resolution(std::chrono::milliseconds t)
    : m_Resolution{request(maths::checked_conversion_to<unsigned int>(t.count()))}
  {}

  timer_resolution::~timer_resolution()
  {
    end_request(m_Resolution);
  }
}
