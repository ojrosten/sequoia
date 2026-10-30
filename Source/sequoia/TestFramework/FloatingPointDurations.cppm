////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.test_framework:FloatingPointDurations;

import std;

/** \file
    \brief Durations with a floating-point representation.
 */

export namespace sequoia::testing
{
  using double_precision_seconds      = std::chrono::duration<double>;
  using double_precision_milliseconds = std::chrono::duration<double, std::milli>;
}
