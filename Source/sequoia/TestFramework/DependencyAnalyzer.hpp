////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::optional<std::vector<std::filesystem::path>> tests_to_run(const std::filesystem::path& sourceRepo,
                                                                 const std::filesystem::path& testRepo,
                                                                 const std::filesystem::path& materialsRepo,
                                                                 const std::optional<std::filesystem::file_time_type>& timeStamp);
}
