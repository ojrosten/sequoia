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
  /** \brief The form of a dump, shared by the logger which writes one and the reader.

      A top-level check is its message, then the message of every check nested in it, one per
      line, then the separator. An empty message writes nothing.
   */
  namespace dump_format
  {
    constexpr std::string_view check_separator{"\n=======================================\n\n"};
  }

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

  /** \brief The top-level checks recorded in a dump, each with its nested checks, without
             surrounding newlines, in the order run. A check which recorded nothing is not among them.
   */
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
             added checks under headings, each as its location and the first line after it.
   */
  [[nodiscard]]
  std::string to_string(const dump_comparison& comparison, std::string_view baselineName);
}
