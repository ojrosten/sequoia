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
#include <string_view>
#include <vector>

namespace sequoia::testing
{
  /** \brief The checks in one dump and not the other.

      A check is identified by everything the dump records of it but the line numbers of its
      locations, so a check moved by an edit above it is neither missing nor added. Checks are
      counted, so an instantiation lost from a template instantiated several times is one
      missing check; by the same token a check deleted at one place and written again, with the
      same text, at another is neither missing nor added.
   */
  struct dump_comparison
  {
    std::vector<std::string> missing{}, added{};

    [[nodiscard]]
    friend bool operator==(const dump_comparison&, const dump_comparison&) noexcept = default;
  };

  /** \brief Reads the top-level checks recorded in `dump`, in the order run.

      Each check is read with its nested checks, and without surrounding
      newlines. A check which recorded nothing is not among them.

      \throws std::runtime_error if `dump` is not a regular file,
              or cannot be opened.
   */
  [[nodiscard]]
  std::vector<std::string> read_dump(const std::filesystem::path& dump);

  /** \brief Compares the checks of a dump with a baseline's.

      \returns The checks in the baseline and not the dump (`missing`), and in the dump and not
               the baseline (`added`), each in the order they appear in the dump which holds them.
      \throws std::runtime_error if `baseline` or `dump` is not a regular file,
              or cannot be opened.
   */
  [[nodiscard]]
  dump_comparison compare_dumps(const std::filesystem::path& baseline, const std::filesystem::path& dump);

  /** \brief Returns a report of `comparison`.

      The report names `baselineName` as the baseline, counts the missing and
      added checks, and lists each group under a heading. Each check is shown
      as its location and, if there is one, the first non-empty line after the
      location.
   */
  [[nodiscard]]
  std::string to_string(const dump_comparison& comparison, std::string_view baselineName);
}
