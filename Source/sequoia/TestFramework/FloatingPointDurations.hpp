////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Durations with a floating-point representation.
 */

#include <chrono>

namespace sequoia::testing
{
  using double_precision_seconds      = std::chrono::duration<double>;
  using double_precision_milliseconds = std::chrono::duration<double, std::milli>;
}
