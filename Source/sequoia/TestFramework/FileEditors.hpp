////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Contains utilities for automatically editing certain files as part of creating and removing tests.

    Each `remove_` function undoes what its `add_` counterpart writes, and, like `std::filesystem::remove`,
    returns whether there was anything to remove.
 */

#include "sequoia/Core/Meta/Concepts.hpp"

#include <filesystem>
#include <optional>
#include <vector>

namespace sequoia::testing
{
  void add_include(const std::filesystem::path& file, std::string_view includePath);

  /** \brief Removes from `file` each line which is exactly the include of `includePath` that `add_include` writes.

      \throws std::runtime_error if `file` cannot be read, or cannot be opened to write the edit.
   */
  bool remove_include(const std::filesystem::path& file, std::string_view includePath);

  void add_to_cmake(const std::filesystem::path& cmakeLists,
                    const std::filesystem::path& hostDir,
                    const std::filesystem::path& file,
                    std::string_view patternOpen,
                    std::string_view patternClose,
                    std::string_view cmakeEntryPrefix);

  /** \brief Removes the entry for `file`, spelt as `add_to_cmake` spells it, from the first list in `cmakeLists`
      which `patternOpen` opens and `patternClose` closes.

      A file with no such list has no entry to remove.

      \throws std::runtime_error if `cmakeLists` cannot be read, or cannot be opened to write the edit.
   */
  bool remove_from_cmake(const std::filesystem::path& cmakeLists,
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

  /** \brief Removes from `file` each line which, leading blanks aside, begins with the registration of one of `tests`.

      \throws std::runtime_error if `file` cannot be read, or cannot be opened to write the edit.
   */
  bool remove_test_registrations(const std::filesystem::path& file, const std::vector<std::string>& tests);


  struct reduced_file_contents
  {
    std::optional<std::string> working, prediction;
  };

  [[nodiscard]]
  reduced_file_contents get_reduced_file_content(const std::filesystem::path& file, const std::filesystem::path& prediction);

}
