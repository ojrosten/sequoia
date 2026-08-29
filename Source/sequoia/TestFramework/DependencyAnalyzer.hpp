////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Facility to detect changes on disk and only run the relevant tests.

 */

#include "sequoia/TestFramework/ProjectPaths.hpp"

#include <iostream>
#include <chrono>
#include <format>
#include <limits>

namespace sequoia::testing
{
  enum class prune_mode { passive, active };

  struct prune_record
  {
    using stamp_t    = std::filesystem::file_time_type;
    using duration_t = stamp_t::duration;

    std::filesystem::path test_path;
    stamp_t time_stamp;

    friend std::ostream& operator<<(std::ostream& s, const prune_record& record) {      
      return s <<  record.test_path.generic_string()
               << ' '
               << std::format("{}", record.time_stamp.time_since_epoch().count());
    }

    [[nodiscard]]
    friend auto operator<=>(const prune_record&, const prune_record&) noexcept = default;

    friend std::istream& operator>>(std::istream& s, prune_record& record) {
      std::size_t duration{};
      s >> record.test_path >> duration;

      record.time_stamp = {stamp_t{} + duration_t{duration}};
      return s;
    }
  };

  [[nodiscard]]
  std::vector<prune_record> read_tests(const std::filesystem::path& file);

  void write_tests(const project_paths& projPaths, const std::filesystem::path& file, std::span<const prune_record> tests);

  [[nodiscard]]
  std::optional<std::vector<std::filesystem::path>> tests_to_run(const project_paths& projPaths, std::string_view cutoff);

  void update_prune_files(const project_paths& projPaths,
                          std::span<const std::filesystem::path> failedTests,
                          std::filesystem::file_time_type updateTime,
                          std::optional<std::size_t> id);

  void update_prune_files(const project_paths& projPaths,
                          std::span<const std::filesystem::path> executedTests,
                          std::span<const std::filesystem::path> failedTests,
                          std::filesystem::file_time_type updateTime,
                          std::optional<std::size_t> id);

  void setup_instability_analysis_prune_folder(const project_paths& projPaths);

  void aggregate_instability_analysis_prune_files(const project_paths& projPaths, prune_mode mode, std::filesystem::file_time_type timeStamp, std::size_t numReps);
}
