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

      The registrations follow the last existing one or, if there is none, precede the line calling
      `runner.execute`; each is a line of its own, indented as the line it follows or precedes.
   */
  void add_test_registrations(const std::filesystem::path& file, const std::vector<std::string>& tests);


  struct reduced_file_contents
  {
    std::optional<std::string> working, prediction;
  };

  [[nodiscard]]
  reduced_file_contents get_reduced_file_content(const std::filesystem::path& file, const std::filesystem::path& prediction);

}
