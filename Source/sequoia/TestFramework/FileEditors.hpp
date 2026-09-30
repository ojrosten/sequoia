////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Contains utilities for automatically editing certain files as part of the test creation process.
 */

#include "sequoia/Core/Meta/Concepts.hpp"

#include <filesystem>
#include <optional>
#include <vector>

namespace sequoia::testing
{
  void add_include(const std::filesystem::path& file, std::string_view includePath);

  /** \brief Adds an entry for `file` to the entries between `patternOpen` and `patternClose` in `cmakeLists`.

      The new entry is `cmakeEntryPrefix` followed by `file` relative to `hostDir`. The entries are then sorted.
      Each entry is aligned one column after the parenthesis that `patternOpen` opens.

      \throws std::logic_error if `patternOpen` has no parenthesis
      \throws std::runtime_error if `cmakeLists` cannot be read, or has no section between the patterns
   */
  void add_to_cmake(const std::filesystem::path& cmakeLists,
                    const std::filesystem::path& hostDir,
                    const std::filesystem::path& file,
                    std::string_view patternOpen,
                    std::string_view patternClose,
                    std::string_view cmakeEntryPrefix);

  /** \brief Registers each of `tests` in `file`, skipping any already registered there.

      \throws std::logic_error if `tests` is empty.
      \throws std::runtime_error if `file` cannot be read, does not contain `runner.execute`, or cannot be
              opened to write a registration.
   */
  void add_test_registrations(const std::filesystem::path& file, const std::vector<std::string>& tests);


  struct reduced_file_contents
  {
    std::optional<std::string> working, prediction;
  };

  [[nodiscard]]
  reduced_file_contents get_reduced_file_content(const std::filesystem::path& file, const std::filesystem::path& prediction);

}
