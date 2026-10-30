////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.test_framework:InstabilityProjection;

import std;

/** \file
    \brief The form in which a search for instabilities compares failures.
 */

export namespace sequoia::testing
{
  /** \brief Returns `message` with every measurement in its summary lines set
             to zero.

      Measurements and summary lines are as defined by
      `text_with_zeroed_measurements`.
   */
  [[nodiscard]]
  std::string instability_projection(std::string_view message);
}
