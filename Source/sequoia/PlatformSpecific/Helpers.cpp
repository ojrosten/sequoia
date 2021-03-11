////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "Helpers.hpp"
#include "Preprocessor.hpp"
#ifdef _MSC_VER
  #include "Windows.h"
#endif
/*! \file Utilities dependent on platform-specific macros */

namespace sequoia
{

  timer_resolution::timer_resolution(unsigned int millisecs)
    : m_Resolution{millisecs}
  {
    #ifdef _MSC_VER
      timeBeginPeriod(m_Resolution);
    #endif
  }

  timer_resolution::~timer_resolution()
  {
    #ifdef _MSC_VER
      timeEndPeriod(m_Resolution);
    #endif
  }
}
