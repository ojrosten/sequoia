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

#include "sequoia/TestFramework/ProjectPaths.hpp"

namespace sequoia::testing
{
  enum class prune_mode { passive, active };

  std::vector<std::filesystem::path>& read_tests(const std::filesystem::path& file, std::vector<std::filesystem::path>& tests);

  [[nodiscard]]
  std::vector<std::filesystem::path> read_tests(const std::filesystem::path& file);

  void write_tests(const project_paths& projPaths, const std::filesystem::path& file, const std::vector<std::filesystem::path>& tests);

  [[nodiscard]]
  std::optional<std::vector<std::filesystem::path>> tests_to_run(const project_paths& projPaths, std::string_view cutoff);

  void update_prune_files(const project_paths& projPaths,
                          std::vector<std::filesystem::path> failedTests,
                          std::filesystem::file_time_type updateTime,
                          std::optional<std::size_t> id);

  void update_prune_files(const project_paths& projPaths,
                          std::vector<std::filesystem::path> executedTests,
                          std::vector<std::filesystem::path> failedTests,
                          std::optional<std::size_t> id);

  void setup_instability_analysis_prune_folder(const project_paths& projPaths);

  void aggregate_instability_analysis_prune_files(const project_paths& projPaths, prune_mode mode, std::filesystem::file_time_type timeStamp, std::size_t numReps);
}
