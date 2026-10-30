////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"

export module sequoia.test_framework:InstabilityAnalysis;

import std;

export import sequoia.core.functional;
export import sequoia.core.meta;
export import sequoia.core.object;

/** \file
    \brief Analysis of the failures of repeated runs of tests.
 */

export namespace sequoia::testing
{
  /** \brief A function from a failure's message to the form in which it is
             compared with the same failure from other runs.
   */
  using message_projection = erased_function<std::string(std::string_view) const>;

  /** \brief Reports each test whose `trials` runs did not all record the
             same failures.

      Each failure's message passes through `projection` before the runs are
      compared, so that what the projection discards, such as measurements, is
      not a difference.
   */
  [[nodiscard]]
  std::string instability_analysis(const std::filesystem::path& root,
                                   const std::size_t trials,
                                   const message_projection& projection);
}
