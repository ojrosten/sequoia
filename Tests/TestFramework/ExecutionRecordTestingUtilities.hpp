////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Readers for the execution records which the test runner writes.

    A line of a record holds a label and a value. The value is the text
    after the line's last space, and the label is the text before that space.
 */

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace sequoia::testing
{
  /** \brief The lines of the execution record `record`, without their line
             breaks.

      \throws std::runtime_error if `record` cannot be opened
   */
  [[nodiscard]]
  std::vector<std::string> execution_record_lines(const std::filesystem::path& record);

  /** \brief The label of each line of the execution record `record`, in the
             order of the lines.

      \throws std::runtime_error if `record` cannot be opened
   */
  [[nodiscard]]
  std::vector<std::string> execution_record_labels(const std::filesystem::path& record);

  /** \brief The value of the line of the execution record `record` labelled
             `label`.

      \throws std::runtime_error if `record` cannot be opened, or has no line
              labelled `label`
   */
  [[nodiscard]]
  std::string execution_record_value(const std::filesystem::path& record, std::string_view label);
}
