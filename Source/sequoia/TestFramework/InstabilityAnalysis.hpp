////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Analysis of the failures of repeated runs of tests.
 */

#include "sequoia/Core/Functional/ErasedFunction.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

namespace sequoia::testing
{
  /** \brief A map from a failure's message to the form in which it is compared
             with the same failure from other runs.
   */
  using message_projection = erased_function<std::string(std::string_view) const>;

  /** \brief Reports each test whose runs, `trials` of them, failed
             differently.

      Each failure's message passes through `projection` before the runs are
      compared, so that what the projection discards, such as measurements, is
      not a difference.
   */
  [[nodiscard]]
  std::string instability_analysis(const std::filesystem::path& root,
                                   const std::size_t trials,
                                   const message_projection& projection);
}
