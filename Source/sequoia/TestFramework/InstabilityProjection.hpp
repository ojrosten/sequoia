////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief The form in which a search for instabilities compares failures.
 */

#include <string>
#include <string_view>

namespace sequoia::testing
{
  /** \brief Returns `message` with every measurement in its summary lines set
             to zero.

      Measurements and summary lines are as defined by
      `text_with_zeroed_measurements`. The number of trials and the attempt
      are among the values set to zero, since they can differ from run to
      run.
   */
  [[nodiscard]]
  std::string instability_projection(std::string_view message);
}
