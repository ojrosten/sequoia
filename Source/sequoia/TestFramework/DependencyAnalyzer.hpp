////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Facility to detect changes on disk and only run the relevant tests.

 */

#include "sequoia/TestFramework/FileSystemUtilities.hpp"

namespace sequoia::testing
{
  std::vector<std::filesystem::path>& read_tests(const std::filesystem::path& file, std::vector<std::filesystem::path>& tests);

  [[nodiscard]]
  std::vector<std::filesystem::path> read_tests(const std::filesystem::path& file);

  void write_tests(const project_paths& projPaths, const std::filesystem::path& file, const std::vector<std::filesystem::path>& tests);

  [[nodiscard]]
  std::optional<std::vector<std::filesystem::path>> tests_to_run(const project_paths& projPaths, std::string_view cutoff);
}
