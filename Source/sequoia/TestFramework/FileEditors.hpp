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

  void add_to_cmake(const std::filesystem::path& cmakeLists,
                    const std::filesystem::path& hostDir,
                    const std::filesystem::path& file,
                    std::string_view patternOpen,
                    std::string_view patternClose,
                    std::string_view cmakeEntryPrefix);

  /** \brief Registers each of `tests` not already registered in `file`, a main which calls `runner.execute`.

      The point of registration is the first occurrence of the text `runner.execute`, comments included.
      The registrations are inserted, in the order given, straight after the last line before that one
      which holds anything, each on a line of its own indented as the line calling `runner.execute`. A
      test counts as registered when some line, leading whitespace aside, begins with its registration.

      \throws std::logic_error if `tests` is empty.
      \throws std::runtime_error if `file` cannot be read or holds no `runner.execute`.
   */
  void add_test_registrations(const std::filesystem::path& file, const std::vector<std::string>& tests);


  struct reduced_file_contents
  {
    std::optional<std::string> working, prediction;
  };

  [[nodiscard]]
  reduced_file_contents get_reduced_file_content(const std::filesystem::path& file, const std::filesystem::path& prediction);

}
