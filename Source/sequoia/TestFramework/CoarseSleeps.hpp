////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Detection of sleeps rounded up to a coarse timer tick, and
           warnings about them.
 */

#include "sequoia/PlatformSpecific/PlatformDiscriminators.hpp"
#include "sequoia/TestFramework/FloatingPointDurations.hpp"

#include <string>

namespace sequoia::testing
{
  /** \brief Whether `slept`, compared to `target`, indicates sleeps rounded
             up to a coarse timer tick.
   */
  [[nodiscard]]
  bool is_coarse_sleep(double_precision_milliseconds slept, double_precision_milliseconds target);

  /** \brief A warning that sleeps of `target` lasted `slept` or more, so
             timings built on sleeps are unreliable.

      The warning names the likely cause on the platform the program is
      built for.
   */
  [[nodiscard]]
  std::string coarse_sleep_message(double_precision_milliseconds slept, double_precision_milliseconds target);

  /** \brief The warning `coarse_sleep_message(slept, target)` gives on
             Windows.
   */
  [[nodiscard]]
  std::string coarse_sleep_message(double_precision_milliseconds slept,
                                   double_precision_milliseconds target,
                                   windows_type);

  /** \brief The warning `coarse_sleep_message(slept, target)` gives on
             macOS.
   */
  [[nodiscard]]
  std::string coarse_sleep_message(double_precision_milliseconds slept,
                                   double_precision_milliseconds target,
                                   macos_type);

  /** \brief The warning `coarse_sleep_message(slept, target)` gives on
             Linux.
   */
  [[nodiscard]]
  std::string coarse_sleep_message(double_precision_milliseconds slept,
                                   double_precision_milliseconds target,
                                   linux_type);

  /** \brief The warning `coarse_sleep_message(slept, target)` gives on any
             other platform.
   */
  [[nodiscard]]
  std::string coarse_sleep_message(double_precision_milliseconds slept,
                                   double_precision_milliseconds target,
                                   other_os_type);
}
