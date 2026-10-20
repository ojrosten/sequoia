////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include <concepts>
#include <filesystem>
#include <string>
#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  inline std::filesystem::path fake_project_root(const std::filesystem::path& materials)
  {
    return materials / "FakeProject";
  }

  [[nodiscard]]
  inline std::filesystem::path fake_project_executable(const std::filesystem::path& materials)
  {
    return fake_project_root(materials).append("build/CMade/FakeExe.txt");
  }

  /** \brief Returns the command line which runs the fake project in
             `materials` with `arguments`.
   */
  template<class... Arguments>
    requires (std::convertible_to<const Arguments&, std::string> && ...)
  [[nodiscard]]
  std::vector<std::string> fake_project_commandline(const std::filesystem::path& materials,
                                                    const Arguments&... arguments)
  {
    return {fake_project_executable(materials).generic_string(), arguments...};
  }
}
