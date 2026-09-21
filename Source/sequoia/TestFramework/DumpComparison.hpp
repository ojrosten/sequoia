////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Compares two dumps of a test run, check by check.

    A dump holds, for every check that ran, the text which would have been printed had it
    failed: its source location, its description and the types compared. Comparing a dump
    against one kept from before a change names the checks the change removed and the checks it
    added, which a check count cannot.
 */

#include <filesystem>
#include <string>
#include <vector>

namespace sequoia::testing
{
  /** \brief The checks in one dump and not the other.

      A check is identified by everything the dump records of it but its line number, so a
      check moved by an edit above it is neither missing nor new. Checks are counted, so an
      instantiation lost from a template instantiated several times is one missing check.
   */
  struct dump_comparison
  {
    std::vector<std::string> missing{}, added{};

    [[nodiscard]]
    friend bool operator==(const dump_comparison&, const dump_comparison&) noexcept = default;
  };

  /** \brief The checks recorded in a dump, one string each without surrounding newlines, in the order run. */
  [[nodiscard]]
  std::vector<std::string> read_dump(const std::filesystem::path& dump);

  /** \brief Compares the checks of a dump with a baseline's.

      \returns The checks in the baseline and not the dump (`missing`), and in the dump and not
               the baseline (`added`), each in the order they appear in the dump which holds them.
      \throws std::runtime_error if either file cannot be read.
   */
  [[nodiscard]]
  dump_comparison compare_dumps(const std::filesystem::path& baseline, const std::filesystem::path& dump);

  /** \brief A report of a comparison, naming the baseline: a summary line, then the missing and
             added checks under headings, each as its location and description.
   */
  [[nodiscard]]
  std::string to_string(const dump_comparison& comparison, std::string_view baselineName);
}
